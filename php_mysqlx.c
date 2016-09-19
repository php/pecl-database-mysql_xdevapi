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
#include "php_mysqlx.h"

#include "ext/standard/info.h"
#include "zend_smart_str.h"


PHPAPI int mysqlx_minit_classes(INIT_FUNC_ARGS);
PHPAPI int mysqlx_mshutdown_classes(SHUTDOWN_FUNC_ARGS);


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mysqlx)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlx", "enabled");
	php_info_print_table_row(2, "message classes",
#ifdef MYSQLX_MESSAGE_CLASSES
								"enabled");
#else
								"disabled");
#endif
	php_info_print_table_row(2, "experimental features",
#ifdef MYSQLX_EXPERIMENTAL_FEATURES
								"enabled");
#else
								"disabled");
#endif
	php_info_print_table_end();
}
/* }}} */


PHPAPI ZEND_DECLARE_MODULE_GLOBALS(mysqlx)


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mysqlx)
{
#if defined(COMPILE_DL_MYSQLX) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
}
/* }}} */


/* {{{ PHP_INI_BEGIN
*/
PHP_INI_BEGIN()
PHP_INI_END()
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(mysqlx)
{
	REGISTER_INI_ENTRIES();

	mysqlx_minit_classes(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(mysqlx)
{
	mysqlx_mshutdown_classes(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */


#if PHP_DEBUG
/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(mysqlx)
{
	return SUCCESS;
}
/* }}} */
#endif


#if PHP_DEBUG
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(mysqlx)
{
	return SUCCESS;
}
/* }}} */
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx__get_node_session, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_TYPE_INFO(0, hostname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, username, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx__expression, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, expression, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(mysqlx__getNodeSession);
PHP_FUNCTION(mysqlx__expression);

/*
  We need a proper macro, that is included in all mysqlx_ files which register classes by using INIT_NS_CLASS_ENTRY.
  For now we use in these files const string "Mysqlx".
*/
#define MYSQLX_NAMESPACE "Mysqlx"

/* {{{ mysqlx_functions */
static const zend_function_entry mysqlx_functions[] = {
	ZEND_NS_NAMED_FE(MYSQLX_NAMESPACE, getNodeSession, ZEND_FN(mysqlx__getNodeSession), arginfo_mysqlx__get_node_session)
	ZEND_NS_NAMED_FE(MYSQLX_NAMESPACE, expression, ZEND_FN(mysqlx__expression), arginfo_mysqlx__expression)
	PHP_FE_END
};
/* }}} */


/* {{{ mysqlx_deps */
static const zend_module_dep mysqlx_deps[] = {
	ZEND_MOD_REQUIRED("standard")
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("xmysqlnd")
	ZEND_MOD_END
};
/* }}} */


/* {{{ mysqlx_module_entry */
zend_module_entry mysqlx_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlx_deps,
	"mysqlx",
	mysqlx_functions, /* mysqlx_functions */ /* when mysqlx and mysqlx get split this will be NULL */
	PHP_MINIT(mysqlx),
	PHP_MSHUTDOWN(mysqlx),
#if PHP_DEBUG
	PHP_RINIT(mysqlx),
#else
	NULL,
#endif
#if PHP_DEBUG
	PHP_RSHUTDOWN(mysqlx),
#else
	NULL,
#endif
	PHP_MINFO(mysqlx),
	MYSQLX_VERSION,
	PHP_MODULE_GLOBALS(mysqlx),
	PHP_GINIT(mysqlx),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/* {{{ COMPILE_DL_MYSQLX */
#ifdef COMPILE_DL_MYSQLX
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(mysqlx)
#endif
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
