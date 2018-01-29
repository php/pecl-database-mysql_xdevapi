/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_collection.h"
#include "xmysqlnd_node_table.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_structs.h"
#include "xmysqlnd_utils.h"

namespace mysqlx {

namespace drv {

namespace {

const MYSQLND_CSTRING db_object_type_filter_table_tag = { "TABLE", sizeof("TABLE") - 1 };
const MYSQLND_CSTRING db_object_type_filter_collection_tag = { "COLLECTION", sizeof("COLLECTION") - 1 };
const MYSQLND_CSTRING db_object_type_filter_view_tag = { "VIEW", sizeof("VIEW") - 1 };

} // anonymous namespace

/* {{{ is_table_object_type */
bool is_table_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_table_tag);
}
/* }}} */


/* {{{ is_collection_object_type */
bool is_collection_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_collection_tag);
}
/* }}} */


/* {{{ is_view_object_type */
bool is_view_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_view_tag);
}
/* }}} */

//------------------------------------------------------------------------------

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


struct st_schema_exists_in_database_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	unsigned int counter;
};


/* {{{ schema_xplugin_op_var_binder */
static const enum_hnd_func_status
schema_xplugin_op_var_binder(
	void * context,
	XMYSQLND_NODE_SESSION * session,
	XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret{HND_FAIL};
	st_schema_exists_in_database_var_binder_ctx* ctx = (st_schema_exists_in_database_var_binder_ctx*) context;
	const MYSQLND_CSTRING* param{nullptr};
	DBG_ENTER("schema_xplugin_op_var_binder");
	switch (ctx->counter) {
		case 0:{
			param = &ctx->schema_name;
			ret = HND_PASS;
			{
				enum_func_status result;
				zval zv;
				ZVAL_UNDEF(&zv);
				ZVAL_STRINGL(&zv, param->s, param->l);
				DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);
				result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, ctx->counter, &zv);

				zval_ptr_dtor(&zv);
				if (FAIL == result) {
					ret = HND_FAIL;
				}
			}
			break;
		}
		default:
			assert(!"should not happen");
			break;
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


struct st_schema_exists_in_database_ctx
{
	const MYSQLND_CSTRING expected_schema_name;
	zval* exists;
};


/* {{{ schema_sql_op_on_row */
static const enum_hnd_func_status
schema_sql_op_on_row(
	void * context,
	XMYSQLND_NODE_SESSION * const session,
	XMYSQLND_NODE_STMT * const stmt,
	const XMYSQLND_NODE_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	st_schema_exists_in_database_ctx* ctx = (st_schema_exists_in_database_ctx*) context;
	DBG_ENTER("schema_sql_op_on_row");
	if (ctx && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };

		if (equal_mysqlnd_cstr(object_name, ctx->expected_schema_name))
		{
			ZVAL_TRUE(ctx->exists);
		}
		else
		{
			ZVAL_FALSE(ctx->exists);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::exists_in_database */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, exists_in_database)(
	XMYSQLND_NODE_SCHEMA * const schema,
	struct st_xmysqlnd_node_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_node_schema::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"SHOW SCHEMAS LIKE ?", sizeof("SHOW SCHEMAS LIKE ?") - 1 };
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	st_schema_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		0
	};
	const st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { schema_xplugin_op_var_binder, &var_binder_ctx };

	st_schema_exists_in_database_ctx on_row_ctx = {
		mnd_str2c(schema->data->schema_name),
		exists
	};

	const st_xmysqlnd_node_session_on_row_bind on_row = { schema_sql_op_on_row, &on_row_ctx };

	ret = session->m->query_cb(session,
							   namespace_sql,
							   query,
							   var_binder,
							   noop__on_result_start,
							   on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_collection_object */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING collection_name)
{
	XMYSQLND_NODE_COLLECTION* collection{nullptr};
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

/* {{{ collection_op_handler_on_error */
static const enum_hnd_func_status
collection_op_handler_on_error(void * context,
							   XMYSQLND_NODE_SESSION * const session,
							   XMYSQLND_NODE_STMT * const stmt,
							   const unsigned int code,
							   const MYSQLND_CSTRING sql_state,
							   const MYSQLND_CSTRING message)
{
	st_create_collection_handler_ctx* ctx = (st_create_collection_handler_ctx*) context;
	DBG_ENTER("collection_op_handler_on_error");
	ctx->on_error.handler(ctx->on_error.ctx, ctx->schema, code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ collection_op_var_binder */
static const enum_hnd_func_status
collection_op_var_binder(void * context, XMYSQLND_NODE_SESSION * session, XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	DBG_ENTER("collection_op_var_binder");
	enum_hnd_func_status ret{HND_FAIL};
	st_collection_op_var_binder_ctx* ctx = static_cast<st_collection_op_var_binder_ctx*>(context);
	const MYSQLND_CSTRING* param{nullptr};
	enum_func_status result;
	zval zv;
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			break;
		case 1:{
			param = &ctx->collection_name;
			ret = HND_PASS;
			break;
		}
		default: /* should not happen */
			break;
	}
	if(ret != HND_FAIL) {
		ZVAL_UNDEF(&zv);
		ZVAL_STRINGL(&zv, param->s, param->l);
		DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);
		result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, ctx->counter, &zv);
		//result = stmt->data->m.bind_one_stmt_param(stmt, ctx->counter, &zv);
		zval_ptr_dtor(&zv);
		if (FAIL == result) {
			ret = HND_FAIL;
		}
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_collection_op */
static const enum_func_status
xmysqlnd_collection_op(
	XMYSQLND_NODE_SCHEMA * const schema,
	const util::string_view& collection_name,
	const MYSQLND_CSTRING query,
	const st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	st_collection_op_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		collection_name.to_nd_cstr(),
		0
	};
	const st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	st_create_collection_handler_ctx handler_ctx = { schema, handler_on_error };
	const st_xmysqlnd_node_session_on_error_bind on_error
		= { handler_on_error.handler ? collection_op_handler_on_error : nullptr, &handler_ctx };

	DBG_ENTER("xmysqlnd_collection_op");

	ret = session->m->query_cb(session,
							   namespace_xplugin,
							   query,
							   var_binder,
							   noop__on_result_start,
							   noop__on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_collection */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection)(
	XMYSQLND_NODE_SCHEMA* const schema,
	const util::string_view& collection_name,
	const st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	static const MYSQLND_CSTRING query = {"create_collection", sizeof("create_collection") - 1 };
	XMYSQLND_NODE_COLLECTION* collection{nullptr};
	DBG_ENTER("xmysqlnd_node_schema::create_collection");
	DBG_INF_FMT("schema_name=%s collection_name=%s", schema->data->schema_name.s, collection_name.c_str());
	if (PASS == xmysqlnd_collection_op(schema, collection_name, query, handler_on_error)) {
		collection = xmysqlnd_node_collection_create(
			schema,
			collection_name.to_nd_cstr(),
			schema->persistent,
			schema->data->object_factory,
			schema->data->session->data->stats,
			schema->data->session->data->error_info);
	}
	DBG_RETURN(collection);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::drop_collection */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, drop_collection)(
	XMYSQLND_NODE_SCHEMA * const schema,
	const util::string_view& collection_name,
	const struct st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_node_schema::drop_collection");
	DBG_INF_FMT("schema_name=%s collection_name=%s", schema->data->schema_name.s, collection_name.c_str());

	ret = xmysqlnd_collection_op(schema, collection_name.to_nd_cstr(), query, handler_on_error);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::create_table_object */
static XMYSQLND_NODE_TABLE *
XMYSQLND_METHOD(xmysqlnd_node_schema, create_table_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING table_name)
{
	XMYSQLND_NODE_TABLE* table{nullptr};
	DBG_ENTER("xmysqlnd_node_schema::create_table_object");
	DBG_INF_FMT("schema_name=%s", table_name.s);

	table = xmysqlnd_node_table_create(schema, table_name, schema->persistent, schema->data->object_factory, schema->data->session->data->stats, schema->data->session->data->error_info);
	DBG_RETURN(table);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::drop_table */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, drop_table)(
	XMYSQLND_NODE_SCHEMA* const schema,
	const util::string_view& table_name,
	const st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_node_schema::drop_table");
	DBG_INF_FMT("schema_name=%s table_name=%s ", schema->data->schema_name.s, table_name.c_str());

	ret = xmysqlnd_collection_op(schema, table_name.to_nd_cstr(), query, handler_on_error);

	DBG_RETURN(ret);
}
/* }}} */


namespace {

struct xmysqlnd_schema_get_db_objects_ctx
{
	XMYSQLND_NODE_SCHEMA * schema;
	const db_object_type_filter object_type_filter;
	const st_xmysqlnd_node_schema_on_database_object_bind on_object;
	const st_xmysqlnd_node_schema_on_error_bind on_error;
};


/* {{{ get_db_objects_on_row */
bool match_object_type(
	const db_object_type_filter object_type_filter,
	const MYSQLND_CSTRING& object_type)
{
	switch (object_type_filter) {
		case db_object_type_filter::table_or_view:
			return is_table_object_type(object_type) || is_view_object_type(object_type);

		case db_object_type_filter::collection:
			return is_collection_object_type(object_type);

		default:
			assert(!"unexpected object_type_filter!");
			return false;
	}
}
/* }}} */


/* {{{ get_db_objects_on_row */
static const enum_hnd_func_status
get_db_objects_on_row(void * context,
					  XMYSQLND_NODE_SESSION * const session,
					  XMYSQLND_NODE_STMT * const stmt,
					  const XMYSQLND_NODE_STMT_RESULT_META * const meta,
					  const zval * const row,
					  MYSQLND_STATS * const stats,
					  MYSQLND_ERROR_INFO * const error_info)
{
	const xmysqlnd_schema_get_db_objects_ctx* ctx = static_cast<const xmysqlnd_schema_get_db_objects_ctx*>(context);
	DBG_ENTER("get_db_objects_on_row");
	DBG_INF_FMT("handler=%p", ctx->on_object.handler);
	if (ctx && ctx->on_object.handler && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const MYSQLND_CSTRING object_type = { Z_STRVAL(row[1]), Z_STRLEN(row[1]) };
		DBG_INF_FMT("name=%*s", object_name.l, object_name.s);
		DBG_INF_FMT("type=%*s", object_type.l, object_type.s);

		if (match_object_type(ctx->object_type_filter, object_type)) {
			ctx->on_object.handler(ctx->on_object.ctx, ctx->schema, object_name, object_type);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


struct st_collection_get_objects_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	unsigned int counter;
};


/* {{{ collection_get_objects_var_binder */
static const enum_hnd_func_status
collection_get_objects_var_binder(void * context, XMYSQLND_NODE_SESSION * session, XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret{HND_FAIL};
	st_collection_get_objects_var_binder_ctx* ctx = (st_collection_get_objects_var_binder_ctx*) context;
	const MYSQLND_CSTRING* param{nullptr};
	DBG_ENTER("collection_get_objects_var_binder");
	DBG_INF_FMT("counter=%d", ctx->counter);
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_PASS;
			{
				enum_func_status result;
				zval zv;
				ZVAL_UNDEF(&zv);
				ZVAL_STRINGL(&zv, param->s, param->l);
				DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);

				result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, ctx->counter, &zv);

//				result = stmt->data->m.bind_one_stmt_param(stmt, ctx->counter, &zv);

				zval_ptr_dtor(&zv);
				if (FAIL == result) {
					ret = HND_FAIL;
				}
			}
			break;
		default: /* should not happen */
			break;
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */

} // anonymous namespace

/* {{{ xmysqlnd_node_schema::get_db_objects */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, get_db_objects)(
	XMYSQLND_NODE_SCHEMA * const schema,
	const MYSQLND_CSTRING& collection_name,
	const db_object_type_filter object_type_filter,
	const struct st_xmysqlnd_node_schema_on_database_object_bind on_object,
	const struct st_xmysqlnd_node_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	struct st_collection_get_objects_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_get_objects_var_binder, &var_binder_ctx };

	xmysqlnd_schema_get_db_objects_ctx handler_ctx = { schema, object_type_filter, on_object, handler_on_error };

	const struct st_xmysqlnd_node_session_on_row_bind on_row = { on_object.handler? get_db_objects_on_row : nullptr, &handler_ctx };
	const struct st_xmysqlnd_node_session_on_error_bind on_error = { handler_on_error.handler? collection_op_handler_on_error : nullptr, &handler_ctx };

	DBG_ENTER("xmysqlnd_node_schema::get_db_objects");

	ret = session->m->query_cb(session,
							   namespace_xplugin,
							   query,
							   var_binder,
							   noop__on_result_start,
							   on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
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
	enum_func_status ret{PASS};
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
		schema->data->schema_name.s = nullptr;
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
	XMYSQLND_METHOD(xmysqlnd_node_schema, exists_in_database),

	XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection_object),
	XMYSQLND_METHOD(xmysqlnd_node_schema, create_collection),
	XMYSQLND_METHOD(xmysqlnd_node_schema, drop_collection),
	XMYSQLND_METHOD(xmysqlnd_node_schema, create_table_object),
	XMYSQLND_METHOD(xmysqlnd_node_schema, drop_table),

	XMYSQLND_METHOD(xmysqlnd_node_schema, get_db_objects),

	XMYSQLND_METHOD(xmysqlnd_node_schema, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_schema, dtor),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_schema);


/* {{{ xmysqlnd_node_schema_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_SCHEMA *
xmysqlnd_node_schema_create(XMYSQLND_NODE_SESSION * session,
							const MYSQLND_CSTRING schema_name,
							const zend_bool persistent,
							const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
							MYSQLND_STATS * const stats,
							MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_SCHEMA* ret{nullptr};
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
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_schema_free(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_schema_free");
	DBG_INF_FMT("schema=%p  schema->data=%p  dtor=%p", schema, schema? schema->data:nullptr, schema? schema->data->m.dtor:nullptr);
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

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
