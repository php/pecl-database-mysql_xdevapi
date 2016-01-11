/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include <php.h>
#include "mysqlx_session.h"

zend_class_entry * mysqlx_session_interface_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schemas, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_default_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__create_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__drop_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__start_transaction, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__commit, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__rollback, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__wrap_in_transaction, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, transaction_options, Mysqlx\\TransactionOptions, DONT_ALLOW_NULL)
	ZEND_ARG_CALLABLE_INFO(NO_PASS_BY_REF, callback, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__create_transaction_context, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, options, Mysqlx\\TransactionContextOptions, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__push_execution_context, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, context, Mysqlx\\ExecutionContext, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__pop_execution_context, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__execute_batch, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, context, Mysqlx\\BatchContext, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_uri, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_session_methods[] */
static const zend_function_entry mysqlx_session_methods[] = {
	PHP_ABSTRACT_ME(mysqlx_session, getSchemas, arginfo_mysqlx_session__get_schemas)
	PHP_ABSTRACT_ME(mysqlx_session, getSchema, arginfo_mysqlx_session__get_schema)
	PHP_ABSTRACT_ME(mysqlx_session, getDefaultSchema, arginfo_mysqlx_session__get_default_schema)

	PHP_ABSTRACT_ME(mysqlx_session, createSchema, arginfo_mysqlx_session__create_schema)
	PHP_ABSTRACT_ME(mysqlx_session, dropSchema, arginfo_mysqlx_session__drop_schema)


	PHP_ABSTRACT_ME(mysqlx_session, startTransaction, arginfo_mysqlx_session__start_transaction)
	PHP_ABSTRACT_ME(mysqlx_session, commit, arginfo_mysqlx_session__commit)
	PHP_ABSTRACT_ME(mysqlx_session, rollback, arginfo_mysqlx_session__rollback)
	PHP_ABSTRACT_ME(mysqlx_session, wrapInTransaction, arginfo_mysqlx_session__wrap_in_transaction)
	PHP_ABSTRACT_ME(mysqlx_session, createTransactionContext, arginfo_mysqlx_session__create_transaction_context)

	PHP_ABSTRACT_ME(mysqlx_session, pushExecutionContext, arginfo_mysqlx_session__push_execution_context)
	PHP_ABSTRACT_ME(mysqlx_session, popExecutionContext, arginfo_mysqlx_session__pop_execution_context)

	PHP_ABSTRACT_ME(mysqlx_session, executeBatch, arginfo_mysqlx_session__execute_batch)


	PHP_ABSTRACT_ME(mysqlx_session, getUri, arginfo_mysqlx_session__get_uri)
	PHP_ABSTRACT_ME(mysqlx_session, close, arginfo_mysqlx_session__close)

	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_register_session_interface */
void
mysqlx_register_session_interface(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "Session", mysqlx_session_methods);
	mysqlx_session_interface_entry = zend_register_internal_interface(&tmp_ce);
}
/* }}} */


/* {{{ mysqlx_unregister_session_interface */
void
mysqlx_unregister_session_interface(SHUTDOWN_FUNC_ARGS)
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

