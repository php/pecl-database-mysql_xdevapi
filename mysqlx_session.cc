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
extern "C" {
#include <php.h>
#undef ERROR
}
#include "mysqlx_session.h"

namespace mysqlx {

namespace devapi {

zend_class_entry * mysqlx_session_interface_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schemas, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__create_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__drop_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_session_methods[] */
static const zend_function_entry mysqlx_session_methods[] = {
	PHP_ABSTRACT_ME(mysqlx_session, getSchemas, arginfo_mysqlx_session__get_schemas)
	PHP_ABSTRACT_ME(mysqlx_session, getSchema, arginfo_mysqlx_session__get_schema)

	PHP_ABSTRACT_ME(mysqlx_session, createSchema, arginfo_mysqlx_session__create_schema)
	PHP_ABSTRACT_ME(mysqlx_session, dropSchema, arginfo_mysqlx_session__drop_schema)

	PHP_ABSTRACT_ME(mysqlx_session, close, arginfo_mysqlx_session__close)

	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_register_session_interface */
void
mysqlx_register_session_interface(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Session", mysqlx_session_methods);
	mysqlx_session_interface_entry = zend_register_internal_interface(&tmp_ce);
}
/* }}} */


/* {{{ mysqlx_unregister_session_interface */
void
mysqlx_unregister_session_interface(SHUTDOWN_FUNC_ARGS)
{
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
