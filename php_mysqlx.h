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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef PHP_MYSQLX_H
#define PHP_MYSQLX_H

#include "php_mysql_xdevapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mysqlnd_api.h"

#define phpext_mysql_xdevapi_ptr &mysql_xdevapi_module_entry
extern zend_module_entry mysql_xdevapi_module_entry;

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(mysql_xdevapi)
	zend_bool		collect_statistics;
	zend_bool		collect_memory_statistics;
	char *			debug;					/* The actual string */
	MYSQLND_DEBUG *	dbg;					/* The DBG object for standard tracing */
	char *			trace_alloc_settings;	/* The actual string */
	MYSQLND_DEBUG *	trace_alloc;			/* The DBG object for allocation tracing */
	zend_long		net_read_timeout;
	zend_long		mempool_default_size;
	zend_long		debug_emalloc_fail_threshold;
	zend_long		debug_ecalloc_fail_threshold;
	zend_long		debug_erealloc_fail_threshold;
	zend_long		debug_malloc_fail_threshold;
	zend_long		debug_calloc_fail_threshold;
	zend_long		debug_realloc_fail_threshold;
ZEND_END_MODULE_GLOBALS(mysql_xdevapi)


PHP_MYSQL_XDEVAPI_API ZEND_EXTERN_MODULE_GLOBALS(mysql_xdevapi)
#define MYSQL_XDEVAPI_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(mysql_xdevapi, v)

#if defined(ZTS) && defined(COMPILE_DL_MYSQL_XDEVAPI)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* PHP_MYSQLX_H */
