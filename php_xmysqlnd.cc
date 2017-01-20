/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#include <php.h>
#undef ERROR
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_enum_n_def.h"
#include "ext/mysqlnd/mysqlnd_structs.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_priv.h"

#include "ext/standard/info.h"
#include "zend_smart_str.h"

#ifdef MARINES_0

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(xmysqlnd)
{
	char buf[32];

	php_info_print_table_start();
	php_info_print_table_header(2, "xmysqlnd", "enabled");
	php_info_print_table_row(2, "Version", xmysqlnd_get_client_info());
	php_info_print_table_row(2, "core SSL",
#ifdef XMYSQLND_SSL_SUPPORTED
								"supported");
#else
								"not supported");
#endif
	php_info_print_table_row(2, "extended SSL",
#ifdef XMYSQLND_HAVE_SSL
								"supported");
#else
								"not supported");
#endif
	php_info_print_table_row(2, "experimental features",
#ifdef XMYSQLND_EXPERIMENTAL_FEATURES
								"enabled");
#else
								"disabled");
#endif

	snprintf(buf, sizeof(buf), ZEND_LONG_FMT, XMYSQLND_G(net_read_timeout));
	php_info_print_table_row(2, "Read timeout", buf);

	php_info_print_table_row(2, "Collecting statistics", XMYSQLND_G(collect_statistics)? "Yes":"No");
	php_info_print_table_row(2, "Collecting memory statistics", XMYSQLND_G(collect_memory_statistics)? "Yes":"No");

	php_info_print_table_row(2, "Tracing", XMYSQLND_G(debug)? XMYSQLND_G(debug):"n/a");

	php_info_print_table_end();
}
/* }}} */


PHP_MYSQL_XDEVAPI_API ZEND_DECLARE_MODULE_GLOBALS(xmysqlnd)


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(xmysqlnd)
{
#if defined(COMPILE_DL_XMYSQLND) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	xmysqlnd_globals->collect_statistics = TRUE;
	xmysqlnd_globals->collect_memory_statistics = FALSE;
	xmysqlnd_globals->debug = NULL;	/* The actual string */
	xmysqlnd_globals->dbg = NULL;	/* The DBG object*/
	xmysqlnd_globals->trace_alloc_settings = NULL;
	xmysqlnd_globals->trace_alloc = NULL;
	xmysqlnd_globals->net_read_timeout = 31536000;
	xmysqlnd_globals->mempool_default_size = 16000;
	xmysqlnd_globals->debug_emalloc_fail_threshold = -1;
	xmysqlnd_globals->debug_ecalloc_fail_threshold = -1;
	xmysqlnd_globals->debug_erealloc_fail_threshold = -1;
	xmysqlnd_globals->debug_malloc_fail_threshold = -1;
	xmysqlnd_globals->debug_calloc_fail_threshold = -1;
	xmysqlnd_globals->debug_realloc_fail_threshold = -1;
}
/* }}} */


/* {{{ PHP_INI_BEGIN
*/
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("xmysqlnd.collect_statistics",	"1", 	PHP_INI_ALL,	OnUpdateBool,	collect_statistics, 		zend_xmysqlnd_globals, xmysqlnd_globals)
	STD_PHP_INI_BOOLEAN("xmysqlnd.collect_memory_statistics","0",PHP_INI_SYSTEM,OnUpdateBool,	collect_memory_statistics,	zend_xmysqlnd_globals, xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug",					NULL, 	PHP_INI_SYSTEM, OnUpdateString,	debug,						zend_xmysqlnd_globals, xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.trace_alloc",			NULL, 	PHP_INI_SYSTEM, OnUpdateString,	trace_alloc_settings,		zend_xmysqlnd_globals, xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.net_read_timeout",	"31536000",	PHP_INI_SYSTEM, OnUpdateLong,	net_read_timeout,			zend_xmysqlnd_globals, xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.mempool_default_size","16000",   PHP_INI_ALL,	OnUpdateLong,	mempool_default_size,		zend_xmysqlnd_globals, xmysqlnd_globals)
#if PHP_DEBUG
	STD_PHP_INI_ENTRY("xmysqlnd.debug_emalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_emalloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_ecalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_ecalloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_erealloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_erealloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)

	STD_PHP_INI_ENTRY("xmysqlnd.debug_malloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_malloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_calloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_calloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_realloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_realloc_fail_threshold,	zend_xmysqlnd_globals,		xmysqlnd_globals)
#endif
PHP_INI_END()
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(xmysqlnd)
{
	REGISTER_INI_ENTRIES();

	xmysqlnd_library_init();

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(xmysqlnd)
{
	xmysqlnd_library_end();

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */


#if PHP_DEBUG
/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(xmysqlnd)
{
	if (XMYSQLND_G(debug)) {
		struct st_mysqlnd_plugin_trace_log * trace_log_plugin = mysqlnd_plugin_find("debug_trace");
		XMYSQLND_G(dbg) = NULL;
		if (trace_log_plugin) {
			MYSQLND_DEBUG * dbg = trace_log_plugin->methods.trace_instance_init(mysqlnd_debug_std_no_trace_funcs);
			MYSQLND_DEBUG * trace_alloc = trace_log_plugin->methods.trace_instance_init(NULL);
			if (!dbg || !trace_alloc) {
				return FAILURE;
			}
			dbg->m->set_mode(dbg, XMYSQLND_G(debug));
			trace_alloc->m->set_mode(trace_alloc, XMYSQLND_G(trace_alloc_settings));
			XMYSQLND_G(dbg) = dbg;
			XMYSQLND_G(trace_alloc) = trace_alloc;
		}
	}

	return SUCCESS;
}
/* }}} */
#endif


#if PHP_DEBUG
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(xmysqlnd)
{
	MYSQLND_DEBUG * dbg = XMYSQLND_G(dbg);
	MYSQLND_DEBUG * trace_alloc = XMYSQLND_G(trace_alloc);
	DBG_ENTER("RSHUTDOWN");
	if (dbg) {
		dbg->m->close(dbg);
		dbg->m->free_handle(dbg);
		XMYSQLND_G(dbg) = NULL;
	}
	if (trace_alloc) {
		trace_alloc->m->close(trace_alloc);
		trace_alloc->m->free_handle(trace_alloc);
		XMYSQLND_G(trace_alloc) = NULL;
	}
	return SUCCESS;
}
/* }}} */
#endif


/* {{{ xmysqlnd_deps */
static const zend_module_dep xmysqlnd_deps[] = {
	ZEND_MOD_REQUIRED("standard")
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("json")
	ZEND_MOD_END
};
/* }}} */


/* {{{ xmysqlnd_module_entry */
zend_module_entry xmysqlnd_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	xmysqlnd_deps,
	"xmysqlnd",
	NULL, /* xmysqlnd_functions */
	PHP_MINIT(xmysqlnd),
	PHP_MSHUTDOWN(xmysqlnd),
#if PHP_DEBUG
	PHP_RINIT(xmysqlnd),
#else
	NULL,
#endif
#if PHP_DEBUG
	PHP_RSHUTDOWN(xmysqlnd),
#else
	NULL,
#endif
	PHP_MINFO(xmysqlnd),
	PHP_XMYSQLND_VERSION,
	PHP_MODULE_GLOBALS(xmysqlnd),
	PHP_GINIT(xmysqlnd),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/* {{{ COMPILE_DL_XMYSQLND */
#ifdef COMPILE_DL_XMYSQLND
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(xmysqlnd)
#endif
/* }}} */

#endif // MARINES_0

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
