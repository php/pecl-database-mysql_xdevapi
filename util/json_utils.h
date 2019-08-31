/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include <boost/version.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace mysqlx {

namespace util {

class zvalue;

namespace json {

void to_zv_string(zval* src, zval* dest);
util::zvalue to_zv_string(const util::zvalue& src);

void ensure_doc_id(
	zval* raw_doc,
	const string_view& id,
	zval* doc_with_id);

void ensure_doc_id_as_string(
	const string_view& doc_id,
	zval* doc);

/*
	in older versions of boost (e.g. 1.53.0 which is at the moment still officially
	delivered as the newest one package for CentOS7) there is bug in boost::property_tree
	it doesn't support strings with custom allocator - somewhere deep in code there is
	applied std::string directly with standard allocator
	compiler fails at conversion std::string <=> util::string (custom allocator)
	in newer versions it is fixed
	the oldest version we've successfully tried is 1.59.0
	and beginning with that version we apply util::string, while for older one std::string
*/
#if 105900 <= BOOST_VERSION
using ptree_string = util::string;
#else
using ptree_string = std::string;
#endif

} // namespace json

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_JSON_UTILS_H
