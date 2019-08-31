/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "hash_table.h"
#include "value.h"

namespace mysqlx {

namespace util {

namespace json {

/* {{{ to_zv_string */
void to_zv_string(zval* src, zval* dest)
{
	smart_str buf = { 0 };
	JSON_G(error_code) = PHP_JSON_ERROR_NONE;
	JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;
	const int encode_flag = (Z_TYPE_P(src) == IS_OBJECT) ? PHP_JSON_FORCE_OBJECT : 0;
	php_json_encode(&buf, src, encode_flag);

	if (JSON_G(error_code) != PHP_JSON_ERROR_NONE) {
		smart_str_free(&buf);
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
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
/* }}} */

util::zvalue to_zv_string(const util::zvalue& src)
{
	util::zvalue dest;
	to_zv_string(src.ptr(), dest.ptr());
	return dest;
}

namespace {

class Ensure_doc_id
{
	public:
		Ensure_doc_id(
			zval* raw_doc,
			const string_view& doc_id,
			zval* doc_with_id);

	public:
		void run();

	private:
		void process_string();
		void process_array();
		void process_object();

		void decode_json(zval* doc_as_str);
		void store_id();

	private:
		zval* raw_doc{nullptr};
		const string_view& doc_id;
		zval* doc_with_id{nullptr};

};

//------------------------------------------------------------------------------

Ensure_doc_id::Ensure_doc_id(zval* src, const string_view& id, zval* dest)
	: raw_doc(src)
	, doc_id(id)
	, doc_with_id(dest)
{
}

void Ensure_doc_id::run()
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

	ZVAL_DUP(doc_with_id, raw_doc);
	store_id();
}

void Ensure_doc_id::process_object()
{
	zval doc_as_str;
	to_zv_string(raw_doc, &doc_as_str);
	decode_json(&doc_as_str);
	zval_dtor(&doc_as_str);
	store_id();
}

void Ensure_doc_id::decode_json(zval* doc_as_str)
{
	assert(Z_TYPE_P(doc_as_str) == IS_STRING);
	char* json_str = Z_STRVAL_P(doc_as_str);
	int json_len = static_cast<int>(Z_STRLEN_P(doc_as_str));
	php_json_decode(doc_with_id, json_str, json_len, true, PHP_JSON_PARSER_DEFAULT_DEPTH);

	if (Z_TYPE_P(doc_with_id) != IS_ARRAY) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
}

void Ensure_doc_id::store_id()
{
	if (Z_TYPE_P(doc_with_id) != IS_ARRAY) return;

	Hash_table ht(doc_with_id, false);
	const char* Id_column_name = "_id";
	ht.insert(Id_column_name, doc_id);
}

} // anonymous namespace

/* {{{ ensure_doc_id */
void ensure_doc_id(
	zval* raw_doc,
	const string_view& id,
	zval* doc_with_id)
{
	ZVAL_UNDEF(doc_with_id);
	Ensure_doc_id edi(raw_doc, id, doc_with_id);
	edi.run();
}
/* }}} */

/* {{{ ensure_doc_id_as_string */
void ensure_doc_id_as_string(
	const string_view& doc_id,
	zval* doc)
{
	zval doc_with_string_id;
	util::json::ensure_doc_id(
		doc,
		doc_id,
		&doc_with_string_id);
	to_zv_string(&doc_with_string_id, doc);
	zval_dtor(&doc_with_string_id);
}
/* }}} */

} // namespace json

} // namespace util

} // namespace mysqlx
