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
#ifndef MYSQL_XDEVAPI_UTIL_COMPILER_UTILS_H
#define MYSQL_XDEVAPI_UTIL_COMPILER_UTILS_H

#define UNUSED(x) (void)x

#define UNUSED_INTERNAL_FUNCTION_PARAMETERS() \
	UNUSED(return_value); \
	UNUSED(execute_data)

#define UNUSED_FUNC_ARGS() \
	UNUSED(module_number); \
	UNUSED(type)

#define UNUSED_INIT_FUNC_ARGS int /*type*/, int /*module_number*/
#define UNUSED_SHUTDOWN_FUNC_ARGS int /*type*/, int /*module_number*/


#ifdef PHP_WIN32

#define MYSQLX_SUPPRESS_ALL_WARNINGS() \
	__pragma(warning(push, 0))

#define MYSQLX_SUPPRESS_MSVC_WARNINGS(...) \
	__pragma(warning(push)) \
	__pragma(warning(disable :  __VA_ARGS__))

#define MYSQLX_RESTORE_WARNINGS() \
	__pragma(warning(pop))

#else

#define MYSQLX_SUPPRESS_ALL_WARNINGS()
#define MYSQLX_SUPPRESS_MSVC_WARNINGS(...)
#define MYSQLX_RESTORE_WARNINGS()

#endif

#endif // MYSQL_XDEVAPI_UTIL_COMPILER_UTILS_H
