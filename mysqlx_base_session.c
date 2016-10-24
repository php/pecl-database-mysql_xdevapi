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
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result_meta.h>
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_base_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_sql_statement.h"

#include "mysqlx_session.h"

//static zend_class_entry *mysqlx_base_session_class_entry;
zend_class_entry *mysqlx_base_session_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__create_statement, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, query, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__get_server_version, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__get_client_id, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__generate_uuid, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__get_schemas, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__create_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__drop_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__drop_collection, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, schema_name, IS_STRING, DONT_ALLOW_NULL)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, collection_name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__drop_table, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, schema_name, IS_STRING, DONT_ALLOW_NULL)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, table_name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__list_clients, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__kill_client, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, client_id, IS_LONG, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_session__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()



#define MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_session *) mysqlx_object->ptr; \
	if (!(_to) && !(_to)->session) { \
		if ((_to)->closed) { \
			php_error_docref(NULL, E_WARNING, "closed session"); \
		} else { \
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		} \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_throw_exception_from_session_if_needed */
static zend_bool
mysqlx_throw_exception_from_session_if_needed(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	const unsigned int error_num = session->m->get_error_no(session);
	DBG_ENTER("mysqlx_throw_exception_from_session_if_needed");
	if (error_num) {
		MYSQLND_CSTRING sqlstate = { session->m->get_sqlstate(session) , 0 };
		MYSQLND_CSTRING errmsg = { session->m->get_error_str(session) , 0 };
		sqlstate.l = strlen(sqlstate.s);
		errmsg.l = strlen(errmsg.s);
		mysqlx_new_exception(error_num, sqlstate, errmsg);
		DBG_RETURN(TRUE);
	}
	DBG_RETURN(FALSE);
}
/* }}} */


/* {{{ proto mixed mysqlx_base_session::createStatement(string query) */
static
PHP_METHOD(mysqlx_base_session, createStatement)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::createStatement");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(query.s), &(query.l)) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		XMYSQLND_NODE_STMT * const stmt = session->m->create_statement_object(session);
		if (stmt) {
			mysqlx_new_sql_stmt(return_value, stmt, namespace_sql, query);
			if (Z_TYPE_P(return_value) == IS_NULL) {
				xmysqlnd_node_stmt_free(stmt, NULL, NULL);
				mysqlx_throw_exception_from_session_if_needed(session->data);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_execute_base_session_query */
static void
mysqlx_execute_base_session_query(XMYSQLND_NODE_SESSION * const session,
								  const MYSQLND_CSTRING namespace_,
								  const MYSQLND_CSTRING query,
								  const zend_long flags,
								  zval * const return_value,
								  const unsigned int argc,
								  const zval * args)
{
	XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
	DBG_ENTER("mysqlx_execute_base_session_query");

	if (stmt) {
		zval stmt_zv;
		ZVAL_UNDEF(&stmt_zv);
		mysqlx_new_sql_stmt(&stmt_zv, stmt, namespace_, query);
		if (Z_TYPE(stmt_zv) == IS_NULL) {
			xmysqlnd_node_stmt_free(stmt, NULL, NULL);		
		}
		if (Z_TYPE(stmt_zv) == IS_OBJECT) {
			zval zv;
			unsigned int i = 0;
			ZVAL_UNDEF(&zv);

			for (; i < argc; ++i) {
				ZVAL_UNDEF(&zv);
				mysqlx_node_sql_statement_bind_one_param(&stmt_zv, &args[i], i, &zv);
				if (Z_TYPE(zv) == IS_FALSE) {
					goto end;
				}
				zval_dtor(&zv);
			}
			ZVAL_UNDEF(&zv);

			mysqlx_node_sql_statement_execute(Z_MYSQLX_P(&stmt_zv), flags, &zv);

			ZVAL_COPY(return_value, &zv);
		}
end:
		zval_ptr_dtor(&stmt_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_base_session::quoteName(string query) */
static
PHP_METHOD(mysqlx_base_session, quoteName)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::quoteName");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(name.s), &(name.l)) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		MYSQLND_STRING quoted_name = session->data->m->quote_name(session->data, name);
		RETVAL_STRINGL(quoted_name.s, quoted_name.l);
		if (quoted_name.s) {
			mnd_efree(quoted_name.s);
		}
		mysqlx_throw_exception_from_session_if_needed(session->data);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_base_session::getServerVersion() */
static
PHP_METHOD(mysqlx_base_session, getServerVersion)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::getServerVersion");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		RETVAL_LONG(session->m->get_server_version(session));
		mysqlx_throw_exception_from_session_if_needed(session->data);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_base_session::getClientId() */
static
PHP_METHOD(mysqlx_base_session, getClientId)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::getClientId");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		RETVAL_LONG(session->data->m->get_client_id(session->data));
		mysqlx_throw_exception_from_session_if_needed(session->data);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_base_session::generateUUID() */
static
PHP_METHOD(mysqlx_base_session, generateUUID)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::generateUUID");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if ((session = object->session)) {
		const MYSQLND_CSTRING unique_id = session->m->get_uuid(session);
		if (unique_id.s) {
			RETVAL_STRINGL(unique_id.s, unique_id.l);
		}
		mysqlx_throw_exception_from_session_if_needed(session->data);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::__construct */
static
PHP_METHOD(mysqlx_base_session, __construct)
{
}
/* }}} */



struct st_mysqlx_get_schemas_ctx
{
	zval * list;
};


/* {{{ get_schemas_handler_on_row */
static const enum_hnd_func_status
get_schemas_handler_on_row(void * context,
						   XMYSQLND_NODE_SESSION * const session,
						   XMYSQLND_NODE_STMT * const stmt,
						   const XMYSQLND_NODE_STMT_RESULT_META * const meta,
						   const zval * const row,
						   MYSQLND_STATS * const stats,
						   MYSQLND_ERROR_INFO * const error_info)
{
	const struct st_mysqlx_get_schemas_ctx * ctx = (const struct st_mysqlx_get_schemas_ctx *) context;
	DBG_ENTER("get_schemas_handler_on_row");
	if (ctx && ctx->list && row) {
		if (Z_TYPE_P(ctx->list) != IS_ARRAY) {
			array_init(ctx->list);
		}
		if (Z_TYPE_P(ctx->list) == IS_ARRAY) {
			const MYSQLND_CSTRING schema_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
			XMYSQLND_NODE_SCHEMA * schema = session->m->create_schema_object(session, schema_name);
			if (schema) {
				zval zv;
				ZVAL_UNDEF(&zv);
				mysqlx_new_node_schema(&zv, schema);
				zend_hash_next_index_insert(Z_ARRVAL_P(ctx->list), &zv);
			}
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ mysqlx_base_session_command_handler_on_error */
static const enum_hnd_func_status
mysqlx_base_session_command_handler_on_error(void * context,
											 XMYSQLND_NODE_SESSION * const session,
											 XMYSQLND_NODE_STMT * const stmt,
											 const unsigned int code,
											 const MYSQLND_CSTRING sql_state,
											 const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_base_session_command_handler_on_error");
	if (session) {
		session->data->m->handler_on_error(session->data, code, sql_state, message);
	}
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ mysqlx_base_session::getSchemas() */
static
PHP_METHOD(mysqlx_base_session, getSchemas)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::getSchemas");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ((session = object->session)) {
		const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { NULL, NULL };
		const MYSQLND_CSTRING list_query = { "SHOW DATABASES", sizeof("SHOW DATABASES") - 1 };
		zval list;
		struct st_mysqlx_get_schemas_ctx ctx = { &list };
		const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_row_bind on_row = { get_schemas_handler_on_row, &ctx };
		const struct st_xmysqlnd_node_session_on_warning_bind on_warning = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_base_session_command_handler_on_error, NULL };
		const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok = { NULL, NULL };

		ZVAL_UNDEF(&list);

		if (PASS == session->m->query_cb(session, namespace_sql, list_query, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::getSchema(string name) */
static
PHP_METHOD(mysqlx_base_session, getSchema)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING schema_name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::getSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(schema_name.s), &(schema_name.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {
		XMYSQLND_NODE_SCHEMA * schema = session->m->create_schema_object(session, schema_name);
		if (schema) {
			mysqlx_new_node_schema(return_value, schema);
		} else {
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::createSchema(string name) */
static
PHP_METHOD(mysqlx_base_session, createSchema)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING schema_name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::createSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(schema_name.s), &(schema_name.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {
		XMYSQLND_NODE_SCHEMA * schema = NULL;
		if (PASS == session->m->create_db(session, schema_name) &&
			(schema = session->m->create_schema_object(session, schema_name)))
		{
			DBG_INF_FMT("schema=%p", schema);
			mysqlx_new_node_schema(return_value, schema);
		} else {
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::dropSchema(string name) */
static
PHP_METHOD(mysqlx_base_session, dropSchema)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	MYSQLND_CSTRING schema_name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::dropSchema");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(schema_name.s), &(schema_name.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	RETVAL_BOOL(object->session && schema_name.s && schema_name.l && PASS == object->session->m->drop_db(object->session, schema_name));
	mysqlx_throw_exception_from_session_if_needed(object->session->data);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::dropCollection(string schema_name, string collection_name) */
static
PHP_METHOD(mysqlx_base_session, dropCollection)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING schema_name = {NULL, 0};
	MYSQLND_CSTRING collection_name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::dropCollection");
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oss",
		&object_zv, mysqlx_base_session_class_entry,
		&(schema_name.s), &(schema_name.l),
		&(collection_name.s), &(collection_name.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	session = object->session;
	if (session && schema_name.s && schema_name.l && collection_name.s && collection_name.l) {

		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_base_session_command_handler_on_error, NULL };

		RETVAL_BOOL(PASS == session->m->drop_collection(session, schema_name, collection_name, on_error));
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::dropTable(string schema_name, string table_name) */
static
PHP_METHOD(mysqlx_base_session, dropTable)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING schema_name = {NULL, 0};
	MYSQLND_CSTRING table_name = {NULL, 0};

	DBG_ENTER("mysqlx_base_session::dropTable");
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oss",
		&object_zv, mysqlx_base_session_class_entry,
		&(schema_name.s), &(schema_name.l),
		&(table_name.s), &(table_name.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	session = object->session;
	if (session && schema_name.s && schema_name.l && table_name.s && table_name.l) {

		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_base_session_command_handler_on_error, NULL };

		RETVAL_BOOL(PASS == session->m->drop_table(session, schema_name, table_name, on_error));
	}

	DBG_VOID_RETURN;
}
/* }}} */


struct st_mysqlx_list_clients__ctx
{
	zval * list;
};

/* {{{ list_clients__handler_on_row */
static const enum_hnd_func_status
list_clients__handler_on_row(void * context,
							 XMYSQLND_NODE_SESSION * const session,
							 XMYSQLND_NODE_STMT * const stmt,
							 const XMYSQLND_NODE_STMT_RESULT_META * const meta,
							 const zval * const row,
							 MYSQLND_STATS * const stats,
							 MYSQLND_ERROR_INFO * const error_info)
{
	const struct st_mysqlx_list_clients__ctx * ctx = (const struct st_mysqlx_list_clients__ctx *) context;
	DBG_ENTER("list_clients__handler_on_row");
	if (ctx && ctx->list && row) {
		if (Z_TYPE_P(ctx->list) != IS_ARRAY) {
			array_init(ctx->list);
		}
		if (Z_TYPE_P(ctx->list) == IS_ARRAY) {
			const unsigned int field_count = meta->m->get_field_count(meta);
			unsigned int i;
			zval zv;
			ZVAL_UNDEF(&zv);
			array_init_size(&zv, field_count);

			for (i = 0; i < field_count; ++i) {
				const XMYSQLND_RESULT_FIELD_META * field_meta = meta->m->get_field(meta, i);
				if (field_meta) {
					zend_hash_add(Z_ARRVAL(zv), field_meta->zend_hash_key.sname, (zval *)&(row[i]));
				}
			}
			zend_hash_next_index_insert(Z_ARRVAL_P(ctx->list), &zv);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ mysqlx_base_session::listClients() */
static
PHP_METHOD(mysqlx_base_session, listClients)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::listClients");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ((session = object->session)) {
		const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { NULL, NULL };
		const MYSQLND_CSTRING list_query = { "list_clients", sizeof("list_clients") - 1 };
		zval list;
		struct st_mysqlx_list_clients__ctx ctx = { &list };
		const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_row_bind on_row = { list_clients__handler_on_row, &ctx };
		const struct st_xmysqlnd_node_session_on_warning_bind on_warning = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_base_session_command_handler_on_error, NULL };
		const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok = { NULL, NULL };

		ZVAL_UNDEF(&list);

		if (PASS == session->m->query_cb(session, namespace_xplugin, list_query, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::killClient() */
static
PHP_METHOD(mysqlx_base_session, killClient)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	zend_long client_id;

	DBG_ENTER("mysqlx_base_session::killClient");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol", &object_zv, mysqlx_base_session_class_entry,
																	   &client_id) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	if (object->session) {
		const MYSQLND_CSTRING query = { "kill_client", sizeof("kill_client") - 1 };
		zval c_id;
		ZVAL_LONG(&c_id, client_id);
		mysqlx_execute_base_session_query(object->session, namespace_xplugin, query, MYSQLX_EXECUTE_FLAG_BUFFERED, return_value, 1, &c_id);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session::close */
static
PHP_METHOD(mysqlx_base_session, close)
{
	zval * object_zv;
	struct st_mysqlx_session * object;
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("mysqlx_base_session::close");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_base_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_BASE_SESSION_FROM_ZVAL(object, object_zv);
	if ((session = object->session)) {
		session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
		object->session = NULL;
		object->closed = TRUE;
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_session_methods[] */
static const zend_function_entry mysqlx_base_session_methods[] = {
	PHP_ME(mysqlx_base_session, __construct, 		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_base_session, createStatement,	arginfo_mysqlx_base_session__create_statement, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, getServerVersion,	arginfo_mysqlx_base_session__get_server_version, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, getClientId,		arginfo_mysqlx_base_session__get_client_id, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, generateUUID,		arginfo_mysqlx_base_session__generate_uuid, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, getSchemas,			arginfo_mysqlx_base_session__get_schemas, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, getSchema,			arginfo_mysqlx_base_session__get_schema, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_base_session, createSchema,		arginfo_mysqlx_base_session__create_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, dropSchema,			arginfo_mysqlx_base_session__drop_schema, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_base_session, dropCollection,		arginfo_mysqlx_base_session__drop_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, dropTable,			arginfo_mysqlx_base_session__drop_table, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_base_session, listClients,		arginfo_mysqlx_base_session__list_clients, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, killClient,			arginfo_mysqlx_base_session__kill_client, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_base_session, close,				arginfo_mysqlx_base_session__close, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_base_session_handlers;
static HashTable mysqlx_base_session_properties;

const struct st_mysqlx_property_entry mysqlx_base_session_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_base_session_free_storage */
static void
mysqlx_base_session_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_session * inner_obj = (struct st_mysqlx_session *) mysqlx_object->ptr;

	if (inner_obj) {
		XMYSQLND_NODE_SESSION * session = (XMYSQLND_NODE_SESSION *) inner_obj->session;

		if (session) {
			session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
			session->m->free_reference(session);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_base_session_object_allocator */
static zend_object *
php_mysqlx_base_session_object_allocator(zend_class_entry * class_type)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory = MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_session * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_session));
	MYSQLND_STATS * stats = NULL;
	MYSQLND_ERROR_INFO * error_info = NULL;

	DBG_ENTER("php_mysqlx_base_session_object_allocator");
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

	mysqlx_object->zo.handlers = &mysqlx_object_base_session_handlers;
	mysqlx_object->properties = &mysqlx_base_session_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_base_session_class */
void
mysqlx_register_base_session_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_base_session_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_base_session_handlers.free_obj = mysqlx_base_session_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "BaseSession", mysqlx_base_session_methods);
		tmp_ce.create_object = php_mysqlx_base_session_object_allocator;
		mysqlx_base_session_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_base_session_class_entry, 1, mysqlx_session_interface_entry);
	}

	zend_hash_init(&mysqlx_base_session_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_base_session_properties, mysqlx_base_session_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_base_session_class */
void
mysqlx_unregister_base_session_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_base_session_properties);
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

