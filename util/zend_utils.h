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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H
#define MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H

#include <utility>

namespace mysqlx {

namespace util {

namespace zend {

void ensure_is_array(zval* zv);

void free_error_info_list(
	MYSQLND_ERROR_INFO* error_info,
	zend_bool persistent);

// ----------------

void verify_call_parameters(
	bool is_method,
	zend_execute_data* execute_data,
	const char* type_spec);

template<typename ...Params>
int parse_method_parameters(
	zend_execute_data* execute_data,
	zval* this_ptr,
	const char* type_spec,
	Params&&... params)
{
#ifdef MYSQL_XDEVAPI_DEV_MODE
	verify_call_parameters(true, execute_data, type_spec);
#endif
	return zend_parse_method_parameters(
		ZEND_NUM_ARGS(),
		this_ptr,
		type_spec,
		std::forward<Params>(params)...);
}

template<typename ...Params>
int parse_function_parameters(
	zend_execute_data* execute_data,
	const char* type_spec,
	Params&&... params)
{
#ifdef MYSQL_XDEVAPI_DEV_MODE
	verify_call_parameters(false, execute_data, type_spec);
#endif
	return zend_parse_parameters(
		ZEND_NUM_ARGS(),
		type_spec,
		std::forward<Params>(params)...);
}

// ----------------

bool is_module_loaded(const char* module_name);
bool is_openssl_loaded();

} // namespace zend

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_ZEND_UTILS_H
