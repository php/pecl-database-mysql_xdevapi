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
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include <xmysqlnd/xmysqlnd_node_table.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_database_object.h"
#include "mysqlx_exception.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_table.h"
#include "mysqlx_node_schema.h"

static zend_class_entry *mysqlx_node_schema_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__drop, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema_object__get_schema, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()
/************************************** INHERITED END   ****************************************/


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__create_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collections, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_table, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_tables, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()



struct st_mysqlx_node_schema
{
	XMYSQLND_NODE_SCHEMA * schema;
};

static const MYSQLND_CSTRING namespace_sql = { "sql", sizeof("sql") - 1 };

#define MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_schema *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->schema) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_schema::__construct */
static
PHP_METHOD(mysqlx_node_schema, __construct)
{
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_node_schema::getSession() */
static
PHP_METHOD(mysqlx_node_schema, getSession)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_schema::getSession");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->schema) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getName() */
static
PHP_METHOD(mysqlx_node_schema, getName)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_schema::getName");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->schema) {
		RETVAL_STRINGL(object->schema->data->schema_name.s, object->schema->data->schema_name.l);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::existsInDatabase() */
static
PHP_METHOD(mysqlx_node_schema, existsInDatabase)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_schema::existsInDatabase");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->schema) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::drop() */
static
PHP_METHOD(mysqlx_node_schema, drop)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_schema::drop");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->schema) {
		XMYSQLND_NODE_SESSION * session = object->schema->data->session;
		const MYSQLND_CSTRING schema_name = mnd_str2c(object->schema->data->schema_name);

		RETVAL_BOOL(session && PASS == session->m->drop_db(session, schema_name));
	}

	DBG_VOID_RETURN;
}
/* }}} */
/************************************** INHERITED END   ****************************************/
/* {{{ mysqlx_node_schema_on_error */
static const enum_hnd_func_status
mysqlx_node_schema_on_error(void * context, const XMYSQLND_NODE_SCHEMA * const schema, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_schema_on_error");
	mysqlx_new_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::createCollection(string name) */
static
PHP_METHOD(mysqlx_node_schema, createCollection)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;
	MYSQLND_CSTRING collection_name = { NULL, 0 };

	DBG_ENTER("mysqlx_node_schema::createCollection");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(collection_name.s), &(collection_name.l)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if (collection_name.s && collection_name.l && object->schema) {
		const struct st_xmysqlnd_node_schema_on_error_bind on_error = { mysqlx_node_schema_on_error, NULL };

		struct st_xmysqlnd_node_collection * const collection = object->schema->data->m.create_collection(object->schema, collection_name, on_error);
		if (collection) {
			mysqlx_new_node_collection(return_value, collection, FALSE);
			if (Z_TYPE_P(return_value) != IS_OBJECT) {
				xmysqlnd_node_collection_free(collection, NULL, NULL);

			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getCollection(string name) */
static
PHP_METHOD(mysqlx_node_schema, getCollection)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;
	MYSQLND_CSTRING collection_name = { NULL, 0 };

	DBG_ENTER("mysqlx_node_schema::getCollection");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(collection_name.s), &(collection_name.l)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if (collection_name.s && collection_name.l && object->schema) {
		struct st_xmysqlnd_node_collection * const collection = object->schema->data->m.create_collection_object(object->schema, collection_name);
		if (collection) {
			mysqlx_new_node_collection(return_value, collection, FALSE);
			if (Z_TYPE_P(return_value) != IS_OBJECT) {
				xmysqlnd_node_collection_free(collection, NULL, NULL);
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */



struct st_mysqlx_get_list_from_schema_ctx
{
	XMYSQLND_NODE_SCHEMA * schema;
	zval * list;
};


/* {{{ get_collections_handler_on_row */
static const enum_hnd_func_status
get_collections_handler_on_row(void * context,
							   XMYSQLND_NODE_SESSION * const session,
							   XMYSQLND_NODE_STMT * const stmt,
							   const struct st_xmysqlnd_node_stmt_result_meta * const meta,
							   const zval * const row,
							   MYSQLND_STATS * const stats,
							   MYSQLND_ERROR_INFO * const error_info)
{
	const struct st_mysqlx_get_list_from_schema_ctx * ctx = (const struct st_mysqlx_get_list_from_schema_ctx *) context;
	DBG_ENTER("get_collections_handler_on_row");
	if (ctx && ctx->list && row) {
		if (Z_TYPE_P(ctx->list) != IS_ARRAY) {
			array_init(ctx->list);
		}
		if (Z_TYPE_P(ctx->list) == IS_ARRAY) {
			const MYSQLND_CSTRING collection_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
			XMYSQLND_NODE_SCHEMA * const schema = ctx->schema;
			XMYSQLND_NODE_COLLECTION * const collection = schema->data->m.create_collection_object(schema, collection_name);
			if (collection) {
				zval zv;
				ZVAL_UNDEF(&zv);
				mysqlx_new_node_collection(&zv, collection, FALSE);
				if (Z_TYPE(zv) == IS_OBJECT) {
					add_assoc_zval_ex(ctx->list, collection_name.s, collection_name.l, &zv);
#if 0
					zend_hash_next_index_insert(Z_ARRVAL_P(ctx->list), &zv);
#endif
				} else {
					xmysqlnd_node_collection_free(collection, NULL, NULL);
					zval_dtor(&zv);
				}
			}
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ get_list_from_schema_handler_on_error */
static const enum_hnd_func_status
get_list_from_schema_handler_on_error(void * context,
									  XMYSQLND_NODE_SESSION * const session,
									  XMYSQLND_NODE_STMT * const stmt,
									  const unsigned int code,
									  const MYSQLND_CSTRING sql_state,
									  const MYSQLND_CSTRING message)
{
	DBG_ENTER("get_list_from_schema_handler_on_error");
	if (session) {
		session->data->m->handler_on_error(session->data, code, sql_state, message);
	}
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


struct st_mysqlx_get_collections_builder
{
	struct st_xmysqlnd_query_builder parent;
	struct {
		const char * format;
		MYSQLND_CSTRING db;
		XMYSQLND_NODE_SESSION * session;
	} ext;
};


/* {{{ list_from_schema__create_query */
static enum_func_status
list_from_schema__create_query(struct st_xmysqlnd_query_builder * b)
{
	struct st_mysqlx_get_collections_builder * ctx = (struct st_mysqlx_get_collections_builder *) b;
	XMYSQLND_NODE_SESSION_DATA * const session = ctx->ext.session->data; 
	const MYSQLND_STRING quoted_db = session->m->quote_name(session, ctx->ext.db);

	DBG_ENTER("list_from_schema__create_query");

	b->query.l = spprintf(&b->query.s, 0, ctx->ext.format, quoted_db.l, quoted_db.s);

	DBG_INF_FMT("query=%*s", b->query.l, b->query.s);
	DBG_RETURN(b->query.s? PASS:FAIL);
}
/* }}} */


/* {{{ list_from_schema__destroy_query */
static void
list_from_schema__destroy_query(struct st_xmysqlnd_query_builder * b)
{
	if (b->query.s) {
		mnd_efree(b->query.s);
		b->query.s = NULL;
	}
	b->query.l = 0;
}
/* }}} */


/* {{{ mysqlx_node_session::getCollections() */
static
PHP_METHOD(mysqlx_node_schema, getCollections)
{
	zval * object_zv;
	struct st_mysqlx_node_schema * object;
	XMYSQLND_NODE_SCHEMA * schema;

	DBG_ENTER("mysqlx_node_schema::getCollections");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ((schema = object->schema)) {
		XMYSQLND_NODE_SESSION * const session = schema->data->session;
		const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { NULL, NULL };
		struct st_mysqlx_get_collections_builder query_builder = {
			{
				list_from_schema__create_query,
				list_from_schema__destroy_query,
				{ NULL, 0 }
			},
			{
				"SHOW TABLES FROM %*s",
				mnd_str2c(schema->data->schema_name),
				session
			}
		};
		zval list;
		struct st_mysqlx_get_list_from_schema_ctx ctx = { schema, &list };
		const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_row_bind on_row = { get_collections_handler_on_row, &ctx };
		const struct st_xmysqlnd_node_session_on_warning_bind on_warning = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { get_list_from_schema_handler_on_error, &ctx };
		const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok = { NULL, NULL };

		ZVAL_UNDEF(&list);

		if (PASS == session->m->query_cb_ex(session, namespace_sql, &query_builder.parent, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getTable(string name) */
static
PHP_METHOD(mysqlx_node_schema, getTable)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;
	MYSQLND_CSTRING table_name = { NULL, 0 };

	DBG_ENTER("mysqlx_node_schema::getTable");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(table_name.s), &(table_name.l)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if (table_name.s && table_name.l && object->schema) {
		struct st_xmysqlnd_node_table * const table = object->schema->data->m.create_table_object(object->schema, table_name);
		mysqlx_new_node_table(return_value, table, FALSE /* no clone */);
		if (Z_TYPE_P(return_value) != IS_OBJECT) {
			xmysqlnd_node_table_free(table, NULL, NULL);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ get_tables_handler_on_row */
static const enum_hnd_func_status
get_tables_handler_on_row(void * context,
						  XMYSQLND_NODE_SESSION * const session,
						  XMYSQLND_NODE_STMT * const stmt,
						  const struct st_xmysqlnd_node_stmt_result_meta * const meta,
						  const zval * const row,
						  MYSQLND_STATS * const stats,
						  MYSQLND_ERROR_INFO * const error_info)
{
	const struct st_mysqlx_get_list_from_schema_ctx * ctx = (const struct st_mysqlx_get_list_from_schema_ctx *) context;
	DBG_ENTER("get_tables_handler_on_row");
	if (ctx && ctx->list && row) {
		if (Z_TYPE_P(ctx->list) != IS_ARRAY) {
			array_init(ctx->list);
		}
		if (Z_TYPE_P(ctx->list) == IS_ARRAY) {
			const MYSQLND_CSTRING table_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
			XMYSQLND_NODE_SCHEMA * const schema = ctx->schema;
			XMYSQLND_NODE_TABLE * const table = schema->data->m.create_table_object(schema, table_name);
			if (table) {
				zval zv;
				ZVAL_UNDEF(&zv);
				mysqlx_new_node_table(&zv, table, FALSE);
				if (Z_TYPE(zv) == IS_OBJECT) {
					add_assoc_zval_ex(ctx->list, table_name.s, table_name.l, &zv);
#if 0
					zend_hash_next_index_insert(Z_ARRVAL_P(ctx->list), &zv);
#endif
				} else {			
					xmysqlnd_node_table_free(table, NULL, NULL);
					zval_dtor(&zv);
				}
			}
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ mysqlx_node_session::getTables() */
static
PHP_METHOD(mysqlx_node_schema, getTables)
{
	zval * object_zv;
	struct st_mysqlx_node_schema * object;
	XMYSQLND_NODE_SCHEMA * schema;

	DBG_ENTER("mysqlx_node_schema::getTables");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ((schema = object->schema)) {
		XMYSQLND_NODE_SESSION * const session = schema->data->session;
		const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { NULL, NULL };
		struct st_mysqlx_get_collections_builder query_builder = {
			{
				list_from_schema__create_query,
				list_from_schema__destroy_query,
				{ NULL, 0 }
			},
			{
				"SHOW TABLES FROM %*s",
				mnd_str2c(schema->data->schema_name),
				session
			}
		};
		zval list;
		struct st_mysqlx_get_list_from_schema_ctx ctx = { schema, &list };
		const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_row_bind on_row = { get_tables_handler_on_row, &ctx };
		const struct st_xmysqlnd_node_session_on_warning_bind on_warning = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { get_list_from_schema_handler_on_error, &ctx };
		const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end = { NULL, NULL };
		const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok = { NULL, NULL };

		ZVAL_UNDEF(&list);

		if (PASS == session->m->query_cb_ex(session, namespace_sql, &query_builder.parent, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_schema_methods[] */
static const zend_function_entry mysqlx_node_schema_methods[] = {
	PHP_ME(mysqlx_node_schema, __construct,		NULL,											ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_node_schema, getSession,			arginfo_mysqlx_node_schema__get_session,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getName,				arginfo_mysqlx_node_schema__get_name,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, existsInDatabase,	arginfo_mysqlx_node_schema__exists_in_database,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, drop,				arginfo_mysqlx_node_schema__drop,				ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/
	PHP_ME(mysqlx_node_schema, createCollection,	arginfo_mysqlx_node_schema__create_collection,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getCollection,		arginfo_mysqlx_node_schema__get_collection,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getCollections,		arginfo_mysqlx_node_schema__get_collections,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getTable,			arginfo_mysqlx_node_schema__get_table,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getTables,			arginfo_mysqlx_node_schema__get_tables,			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_node_schema_property__name */
static zval *
mysqlx_node_schema_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_schema * object = (const struct st_mysqlx_node_schema *) (obj->ptr);
	DBG_ENTER("mysqlx_node_schema_property__name");
	if (object->schema && object->schema->data->schema_name.s) {
		ZVAL_STRINGL(return_value, object->schema->data->schema_name.s, object->schema->data->schema_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


static zend_object_handlers mysqlx_object_node_schema_handlers;
static HashTable mysqlx_node_schema_properties;

const struct st_mysqlx_property_entry mysqlx_node_schema_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_node_schema_property__name,	NULL},
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_schema_free_storage */
static void
mysqlx_node_schema_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_schema * inner_obj = (struct st_mysqlx_node_schema *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->schema) {
			xmysqlnd_node_schema_free(inner_obj->schema, NULL, NULL);
			inner_obj->schema = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_schema_object_allocator */
static zend_object *
php_mysqlx_node_schema_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_schema * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_schema));

	DBG_ENTER("php_mysqlx_node_schema_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_schema_handlers;
	mysqlx_object->properties = &mysqlx_node_schema_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_schema_class */
void
mysqlx_register_node_schema_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_schema_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_schema_handlers.free_obj = mysqlx_node_schema_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSchema", mysqlx_node_schema_methods);
		tmp_ce.create_object = php_mysqlx_node_schema_object_allocator;
		mysqlx_node_schema_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_schema_class_entry, 1, mysqlx_database_object_interface_entry);
	}

	zend_hash_init(&mysqlx_node_schema_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_schema_properties, mysqlx_node_schema_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_schema_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_node_schema_class */
void
mysqlx_unregister_node_schema_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_schema_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_schema */
void
mysqlx_new_node_schema(zval * return_value, XMYSQLND_NODE_SCHEMA * schema)
{
	DBG_ENTER("mysqlx_new_node_schema");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_schema_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_schema * const object = (struct st_mysqlx_node_schema *) mysqlx_object->ptr;
		if (object) {
			object->schema = schema;
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
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
