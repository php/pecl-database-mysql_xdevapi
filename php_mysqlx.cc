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
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend_smart_str.h>
#include <ext/standard/info.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_priv.h"
#include "php_mysqlx.h"
#include "php_mysqlx_ex.h"
#include "mysqlx_client.h"
#include "mysqlx_expression.h"
#include "mysqlx_session.h"
#include <string>

extern "C" {

PHP_MINFO_FUNCTION(mysql_xdevapi)
{
	UNUSED(zend_module);

	php_info_print_table_start();
	//TODO: we need global const "mysql_xdevapi", it appears in plenty of locations
	php_info_print_table_header(2, "mysql_xdevapi", "enabled");
	php_info_print_table_row(2, "Version", mysqlx::drv::xmysqlnd_get_client_info());

	php_info_print_table_row(2, "Read timeout", std::to_string(MYSQL_XDEVAPI_G(net_read_timeout)).c_str());

	php_info_print_table_row(2, "Collecting statistics", MYSQL_XDEVAPI_G(collect_statistics)? "Yes":"No");
	php_info_print_table_row(2, "Collecting memory statistics", MYSQL_XDEVAPI_G(collect_memory_statistics)? "Yes":"No");

	php_info_print_table_row(2, "Tracing", MYSQL_XDEVAPI_G(debug)? MYSQL_XDEVAPI_G(debug):"n/a");

	php_info_print_table_end();
}

PHP_MYSQL_XDEVAPI_API ZEND_DECLARE_MODULE_GLOBALS(mysql_xdevapi)


static PHP_GINIT_FUNCTION(mysql_xdevapi)
{
	/* ---------------- xmysqlnd / mysqlx ---------------- */
#if defined(COMPILE_DL_MYSQL_XDEVAPI) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	mysql_xdevapi_globals->collect_statistics = TRUE;
	mysql_xdevapi_globals->collect_memory_statistics = FALSE;
	mysql_xdevapi_globals->debug = nullptr;	/* The actual string */
	mysql_xdevapi_globals->dbg = nullptr;	/* The DBG object*/
	mysql_xdevapi_globals->trace_alloc_settings = nullptr;
	mysql_xdevapi_globals->trace_alloc = nullptr;
	mysql_xdevapi_globals->net_read_timeout = 31536000;
	mysql_xdevapi_globals->mempool_default_size = 16000;
	mysql_xdevapi_globals->debug_emalloc_fail_threshold = -1;
	mysql_xdevapi_globals->debug_ecalloc_fail_threshold = -1;
	mysql_xdevapi_globals->debug_erealloc_fail_threshold = -1;
	mysql_xdevapi_globals->debug_malloc_fail_threshold = -1;
	mysql_xdevapi_globals->debug_calloc_fail_threshold = -1;
	mysql_xdevapi_globals->debug_realloc_fail_threshold = -1;
}

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("xmysqlnd.collect_statistics",	"1", 	PHP_INI_ALL,	OnUpdateBool,	collect_statistics, 		zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
	STD_PHP_INI_BOOLEAN("xmysqlnd.collect_memory_statistics","0",PHP_INI_SYSTEM,OnUpdateBool,	collect_memory_statistics,	zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug",					nullptr, 	PHP_INI_SYSTEM, OnUpdateString,	debug,						zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.trace_alloc",			nullptr, 	PHP_INI_SYSTEM, OnUpdateString,	trace_alloc_settings,		zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.net_read_timeout",	"31536000",	PHP_INI_SYSTEM, OnUpdateLong,	net_read_timeout,			zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.mempool_default_size","16000",   PHP_INI_ALL,	OnUpdateLong,	mempool_default_size,		zend_mysql_xdevapi_globals, mysql_xdevapi_globals)
#if PHP_DEBUG
	STD_PHP_INI_ENTRY("xmysqlnd.debug_emalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_emalloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_ecalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_ecalloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_erealloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_erealloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)

	STD_PHP_INI_ENTRY("xmysqlnd.debug_malloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_malloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_calloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_calloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)
	STD_PHP_INI_ENTRY("xmysqlnd.debug_realloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_realloc_fail_threshold,	zend_mysql_xdevapi_globals,		mysql_xdevapi_globals)
#endif
PHP_INI_END()

static PHP_MINIT_FUNCTION(mysql_xdevapi)
{
	/* ---------------- xmysqlnd ---------------- */
	REGISTER_INI_ENTRIES();

	mysqlx::drv::xmysqlnd_library_init();

	/* ---------------- mysqlx ---------------- */
	mysqlx::devapi::mysqlx_minit_classes(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(mysql_xdevapi)
{
	/* ---------------- mysqlx ---------------- */
	mysqlx::devapi::client::release_all_clients();
	mysqlx::devapi::mysqlx_mshutdown_classes(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	/* ---------------- xmysqlnd ---------------- */
	mysqlx::drv::xmysqlnd_library_end();

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

#if PHP_DEBUG
static PHP_RINIT_FUNCTION(mysql_xdevapi)
{
	UNUSED_FUNC_ARGS();
	/* ---------------- xmysqlnd ---------------- */
	if (MYSQL_XDEVAPI_G(debug)) {
		st_mysqlnd_plugin_trace_log * trace_log_plugin = static_cast<st_mysqlnd_plugin_trace_log*>(mysqlnd_plugin_find("debug_trace"));
		MYSQL_XDEVAPI_G(dbg) = nullptr;
		if (trace_log_plugin) {
			MYSQLND_DEBUG * dbg = trace_log_plugin->methods.trace_instance_init(mysqlnd_debug_std_no_trace_funcs);
			MYSQLND_DEBUG * trace_alloc = trace_log_plugin->methods.trace_instance_init(nullptr);
			if (!dbg || !trace_alloc) {
				return FAILURE;
			}
			dbg->m->set_mode(dbg, MYSQL_XDEVAPI_G(debug));
			trace_alloc->m->set_mode(trace_alloc, MYSQL_XDEVAPI_G(trace_alloc_settings));
			MYSQL_XDEVAPI_G(dbg) = dbg;
			MYSQL_XDEVAPI_G(trace_alloc) = trace_alloc;
		}
	}

	return SUCCESS;
}

#endif


#if PHP_DEBUG
static PHP_RSHUTDOWN_FUNCTION(mysql_xdevapi)
{
	UNUSED_FUNC_ARGS();
	/* ---------------- xmysqlnd ---------------- */
	MYSQLND_DEBUG * dbg = MYSQL_XDEVAPI_G(dbg);
	MYSQLND_DEBUG * trace_alloc = MYSQL_XDEVAPI_G(trace_alloc);
	DBG_ENTER("RSHUTDOWN");
	if (dbg) {
		dbg->m->close(dbg);
		dbg->m->free_handle(dbg);
		MYSQL_XDEVAPI_G(dbg) = nullptr;
	}
	if (trace_alloc) {
		trace_alloc->m->close(trace_alloc);
		trace_alloc->m->free_handle(trace_alloc);
		MYSQL_XDEVAPI_G(trace_alloc) = nullptr;
	}
	mysqlx::devapi::client::prune_expired_connections();
	return SUCCESS;
}

#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_xdevapi__get_session, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, uri, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_xdevapi__get_client, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, uri, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, client_options, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_xdevapi__expression, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, expression, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*
  We need a proper macro, that is included in all mysqlx_ files which register classes by using INIT_NS_CLASS_ENTRY.
  For now we use in these files const string "Mysqlx".
*/
#define MYSQL_XDEVAPI_NAMESPACE "mysql_xdevapi"

static const zend_function_entry mysqlx_functions[] = {
	ZEND_NS_NAMED_FE(MYSQL_XDEVAPI_NAMESPACE, getSession, mysqlx::devapi::ZEND_FN(mysql_xdevapi_getSession), arginfo_mysql_xdevapi__get_session)
	ZEND_NS_NAMED_FE(MYSQL_XDEVAPI_NAMESPACE, getClient, mysqlx::devapi::ZEND_FN(mysql_xdevapi_getClient), arginfo_mysql_xdevapi__get_client)
	ZEND_NS_NAMED_FE(MYSQL_XDEVAPI_NAMESPACE, expression, mysqlx::devapi::ZEND_FN(mysql_xdevapi__expression), arginfo_mysql_xdevapi__expression)
	PHP_FE_END
};

static const zend_module_dep mysqlx_deps[] = {
	ZEND_MOD_REQUIRED("standard")
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("hash")
	ZEND_MOD_REQUIRED("json")
	ZEND_MOD_END
};

zend_module_entry mysql_xdevapi_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	nullptr,
	mysqlx_deps,
	"mysql_xdevapi",
	mysqlx_functions, /* mysqlx_functions */ /* when mysqlx and mysqlx get split this will be nullptr */
	PHP_MINIT(mysql_xdevapi),
	PHP_MSHUTDOWN(mysql_xdevapi),
#if PHP_DEBUG
	PHP_RINIT(mysql_xdevapi),
#else
	nullptr,
#endif
#if PHP_DEBUG
	PHP_RSHUTDOWN(mysql_xdevapi),
#else
	nullptr,
#endif
	PHP_MINFO(mysql_xdevapi),
	PHP_MYSQL_XDEVAPI_VERSION,
	PHP_MODULE_GLOBALS(mysql_xdevapi),
	PHP_GINIT(mysql_xdevapi),
	nullptr,
	nullptr,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_MYSQL_XDEVAPI
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(mysql_xdevapi)
#endif

} // extern "C"
