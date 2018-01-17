/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#ifndef MYSQL_XDEVAPI_PHPUTILS_JSON_UTILS_H
#define MYSQL_XDEVAPI_PHPUTILS_JSON_UTILS_H

namespace mysqlx {

namespace util {

struct string_view;

namespace json {

void to_zv_string(zval* src, zval* dest);

void ensure_doc_id(
	zval* raw_doc,
	const string_view& id,
	zval* doc_with_id);

void ensure_doc_id_as_string(
	const string_view& doc_id,
	zval* doc);

} // namespace json

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHPUTILS_JSON_UTILS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
