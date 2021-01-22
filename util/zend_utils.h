/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H
#define MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H

#include <utility>
#include "strings.h"

namespace mysqlx::util::zend {

void ensure_is_array(zval* zv);

void free_error_info_list(
	MYSQLND_ERROR_INFO* error_info,
	zend_bool persistent);

// ----------------

bool is_module_loaded(const string_view& module_name);
bool is_openssl_loaded();

} // namespace mysqlx::util::zend

#endif // MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H
