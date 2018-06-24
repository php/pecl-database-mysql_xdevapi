/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#ifndef MYSQL_XDEVAPI_PHP_API_H
#define MYSQL_XDEVAPI_PHP_API_H

#ifdef PHP_WIN32
#pragma warning( push )
#pragma warning( disable : 4018 4244)
#endif // PHP_WIN32

extern "C" {
#include <php.h>
#undef ERROR
#undef add_method
#undef inline
#undef max
}

#ifdef PHP_WIN32
#pragma warning( pop )
#endif // PHP_WIN32

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

#define MYSQLX_SUPPRESS_WARNINGS(...)
	__pragma(warning(push)) \
	__pragma(warning(disable : __VA_ARGS__))

#define MYSQLX_RESTORE_WARNINGS() \
	__pragma(warning(pop))


#define MYSQLX_HASH_FOREACH_VAL(ht, _val) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	ZEND_HASH_FOREACH_VAL(ht, _val) \
	__pragma(warning(pop))

#define MYSQLX_HASH_FOREACH_PTR(ht, _ptr) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	ZEND_HASH_FOREACH_PTR(ht, _ptr) \
	__pragma(warning(pop))

#define MYSQLX_HASH_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	ZEND_HASH_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	__pragma(warning(pop))

#else

#define MYSQLX_SUPPRESS_WARNINGS(...)
#define MYSQLX_RESTORE_WARNINGS()


#define MYSQLX_HASH_FOREACH_VAL ZEND_HASH_FOREACH_VAL
#define MYSQLX_HASH_FOREACH_PTR	ZEND_HASH_FOREACH_PTR
#define MYSQLX_HASH_FOREACH_STR_KEY_VAL ZEND_HASH_FOREACH_STR_KEY_VAL

#endif


#endif // MYSQL_XDEVAPI_PHP_API_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
