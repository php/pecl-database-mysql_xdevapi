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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_UTIL_FUNCTIONS_H
#define MYSQL_XDEVAPI_UTIL_FUNCTIONS_H

#include "arguments.h"

namespace mysqlx::util {

void verify_call_parameters(
	bool is_method,
	zend_execute_data* execute_data,
	const char* type_spec);

template<typename ...Args>
int get_method_arguments(
	zend_execute_data* execute_data,
	zval* this_ptr,
	const char* type_spec,
	Args&&... args)
{
#ifdef MYSQL_XDEVAPI_DEV_MODE
	verify_call_parameters(true, execute_data, type_spec);
#endif
	return zend_parse_method_parameters(
		ZEND_NUM_ARGS(),
		this_ptr,
		type_spec,
		std::forward<Args>(args)...);
}

template<typename ...Args>
int get_function_arguments(
	zend_execute_data* execute_data,
	const char* type_spec,
	Args&&... args)
{
#ifdef MYSQL_XDEVAPI_DEV_MODE
	verify_call_parameters(false, execute_data, type_spec);
#endif
	return zend_parse_parameters(
		ZEND_NUM_ARGS(),
		type_spec,
		std::forward<Args>(args)...);
}

} // namespace mysqlx::util

#define	MYSQL_XDEVAPI_PHP_METHOD(class_name, name) \
static void class_name##_##name##_body(INTERNAL_FUNCTION_PARAMETERS); \
static PHP_METHOD(class_name, name) \
{ \
	util::safe_call_php_method(class_name##_##name##_body, INTERNAL_FUNCTION_PARAM_PASSTHRU); \
} \
static void class_name##_##name##_body(INTERNAL_FUNCTION_PARAMETERS)


#define	MYSQL_XDEVAPI_PHP_FUNCTION(name) \
static void function_##name##_body(INTERNAL_FUNCTION_PARAMETERS); \
PHP_FUNCTION(name) \
{ \
	util::safe_call_php_function(function_##name##_body, INTERNAL_FUNCTION_PARAM_PASSTHRU); \
} \
static void function_##name##_body(INTERNAL_FUNCTION_PARAMETERS)

#endif // MYSQL_XDEVAPI_UTIL_FUNCTIONS_H
