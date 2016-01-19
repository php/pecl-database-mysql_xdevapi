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
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <xmysqlnd/xmysqlnd.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#ifdef HAVE_SPL
#include "ext/spl/spl_exceptions.h" /* spl_ce_RuntimeException */
#endif


static zend_class_entry *mysqlx_exception_class_entry;


/* {{{ mysqlx_exception_methods */
const zend_function_entry mysqlx_exception_methods[] = {
	{NULL, NULL, NULL}
};


/* {{{ mysqlx_register_exception_class */
void
mysqlx_register_exception_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;

	INIT_CLASS_ENTRY(tmp_ce, "mysqlx_exception", mysqlx_exception_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "mysqlx_exception", mysqlx_exception_methods);
#ifdef HAVE_SPL
	mysqlx_exception_class_entry = zend_register_internal_class_ex(&tmp_ce, spl_ce_RuntimeException);
#else
	mysqlx_exception_class_entry = zend_register_internal_class_ex(&tmp_ce, zend_ce_exception);
#endif
	mysqlx_exception_class_entry->ce_flags |= ZEND_ACC_FINAL; /* Forbid extension of the extension */
	zend_declare_property_long(mysqlx_exception_class_entry, "code", sizeof("code")-1, 0, ZEND_ACC_PROTECTED);
	zend_declare_property_string(mysqlx_exception_class_entry, "sqlstate", sizeof("sqlstate")-1, "00000", ZEND_ACC_PROTECTED);
}
/* }}} */


/* {{{ mysqlx_unregister_exception_class */
void
mysqlx_unregister_exception_class(SHUTDOWN_FUNC_ARGS)
{
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

