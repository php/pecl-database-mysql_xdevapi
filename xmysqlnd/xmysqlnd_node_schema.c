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
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_collection.h"
#include "xmysqlnd_node_table.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_stmt.h"

static const MYSQLND_CSTRING namespace_xplugin = { "xplugin", sizeof("xplugin") - 1 };

/* {{{ xmysqlnd_node_schema::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, init)(XMYSQLND_NODE_SCHEMA * const schema,
											const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
											XMYSQLND_NODE_SESSION * const session,
											const MYSQLND_CSTRING schema_name,
											MYSQLND_STATS * const stats,
											MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_schema::init");
	if (!(schema->data->session = session->m->get_reference(session))) {
		return FAIL;
	}
	schema->data->schema_name = mnd_dup_cstring(schema_name, schema->data->persistent);
	DBG_INF_FMT("name=[%d]%*s", schema->data->schema_name.l, schema->data->schema_name.l, schema->data->schema_name.s);

	schema->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_collection_object */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING collection_name)
{
	XMYSQLND_NODE_COLLECTION * collection = NULL;
	DBG_ENTER("xmysqlnd_node_schema::create_collection_object");
	DBG_INF_FMT("schema_name=%s", collection_name.s);

	collection = xmysqlnd_node_collection_create(schema, collection_name, schema->persistent, schema->data->object_factory, schema->data->session->data->stats, schema->data->session->data->error_info);
	DBG_RETURN(collection);
}
/* }}} */


struct st_create_collection_handler_ctx
{
	const XMYSQLND_NODE_SCHEMA * schema;
	const struct st_xmysqlnd_node_schema_on_error_bind on_error;
};

/* {{{ create_collection_handler_on_error */
static const enum_hnd_func_status
create_collection_handler_on_error(void * context,
								   XMYSQLND_NODE_SESSION * const session,
								   XMYSQLND_NODE_STMT * const stmt,
								   const unsigned int code,
								   const MYSQLND_CSTRING sql_state,
								   const MYSQLND_CSTRING message)
{
	struct st_create_collection_handler_ctx * ctx = (struct st_create_collection_handler_ctx *) context;
	DBG_ENTER("create_collection_handler_on_error");
	ctx->on_error.handler(ctx->on_error.ctx, ctx->schema, code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


struct st_create_collection_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	const MYSQLND_CSTRING collection_name;
	unsigned int counter;
};


/* {{{ create_collection_var_binder */
static const enum_hnd_func_status
create_collection_var_binder(void * context, XMYSQLND_NODE_SESSION * session, XMYSQLND_NODE_STMT * const stmt)
{
	enum_hnd_func_status ret = HND_FAIL;
	struct st_create_collection_var_binder_ctx * ctx = (struct st_create_collection_var_binder_ctx *) context;
	const MYSQLND_CSTRING * param = NULL;
	DBG_ENTER("create_collection_var_binder");
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			goto bind;
		case 1:{
			param = &ctx->collection_name;
			ret = HND_PASS;
bind:
			{
				enum_func_status result;
				zval zv;
				ZVAL_UNDEF(&zv);
				ZVAL_STRINGL(&zv, param->s, param->l);
				DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);

				result = stmt->data->m.bind_one_param(stmt, ctx->counter, &zv);

				zval_ptr_dtor(&zv);
				if (FAIL == result) {
					ret = FAIL;
				}
			}
			break;
		}
		default: /* should not happen */
			break;
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_collection */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection)(XMYSQLND_NODE_SCHEMA * const schema,
														 const MYSQLND_CSTRING collection_name,
														 const struct st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	static const MYSQLND_CSTRING query = {"create_collection", sizeof("create_collection") - 1 };
	struct st_create_collection_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		collection_name,
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { create_collection_var_binder, &var_binder_ctx };
	XMYSQLND_NODE_COLLECTION * collection = NULL;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	struct st_create_collection_handler_ctx handler_ctx = { schema, handler_on_error };
	const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start = { NULL, NULL };
	const struct st_xmysqlnd_node_session_on_row_bind on_row = { NULL, NULL };
	const struct st_xmysqlnd_node_session_on_warning_bind on_warning = { NULL, NULL };
	const struct st_xmysqlnd_node_session_on_error_bind on_error = { handler_on_error.handler? create_collection_handler_on_error : NULL, &handler_ctx };
	const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end = { NULL, NULL };
	const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok = { NULL, NULL };

	DBG_ENTER("xmysqlnd_node_schema::create_collection");
	DBG_INF_FMT("schema_name=%s", collection_name.s);

	if (PASS == session->m->query_cb(session, namespace_xplugin, query, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
		collection = xmysqlnd_node_collection_create(schema,
													 collection_name,
													 schema->persistent,
													 schema->data->object_factory,
													 schema->data->session->data->stats,
													 schema->data->session->data->error_info);
	}
	DBG_RETURN(collection);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_table_object */
static XMYSQLND_NODE_TABLE *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_table_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING table_name)
{
	XMYSQLND_NODE_TABLE * table = NULL;
	DBG_ENTER("xmysqlnd_node_schema::create_table_object");
	DBG_INF_FMT("schema_name=%s", table_name.s);

	table = xmysqlnd_node_table_create(schema, table_name, schema->persistent, schema->data->object_factory, schema->data->session->data->stats, schema->data->session->data->error_info);
	DBG_RETURN(table);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::get_reference */
static XMYSQLND_NODE_SCHEMA *
XMYSQLND_METHOD(xmysqlnd_node_schema, get_reference)(XMYSQLND_NODE_SCHEMA * const schema)
{
	DBG_ENTER("xmysqlnd_node_schema::get_reference");
	++schema->data->refcount;
	DBG_INF_FMT("schema=%p new_refcount=%u", schema, schema->data->refcount);
	DBG_RETURN(schema);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, free_reference)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_schema::free_reference");
	DBG_INF_FMT("schema=%p old_refcount=%u", schema, schema->data->refcount);
	if (!(--schema->data->refcount)) {
		schema->data->m.dtor(schema, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_schema, free_contents)(XMYSQLND_NODE_SCHEMA * const schema)
{
	const zend_bool pers = schema->data->persistent;
	DBG_ENTER("xmysqlnd_node_schema::free_contents");
	if (schema->data->schema_name.s) {
		mnd_pefree(schema->data->schema_name.s, pers);
		schema->data->schema_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_schema::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_schema, dtor)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_schema::dtor");
	if (schema) {
		schema->data->m.free_contents(schema);
		schema->data->session->m->free_reference(schema->data->session);

		mnd_pefree(schema->data, schema->data->persistent);
		mnd_pefree(schema, schema->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_schema)
	XMYSQLND_METHOD(xmysqlnd_node_schema, init),

	XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection_object),
	XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection),
	XMYSQLND_METHOD(xmysqlnd_node_schema, create_table_object),

	XMYSQLND_METHOD(xmysqlnd_node_schema, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_schema, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_schema);

/* {{{ xmysqlnd_node_schema_create */
PHPAPI XMYSQLND_NODE_SCHEMA *
xmysqlnd_node_schema_create(XMYSQLND_NODE_SESSION * session,
							const MYSQLND_CSTRING schema_name,
							const zend_bool persistent,
							const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
							MYSQLND_STATS * const stats,
							MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_SCHEMA * ret = NULL;
	DBG_ENTER("xmysqlnd_node_schema_create");
	if (schema_name.s && schema_name.l) {
		ret = object_factory->get_node_schema(object_factory, session, schema_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->data->m.get_reference(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema_free */
PHPAPI void
xmysqlnd_node_schema_free(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_schema_free");
	DBG_INF_FMT("schema=%p  schema->data=%p  dtor=%p", schema, schema? schema->data:NULL, schema? schema->data->m.dtor:NULL);
	if (schema) {
		if (!stats && schema->data->session->data) {
			stats = schema->data->session->data->stats;
		}
		if (!error_info && schema->data->session->data) {
			error_info = schema->data->session->data->error_info;
		}
		schema->data->m.free_reference(schema, stats, error_info);
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
