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
#include "mysqlx_database_object.h"

zend_class_entry * mysqlx_database_object_interface_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__drop, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_database_object_methods[] */
static const zend_function_entry mysqlx_database_object_methods[] = {
	PHP_ABSTRACT_ME(mysqlx_database_object, getSession, arginfo_mysqlx_database_object__get_session)

	PHP_ABSTRACT_ME(mysqlx_database_object, getName, arginfo_mysqlx_database_object__get_name)

	PHP_ABSTRACT_ME(mysqlx_database_object, existsInDatabase, arginfo_mysqlx_database_object__exists_in_database)

	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_register_database_object_interface */
void
mysqlx_register_database_object_interface(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "DatabaseObject", mysqlx_database_object_methods);
	mysqlx_database_object_interface_entry = zend_register_internal_interface(&tmp_ce);
}
/* }}} */


/* {{{ mysqlx_unregister_database_object_interface */
void
mysqlx_unregister_database_object_interface(SHUTDOWN_FUNC_ARGS)
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

