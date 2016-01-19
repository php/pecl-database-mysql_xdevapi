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
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_sql_statement.h"

#include "mysqlx_session.h"

static zend_class_entry *mysqlx_node_session_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__query, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, query, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__create_statement, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, query, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__quote_name, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_server_version, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_client_id, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_schemas, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_default_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__create_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__drop_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__start_transaction, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__commit, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__rollback, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__wrap_in_transaction, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, transaction_options, Mysqlx\\TransactionOptions, DONT_ALLOW_NULL)
	ZEND_ARG_CALLABLE_INFO(NO_PASS_BY_REF, callback, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__create_transaction_context, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, options, Mysqlx\\TransactionContextOptions, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__push_execution_context, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, context, Mysqlx\\ExecutionContext, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__pop_execution_context, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__execute_batch, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_OBJ_INFO(NO_PASS_BY_REF, context, Mysqlx\\BatchContext, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__get_uri, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()



struct st_mysqlx_node_session
{
	XMYSQLND_NODE_SESSION * session;
};


#define MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_session *) mysqlx_object->ptr; \
	if (!(_to) && !(_to)->session) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ proto mixed mysqlx_node_session::query(object session, string query) */
static
PHP_METHOD(mysqlx_node_session, query)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::query");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;

	{
		MYSQLND_STATS * stats = session->data->stats;
		MYSQLND_ERROR_INFO * error_info = session->data->error_info;
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_IMPLICIT);
		if (stmt) {
			if (PASS == stmt->data->m.send_query(stmt, stats, error_info)) {
				zend_bool has_more = FALSE;
				do {
					XMYSQLND_NODE_STMT_RESULT * result = stmt->data->m.get_buffered_result(stmt, &has_more, stats, error_info);
					xmysqlnd_node_stmt_result_free(result, stats, error_info);
					DBG_INF_FMT("has_more=%s", has_more? "TRUE":"FALSE");
				} while (has_more == TRUE);
			}
			xmysqlnd_node_stmt_free(stmt, stats, error_info);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::query_and_discard(object session, string query) */
static
PHP_METHOD(mysqlx_node_session, query_and_discard)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::query_and_discard");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;
	{
		MYSQLND_STATS * stats = session->data->stats;
		MYSQLND_ERROR_INFO * error_info = session->data->error_info;
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_IMPLICIT);
		if (stmt) {
			if (PASS == stmt->data->m.send_query(stmt, stats, error_info)) {
				stmt->data->m.skip_all_results(stmt, stats, error_info);
			}
			xmysqlnd_node_stmt_free(stmt, stats, error_info);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::createStatement(object session, string query) */
static
PHP_METHOD(mysqlx_node_session, createStatement)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::createStatement");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;

	{
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_EXPLICIT);
		if (stmt) {
			mysqlx_new_sql_stmt(return_value, stmt);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::quoteName(object session, string query) */
static
PHP_METHOD(mysqlx_node_session, quoteName)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING name = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::quoteName");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(name.s), &(name.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;

	{
		MYSQLND_STRING quoted_name = session->data->m->quote_name(session->data, name);
		RETVAL_STRINGL(quoted_name.s, quoted_name.l);
		if (quoted_name.s) {
			mnd_efree(quoted_name.s);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::getServerVersion(object session) */
static
PHP_METHOD(mysqlx_node_session, getServerVersion)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getServerVersion");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;
	if (session) {
		RETVAL_LONG(session->data->m->get_server_version(session->data));
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::getClientId(object session) */
static
PHP_METHOD(mysqlx_node_session, getClientId)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getClientId");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;
	if (session) {
		RETVAL_LONG(session->data->m->get_client_id(session->data));
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::__construct */
static
PHP_METHOD(mysqlx_node_session, __construct)
{
}
/* }}} */


/* {{{ mysqlx_node_session::getSchemas */
static
PHP_METHOD(mysqlx_node_session, getSchemas)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getSchemas");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::getSchema */
static
PHP_METHOD(mysqlx_node_session, getSchema)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::getDefaultSchema */
static
PHP_METHOD(mysqlx_node_session, getDefaultSchema)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getDefaultSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::createSchema */
static
PHP_METHOD(mysqlx_node_session, createSchema)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::createSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::dropSchema */
static
PHP_METHOD(mysqlx_node_session, dropSchema)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::dropSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::startTransaction */
static
PHP_METHOD(mysqlx_node_session, startTransaction)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::startTransaction");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::commit */
static
PHP_METHOD(mysqlx_node_session, commit)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::commit");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::rollback */
static
PHP_METHOD(mysqlx_node_session, rollback)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::rollback");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::wrapInTransaction */
static
PHP_METHOD(mysqlx_node_session, wrapInTransaction)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::wrapInTransaction");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::createTransactionContext */
static
PHP_METHOD(mysqlx_node_session, createTransactionContext)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::createTransactionContext");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::pushExecutionContext */
static
PHP_METHOD(mysqlx_node_session, pushExecutionContext)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::pushExecutionContext");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::popExecutionContext */
static
PHP_METHOD(mysqlx_node_session, popExecutionContext)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::popExecutionContext");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::executeBatch */
static
PHP_METHOD(mysqlx_node_session, executeBatch)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::executeBatch");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::getUri */
static
PHP_METHOD(mysqlx_node_session, getUri)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::getUri");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::close */
static
PHP_METHOD(mysqlx_node_session, close)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_node_session::close");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {
		session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
		object->session = NULL;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session_methods[] */
static const zend_function_entry mysqlx_node_session_methods[] = {
	PHP_ME(mysqlx_node_session, __construct, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_session, query,					arginfo_mysqlx_node_session__query, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, query_and_discard,		arginfo_mysqlx_node_session__query, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, createStatement,		arginfo_mysqlx_node_session__create_statement, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, quoteName,				arginfo_mysqlx_node_session__quote_name, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, getServerVersion,		arginfo_mysqlx_node_session__get_server_version, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, getClientId,			arginfo_mysqlx_node_session__get_client_id, ZEND_ACC_PUBLIC)
#if 1
	PHP_ME(mysqlx_node_session, getSchemas,				arginfo_mysqlx_node_session__get_schemas, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, getSchema,				arginfo_mysqlx_node_session__get_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, getDefaultSchema,		arginfo_mysqlx_node_session__get_default_schema, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_session, createSchema,			arginfo_mysqlx_node_session__create_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, dropSchema,				arginfo_mysqlx_node_session__drop_schema, ZEND_ACC_PUBLIC)


	PHP_ME(mysqlx_node_session, startTransaction,		arginfo_mysqlx_node_session__start_transaction, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, commit,					arginfo_mysqlx_node_session__commit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, rollback,				arginfo_mysqlx_node_session__rollback, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, wrapInTransaction,		arginfo_mysqlx_node_session__wrap_in_transaction, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, createTransactionContext, arginfo_mysqlx_node_session__create_transaction_context, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_session, pushExecutionContext,	arginfo_mysqlx_node_session__push_execution_context, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, popExecutionContext,	arginfo_mysqlx_node_session__pop_execution_context, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_session, executeBatch,			arginfo_mysqlx_node_session__execute_batch, ZEND_ACC_PUBLIC)


	PHP_ME(mysqlx_node_session, getUri,					arginfo_mysqlx_node_session__get_uri, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, close,					arginfo_mysqlx_node_session__close, ZEND_ACC_PUBLIC)
#endif
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_session_handlers;
static HashTable mysqlx_node_session_properties;

const struct st_mysqlx_property_entry mysqlx_node_session_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_session_free_storage */
static void
mysqlx_node_session_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_session * inner_obj = (struct st_mysqlx_node_session *) mysqlx_object->ptr;

	if (inner_obj) {
		XMYSQLND_NODE_SESSION * session = (XMYSQLND_NODE_SESSION *) inner_obj->session;

		if (session) {
			session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_session_object_allocator */
static zend_object *
php_mysqlx_node_session_object_allocator(zend_class_entry * class_type)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory = MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_session * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_session));
	MYSQLND_STATS * stats = NULL;
	MYSQLND_ERROR_INFO * error_info = NULL;

	DBG_ENTER("php_mysqlx_node_session_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	if (!(object->session = xmysqlnd_node_session_create(0, FALSE, factory, stats, error_info))) {
		mnd_efree(object);
		mnd_efree(mysqlx_object);
		DBG_RETURN(NULL);
	}

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_session_handlers;
	mysqlx_object->properties = &mysqlx_node_session_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_session_class */
void
mysqlx_register_node_session_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_session_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_session_handlers.free_obj = mysqlx_node_session_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSession", mysqlx_node_session_methods);
		tmp_ce.create_object = php_mysqlx_node_session_object_allocator;
		mysqlx_node_session_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_session_class_entry, 1, mysqlx_session_interface_entry);
	}

	zend_hash_init(&mysqlx_node_session_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_session_properties, mysqlx_node_session_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_session_class */
void
mysqlx_unregister_node_session_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_session_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_session */
enum_func_status
mysqlx_new_node_session(zval * return_value)
{
	DBG_ENTER("mysqlx_new_node_session");
	DBG_RETURN(SUCCESS == object_init_ex(return_value, mysqlx_node_session_class_entry)? PASS:FAIL);
}
/* }}} */


/* {{{ proto bool mysqlx\\getNodeSession(string hostname, string username, string password, int port)
   Bind variables to a prepared statement as parameters */
PHP_FUNCTION(mysqlx__getNodeSession)
{
	MYSQLND_CSTRING hostname = {NULL, 0};
	MYSQLND_CSTRING username = {NULL, 0};
	MYSQLND_CSTRING password = {NULL, 0};
	MYSQLND_CSTRING empty = {NULL, 0};
	zend_long port = 0;
	size_t set_capabilities = 0;
	size_t client_api_flags = 0;

	DBG_ENTER("mysqlx__getNodeSession");
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "sss|l",
										 &(hostname.s), &(hostname.l), 
										 &(username.s), &(username.l),
										 &(password.s), &(password.l),
										 &port))
	{
		DBG_VOID_RETURN;
	}

	if (!hostname.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	if (!port) {
		port = 33060;
	}

	RETVAL_FALSE;
	if (PASS == mysqlx_new_node_session(return_value)) {
		struct st_mysqlx_node_session * object;
		XMYSQLND_NODE_SESSION * new_session;
		MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, return_value);

		new_session = xmysqlnd_node_session_connect(object->session, hostname, username, password,
													empty /*db*/, empty /*s_or_p*/, port, set_capabilities, client_api_flags);

		if (object->session != new_session) {
			php_error_docref(NULL, E_WARNING, "Different object returned");
			if (object->session) {
				object->session->m->close(object->session, XMYSQLND_CLOSE_IMPLICIT);
			}
			object->session = new_session;
		}
	}
	DBG_VOID_RETURN;
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

