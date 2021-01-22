/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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

zvalue encode_document(const zvalue& src)
{
	smart_str buf = { 0 };
	JSON_G(error_code) = PHP_JSON_ERROR_NONE;
	JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;
	const int encode_flag = src.is_object() ? PHP_JSON_FORCE_OBJECT : 0;
	php_json_encode(&buf, src.ptr(), encode_flag);

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

	zvalue dest(buf.s->val, buf.s->len);
	smart_str_free(&buf);
	return dest;
}

zvalue parse_document(const string_view& doc)
{
	zvalue dest;
	if (php_json_decode(
		dest.ptr(),
		const_cast<char*>(doc.data()),
		static_cast<int>(doc.length()),
		false,
		PHP_JSON_PARSER_DEFAULT_DEPTH) != SUCCESS)
	{
		throw xdevapi_exception(
			xdevapi_exception::Code::json_parse_error,
			static_cast<int>(JSON_G(error_code)));
	}
	return dest;
}

namespace {

bool is_first_char(const zvalue& value, const char chr)
{
	assert(value.is_string());
	if (value.empty()) return false;
	const char* value_str = value.c_str();
	return *value_str == chr;
}

} // anonymous namespace

bool can_be_document(const zvalue& value)
{
	return is_first_char(value, '{');
}

bool can_be_array(const zvalue& value)
{
	return is_first_char(value, '[');
}

bool can_be_binding(const zvalue& value)
{
	return is_first_char(value, ':');
}

namespace {

class Ensure_doc_id
{
	public:
		Ensure_doc_id(
			const zvalue& raw_doc,
			const string_view& doc_id);

	public:
		zvalue run();

	private:
		void process_string();
		void process_array();
		void process_object();

		void decode_json(const zvalue& doc_as_str);
		void store_id();

	private:
		const zvalue& raw_doc;
		const string_view& doc_id;
		zvalue doc_with_id;

};

//------------------------------------------------------------------------------

Ensure_doc_id::Ensure_doc_id(const zvalue& src, const string_view& id)
	: raw_doc(src)
	, doc_id(id)
{
}

zvalue Ensure_doc_id::run()
{
	switch(raw_doc.type()) {
		case zvalue::Type::String:
			process_string();
			break;

		case zvalue::Type::Array:
			process_array();
			break;

		case zvalue::Type::Object:
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
	if (raw_doc.size() <= 0) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}

	doc_with_id = raw_doc.clone();
	store_id();
}

void Ensure_doc_id::process_object()
{
	const zvalue doc_as_str = encode_document(raw_doc);
	decode_json(doc_as_str);
	store_id();
}

void Ensure_doc_id::decode_json(const zvalue& doc_as_str)
{
	assert(doc_as_str.is_string());
	doc_with_id = parse_document(doc_as_str.to_string_view());
	if (!doc_with_id.is_array() && !doc_with_id.is_object()) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
}

void Ensure_doc_id::store_id()
{
	const char* Id_column_name = "_id";
	switch (doc_with_id.type()) {
	case zvalue::Type::Array:
		doc_with_id.insert(Id_column_name, doc_id);
		break;

	case zvalue::Type::Object:
		doc_with_id.set_property(Id_column_name, doc_id);
		break;

	default:
		assert(!"unexpected doc type!");
	}
}

} // anonymous namespace

zvalue ensure_doc_id(
	const zvalue& raw_doc,
	const string_view& id)
{
	Ensure_doc_id edi(raw_doc, id);
	return edi.run();
}

} // namespace mysqlx::util::json
