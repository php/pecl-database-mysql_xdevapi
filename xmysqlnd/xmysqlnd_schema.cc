/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_collection.h"
#include "xmysqlnd_table.h"
#include "xmysqlnd_schema.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_structs.h"
#include "xmysqlnd_utils.h"

namespace mysqlx {

namespace drv {

namespace {

const MYSQLND_CSTRING db_object_type_filter_table_tag = { "TABLE", sizeof("TABLE") - 1 };
const MYSQLND_CSTRING db_object_type_filter_collection_tag = { "COLLECTION", sizeof("COLLECTION") - 1 };
const MYSQLND_CSTRING db_object_type_filter_view_tag = { "VIEW", sizeof("VIEW") - 1 };

} // anonymous namespace

bool is_table_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_table_tag);
}

bool is_collection_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_collection_tag);
}

bool is_view_object_type(const MYSQLND_CSTRING& object_type)
{
	return equal_mysqlnd_cstr(object_type, db_object_type_filter_view_tag);
}

//------------------------------------------------------------------------------

struct st_schema_exists_in_database_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	unsigned int counter;
};

xmysqlnd_schema::xmysqlnd_schema(
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const obj_factory,
		XMYSQLND_SESSION provided_session,
		const MYSQLND_CSTRING provided_schema_name,
		zend_bool is_persistent)
{
	DBG_ENTER("xmysqlnd_schema::xmysqlnd_schema");
	session = provided_session;
	schema_name = mnd_dup_cstring(provided_schema_name, persistent);
	DBG_INF_FMT("name=[%d]%*s", provided_schema_name.l, provided_schema_name.l, provided_schema_name.s);
	persistent = is_persistent;
	object_factory = obj_factory;
}

xmysqlnd_schema::~xmysqlnd_schema()
{
	DBG_ENTER("xmysqlnd_schema::~xmysqlnd_schema");
	cleanup();
	session.~shared_ptr();
}

void xmysqlnd_schema::cleanup()
{
	DBG_ENTER("xmysqlnd_schema::cleanup");
	if (schema_name.s) {
		mnd_efree(schema_name.s);
		schema_name.s = nullptr;
	}
	DBG_VOID_RETURN;
}


static const enum_hnd_func_status
schema_xplugin_op_var_binder(
	void * context,
	XMYSQLND_SESSION session,
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

struct st_schema_exists_in_database_ctx
{
	const MYSQLND_CSTRING expected_schema_name;
	zval* exists;
};


static const enum_hnd_func_status
schema_sql_op_on_row(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
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

enum_func_status
xmysqlnd_schema::exists_in_database(
	struct st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_schema::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"SHOW SCHEMAS LIKE ?", sizeof("SHOW SCHEMAS LIKE ?") - 1 };

	st_schema_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema_name),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = {
		schema_xplugin_op_var_binder,
		&var_binder_ctx
	};

	st_schema_exists_in_database_ctx on_row_ctx = {
		mnd_str2c(schema_name),
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { schema_sql_op_on_row, &on_row_ctx };

	ret = session->query_cb(namespace_sql,
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

xmysqlnd_collection *
xmysqlnd_schema::create_collection_object(
		const MYSQLND_CSTRING collection_name
)
{
	xmysqlnd_collection* collection{nullptr};
	DBG_ENTER("xmysqlnd_schema::create_collection_object");
	DBG_INF_FMT("schema_name=%s", collection_name.s);

	collection = xmysqlnd_collection_create(this,
											collection_name,
											persistent,
											object_factory,
											session->data->stats,
											session->data->error_info);
	DBG_RETURN(collection);
}

struct st_create_collection_handler_ctx
{
	const xmysqlnd_schema * schema;
	const struct st_xmysqlnd_schema_on_error_bind on_error;
};

static const enum_hnd_func_status
collection_op_handler_on_error(void * context,
							   XMYSQLND_SESSION session,
							   xmysqlnd_stmt * const /*stmt*/,
							   const unsigned int code,
							   const MYSQLND_CSTRING sql_state,
							   const MYSQLND_CSTRING message)
{
	st_create_collection_handler_ctx* ctx = (st_create_collection_handler_ctx*) context;
	DBG_ENTER("collection_op_handler_on_error");
	ctx->on_error.handler(ctx->on_error.ctx, ctx->schema, code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

static const enum_hnd_func_status
collection_op_var_binder(void * context, XMYSQLND_SESSION session, XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
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
		//result = stmt->m.bind_one_stmt_param(stmt, ctx->counter, &zv);
		zval_ptr_dtor(&zv);
		if (FAIL == result) {
			ret = HND_FAIL;
		}
	}
	++ctx->counter;
	DBG_RETURN(ret);
}

static const enum_func_status
xmysqlnd_collection_op(
	xmysqlnd_schema * const schema,
	const util::string_view& collection_name,
	const MYSQLND_CSTRING query,
	const st_xmysqlnd_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	auto session = schema->get_session();

	st_collection_op_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->get_name()),
		collection_name.to_nd_cstr(),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	st_create_collection_handler_ctx handler_ctx = { schema, handler_on_error };
	const st_xmysqlnd_session_on_error_bind on_error
		= { handler_on_error.handler ? collection_op_handler_on_error : nullptr, &handler_ctx };

	DBG_ENTER("xmysqlnd_collection_op");

	ret = session->query_cb(namespace_xplugin,
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

xmysqlnd_collection *
xmysqlnd_schema::create_collection(
	const util::string_view& collection_name,
	const st_xmysqlnd_schema_on_error_bind handler_on_error)
{
	static const MYSQLND_CSTRING query = {"create_collection", sizeof("create_collection") - 1 };
	xmysqlnd_collection* collection{nullptr};
	DBG_ENTER("xmysqlnd_schema::create_collection");
	DBG_INF_FMT("schema_name=%s collection_name=%s", schema_name.s, collection_name.c_str());
	if (PASS == xmysqlnd_collection_op(this, collection_name, query, handler_on_error)) {
		collection = xmysqlnd_collection_create(
			this,
			collection_name.to_nd_cstr(),
			persistent,
			object_factory,
			session->data->stats,
			session->data->error_info);
	}
	DBG_RETURN(collection);
}

enum_func_status
xmysqlnd_schema::drop_collection(
	const util::string_view& collection_name,
	const struct st_xmysqlnd_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_schema::drop_collection");
	DBG_INF_FMT("schema_name=%s collection_name=%s", schema_name.s, collection_name.c_str());

	ret = xmysqlnd_collection_op(this, collection_name.to_nd_cstr(), query, handler_on_error);

	DBG_RETURN(ret);
}

xmysqlnd_table *
xmysqlnd_schema::create_table_object( const MYSQLND_CSTRING table_name)
{
	xmysqlnd_table* table{nullptr};
	DBG_ENTER("xmysqlnd_schema::create_table_object");
	DBG_INF_FMT("schema_name=%s", table_name.s);

	table = xmysqlnd_table_create(this, table_name, persistent,
								  object_factory,
								  session->data->stats,
								  session->data->error_info);
	DBG_RETURN(table);
}

enum_func_status
xmysqlnd_schema::drop_table(
	const util::string_view& table_name,
	const st_xmysqlnd_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_schema::drop_table");
	DBG_INF_FMT("schema_name=%s table_name=%s ", schema_name.s, table_name.c_str());

	ret = xmysqlnd_collection_op(this, table_name.to_nd_cstr(), query, handler_on_error);

	DBG_RETURN(ret);
}

namespace {

struct xmysqlnd_schema_get_db_objects_ctx
{
	xmysqlnd_schema * schema;
	const db_object_type_filter object_type_filter;
	const st_xmysqlnd_schema_on_database_object_bind on_object;
	const st_xmysqlnd_schema_on_error_bind on_error;
};


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

static const enum_hnd_func_status
get_db_objects_on_row(void * context,
					  XMYSQLND_SESSION session,
					  xmysqlnd_stmt * const /*stmt*/,
					  const XMYSQLND_STMT_RESULT_META * const /*meta*/,
					  const zval * const row,
					  MYSQLND_STATS * const /*stats*/,
					  MYSQLND_ERROR_INFO * const /*error_info*/)
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

struct st_collection_get_objects_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	unsigned int counter;
};


static const enum_hnd_func_status
collection_get_objects_var_binder(void * context, XMYSQLND_SESSION session, XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
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

//				result = stmt->m.bind_one_stmt_param(stmt, ctx->counter, &zv);

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

} // anonymous namespace

enum_func_status
xmysqlnd_schema::get_db_objects(
	const MYSQLND_CSTRING& /*collection_name*/,
	const db_object_type_filter object_type_filter,
	const struct st_xmysqlnd_schema_on_database_object_bind on_object,
	const struct st_xmysqlnd_schema_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };

	struct st_collection_get_objects_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema_name),
		0
	};
	const struct st_xmysqlnd_session_query_bind_variable_bind var_binder = { collection_get_objects_var_binder, &var_binder_ctx };

	xmysqlnd_schema_get_db_objects_ctx handler_ctx = { this, object_type_filter, on_object, handler_on_error };

	const struct st_xmysqlnd_session_on_row_bind on_row = { on_object.handler? get_db_objects_on_row : nullptr, &handler_ctx };
	const struct st_xmysqlnd_session_on_error_bind on_error = { handler_on_error.handler? collection_op_handler_on_error : nullptr, &handler_ctx };

	DBG_ENTER("xmysqlnd_schema::get_db_objects");

	ret = session->query_cb(namespace_xplugin,
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

xmysqlnd_schema *
xmysqlnd_schema::get_reference()
{
	DBG_ENTER("xmysqlnd_schema::get_reference");
	++refcount;
	DBG_INF_FMT("new_refcount=%u", refcount);
	DBG_RETURN(this);
}

enum_func_status
xmysqlnd_schema::free_reference(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_schema::free_reference");
	DBG_INF_FMT("old_refcount=%u",  refcount);
	if (!(--refcount)) {
		free_contents();
		session.~shared_ptr();
	}
	DBG_RETURN(ret);
}

void
xmysqlnd_schema::free_contents()
{
	DBG_ENTER("xmysqlnd_schema::free_contents");
	if (schema_name.s) {
		mnd_efree(schema_name.s);
		schema_name.s = nullptr;
	}
	DBG_VOID_RETURN;
}

PHP_MYSQL_XDEVAPI_API xmysqlnd_schema *
xmysqlnd_schema_create(XMYSQLND_SESSION session,
							const MYSQLND_CSTRING schema_name,
							const zend_bool persistent,
							const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
							MYSQLND_STATS * const stats,
							MYSQLND_ERROR_INFO * const error_info)
{
	xmysqlnd_schema* ret{nullptr};
	DBG_ENTER("xmysqlnd_schema_create");
	if (schema_name.s && schema_name.l) {
		ret = object_factory->get_schema(object_factory, session, schema_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->get_reference();
		}
	}
	DBG_RETURN(ret);
}

PHP_MYSQL_XDEVAPI_API void
xmysqlnd_schema_free(xmysqlnd_schema * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_schema_free");
	DBG_INF_FMT("schema=%p",
				schema);
	if (schema) {
		if (!stats && schema->get_session()->data) {
			stats = schema->get_session()->data->stats;
		}
		if (!error_info && schema->get_session()->data) {
			error_info = schema->get_session()->data->error_info;
		}
		schema->free_reference(stats, error_info);
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
