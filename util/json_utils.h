/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
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
#ifndef MYSQL_XDEVAPI_UTIL_JSON_UTILS_H
#define MYSQL_XDEVAPI_UTIL_JSON_UTILS_H

#include "strings.h"
#include "xmysqlnd/proto_gen/mysqlx_datatypes.pb.h"

namespace mysqlx {

namespace util {

class zvalue;

namespace json {

void encode_document(zval* src, zval* dest);
util::zvalue encode_document(const util::zvalue& src);

util::zvalue parse_document(const char* doc, const std::size_t doc_len);
util::zvalue parse_document(const util::string_view& doc);
// util::zvalue parse_document(const std::string_view& options_json);


bool can_be_document(const util::zvalue& value);
bool can_be_array(const util::zvalue& value);
bool can_be_binding(const util::zvalue& value);

util::zvalue ensure_doc_id(
	zval* raw_doc,
	const string_view& id);

//rapidjson::Document parse_doc(const char* doc, std::size_t doc_len);

util::zvalue to_zval(const char* doc, std::size_t doc_len);

void to_any(const char* doc, std::size_t doc_len, Mysqlx::Datatypes::Any& any);

} // namespace json

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_JSON_UTILS_H
