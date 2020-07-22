/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
#include <zend_smart_str.h>
}
#include "json_utils.h"
#include "exceptions.h"
#include "value.h"

namespace mysqlx::util::json {

void encode_document(zval* src, zval* dest)
{
	smart_str buf = { 0 };
	JSON_G(error_code) = PHP_JSON_ERROR_NONE;
	JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;
	const int encode_flag = (Z_TYPE_P(src) == IS_OBJECT) ? PHP_JSON_FORCE_OBJECT : 0;
	php_json_encode(&buf, src, encode_flag);

	if (JSON_G(error_code) != PHP_JSON_ERROR_NONE) {
		smart_str_free(&buf);
		throw xdevapi_exception(
			xdevapi_exception::Code::json_fail,
			static_cast<int>(JSON_G(error_code)));
	}

	//TODO marines: there is fockup with lack of terminating zero, which makes troubles in
	// xmysqlnd_json_string_find_id, i.e. php_json_yyparse returns result != 0
	if (buf.s->len < buf.a)
	{
		buf.s->val[buf.s->len] = '\0';
	}
	ZVAL_UNDEF(dest);
	ZVAL_STRINGL(dest, buf.s->val, buf.s->len);
	smart_str_free(&buf);
}

util::zvalue encode_document(const util::zvalue& src)
{
	util::zvalue dest;
	encode_document(src.ptr(), dest.ptr());
	return dest;
}

util::zvalue parse_document(const char* doc, const std::size_t doc_len)
{
	util::zvalue dest;
	if (php_json_decode(
		dest.ptr(),
		const_cast<char*>(doc),
		static_cast<int>(doc_len),
		false,
		PHP_JSON_PARSER_DEFAULT_DEPTH) != SUCCESS)
	{
		throw xdevapi_exception(
			xdevapi_exception::Code::json_parse_error,
			static_cast<int>(JSON_G(error_code)));
	}
	return dest;
}

util::zvalue parse_document(const util::string_view& doc)
{
	return parse_document(doc.data(), doc.length());
}

namespace {

bool is_first_char(const util::zvalue& value, const char chr)
{
	assert(value.is_string());
	if (value.empty()) return false;
	const char* value_str = value.c_str();
	return *value_str == chr;
}

} // anonymous namespace

bool can_be_document(const util::zvalue& value)
{
	return is_first_char(value, '{');
}

bool can_be_array(const util::zvalue& value)
{
	return is_first_char(value, '[');
}

bool can_be_binding(const util::zvalue& value)
{
	return is_first_char(value, ':');
}

namespace {

class Ensure_doc_id
{
	public:
		Ensure_doc_id(
			zval* raw_doc,
			const string_view& doc_id);

	public:
		util::zvalue run();

	private:
		void process_string();
		void process_array();
		void process_object();

		void decode_json(zval* doc_as_str);
		void store_id();

	private:
		zval* raw_doc{nullptr};
		const string_view& doc_id;
		util::zvalue doc_with_id;

};

//------------------------------------------------------------------------------

Ensure_doc_id::Ensure_doc_id(zval* src, const string_view& id)
	: raw_doc(src)
	, doc_id(id)
{
}

util::zvalue Ensure_doc_id::run()
{
	switch(Z_TYPE_P(raw_doc)) {
		case IS_STRING:
			process_string();
			break;

		case IS_ARRAY:
			process_array();
			break;

		case IS_OBJECT:
			process_object();
			break;

		default:
			throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
	return doc_with_id;
}

void Ensure_doc_id::process_string()
{
	decode_json(raw_doc);
	store_id();
}

void Ensure_doc_id::process_array()
{
	HashTable* ht = Z_ARRVAL_P(raw_doc);
	if (zend_array_count(ht) <= 0) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}

	doc_with_id = zvalue::clone_from(raw_doc);
	store_id();
}

void Ensure_doc_id::process_object()
{
	util::zvalue doc_as_str;
	encode_document(raw_doc, doc_as_str.ptr());
	decode_json(doc_as_str.ptr());
	store_id();
}

void Ensure_doc_id::decode_json(zval* doc_as_str)
{
	assert(Z_TYPE_P(doc_as_str) == IS_STRING);
	const char* doc_str = Z_STRVAL_P(doc_as_str);
	std::size_t doc_len = static_cast<std::size_t>(Z_STRLEN_P(doc_as_str));
	doc_with_id = parse_document(doc_str, doc_len);
	if (!doc_with_id.is_array() && !doc_with_id.is_object()) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
}

void Ensure_doc_id::store_id()
{
	const char* Id_column_name = "_id";
	switch (doc_with_id.type()) {
	case util::zvalue::Type::Array:
		doc_with_id.insert(Id_column_name, doc_id);
		break;

	case util::zvalue::Type::Object:
		doc_with_id.set_property(Id_column_name, doc_id);
		break;

	default:
		assert(!"unexpected doc type!");
	}
}

} // anonymous namespace

util::zvalue ensure_doc_id(
	zval* raw_doc,
	const string_view& id)
{
	Ensure_doc_id edi(raw_doc, id);
	return edi.run();
}

} // namespace mysqlx::util::json
