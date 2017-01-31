/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
#ifndef XMYSQLND_H
#define XMYSQLND_H


#define PHP_XMYSQLND_VERSION "mysqlnd 1.0.0-dev"
#define XMYSQLND_VERSION_ID 10000

#if PHP_DEBUG
#define XMYSQLND_DBG_ENABLED 1
#else
#define XMYSQLND_DBG_ENABLED 0
#endif

extern "C" {
#ifdef ZTS
#include "TSRM.h"
#endif
#include "ext/mysqlnd/mysqlnd_portability.h"
}

#include "php_mysql_xdevapi.h"
#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_structs.h"

namespace mysqlx {

namespace drv {

/* Library related */
PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_init(void);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_end(void);


PHP_MYSQL_XDEVAPI_API const char *	xmysqlnd_get_client_info();
PHP_MYSQL_XDEVAPI_API unsigned int	xmysqlnd_get_client_version();

#define XMYSQLND_METHOD(class, method) 			xmysqlnd_##class##_##method##_pub

PHP_MYSQL_XDEVAPI_API extern MYSQLND_STATS *xmysqlnd_global_stats;

} // namespace drv

} // namespace mysqlx

#ifdef MARINES_0

ZEND_BEGIN_MODULE_GLOBALS(xmysqlnd)
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
ZEND_END_MODULE_GLOBALS(xmysqlnd)

PHP_MYSQL_XDEVAPI_API ZEND_EXTERN_MODULE_GLOBALS(xmysqlnd)
#define XMYSQLND_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(xmysqlnd, v)

#if defined(ZTS) && defined(COMPILE_DL_XMYSQLND)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#endif // MARINES_0


#endif	/* XMYSQLND_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
