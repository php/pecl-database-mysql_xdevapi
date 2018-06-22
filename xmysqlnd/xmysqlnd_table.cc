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
#include <ext/json/php_json_parser.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_schema.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_table.h"
#include "xmysqlnd_utils.h"
#include <vector>
#include "util/exceptions.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_table::xmysqlnd_table */
xmysqlnd_table::xmysqlnd_table(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const cur_obj_factory,
										   xmysqlnd_schema * const cur_schema,
										   const MYSQLND_CSTRING cur_table_name,
											zend_bool is_persistent)
{
	DBG_ENTER("xmysqlnd_table::st_xmysqlnd_table_data");
	if (!(schema = cur_schema->get_reference())) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::table_creation_failed);
	}
	persistent = is_persistent;
	table_name = mnd_dup_cstring(cur_table_name, persistent);
	DBG_INF_FMT("name=[%d]%*s", table_name.l, table_name.l, table_name.s);

	object_factory = cur_obj_factory;
}
/* }}} */

//------------------------------------------------------------------------------

struct table_or_view_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	const MYSQLND_CSTRING table_name;
	unsigned int counter;
};


/* {{{ table_op_var_binder */
const enum_hnd_func_status
table_op_var_binder(
	void * context,
	XMYSQLND_SESSION session,
	XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret{HND_FAIL};
	table_or_view_var_binder_ctx* ctx = static_cast<table_or_view_var_binder_ctx*>(context);
	const MYSQLND_CSTRING* param{nullptr};
	DBG_ENTER("table_op_var_binder");
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			goto bind;
		case 1:{
			param = &ctx->table_name;
			ret = HND_PASS;
bind:
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


struct table_or_view_op_ctx
{
	const MYSQLND_CSTRING expected_name;
	zval* exists;
};


/* {{{ table_or_view_exists_in_database_op */
const enum_hnd_func_status
table_or_view_exists_in_database_op(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const stmt,
	const XMYSQLND_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	table_or_view_op_ctx* ctx = static_cast<table_or_view_op_ctx*>(context);
	DBG_ENTER("table_or_view_exists_in_database_op");
	if (ctx && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const MYSQLND_CSTRING object_type = { Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if (equal_mysqlnd_cstr(object_name, ctx->expected_name)
			&& (is_table_object_type(object_type) || is_view_object_type(object_type)))
		{
			ZVAL_TRUE(ctx->exists);
			DBG_RETURN(HND_PASS);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */

/* {{{ xmysqlnd_table::exists_in_database */
enum_func_status
xmysqlnd_table::exists_in_database(
		struct st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_table::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };
	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	table_or_view_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->get_name()),
		mnd_str2c(get_name()),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	table_or_view_op_ctx on_row_ctx = {
		mnd_str2c(get_name()),
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { table_or_view_exists_in_database_op, &on_row_ctx };

	ret = session->query_cb(
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

//------------------------------------------------------------------------------

/* {{{ check_is_view_op */
const enum_hnd_func_status
check_is_view_op(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const stmt,
	const XMYSQLND_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	table_or_view_op_ctx* ctx = static_cast<table_or_view_op_ctx*>(context);
	DBG_ENTER("check_is_view_op");
	if (ctx && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const MYSQLND_CSTRING object_type = { Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if (equal_mysqlnd_cstr(object_name, ctx->expected_name) && is_view_object_type(object_type)) {
			ZVAL_TRUE(ctx->exists);
			DBG_RETURN(HND_PASS);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */

/* {{{ xmysqlnd_table::is_view */
enum_func_status
xmysqlnd_table::is_view(
	st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_table::is_view");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };
	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	table_or_view_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->get_name()),
		mnd_str2c(get_name()),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	table_or_view_op_ctx on_row_ctx = {
		mnd_str2c(get_name()),
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { check_is_view_op, &on_row_ctx };

	ret = session->query_cb(
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

//------------------------------------------------------------------------------

struct st_table_sql_single_result_ctx
{
	zval* result;
};


/* {{{ table_sql_single_result_op_on_row */
const enum_hnd_func_status
table_sql_single_result_op_on_row(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const stmt,
	const XMYSQLND_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	st_table_sql_single_result_ctx* ctx = (st_table_sql_single_result_ctx*) context;
	DBG_ENTER("table_sql_single_result_op_on_row");
	if (ctx && row) {
		ZVAL_COPY_VALUE(ctx->result, &row[0]);
		DBG_RETURN(HND_PASS);
	} else {
		DBG_RETURN(HND_AGAIN);
	}
}
/* }}} */


/* {{{ xmysqlnd_table::count */
enum_func_status
xmysqlnd_table::count(
	struct st_xmysqlnd_session_on_error_bind on_error,
	zval* counter)
{
	DBG_ENTER("xmysqlnd_table::count");
	ZVAL_LONG(counter, 0);

	enum_func_status ret;

	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	char* query_str;
	mnd_sprintf(&query_str, 0, "SELECT COUNT(*) FROM %s.%s", schema->get_name().s, get_name().s);
	if (!query_str) {
		DBG_RETURN(FAIL);
	}
	const MYSQLND_CSTRING query = {query_str, strlen(query_str)};

	st_table_sql_single_result_ctx on_row_ctx = {
		counter
	};

	const st_xmysqlnd_session_on_row_bind on_row = { table_sql_single_result_op_on_row, &on_row_ctx };

	ret = session->query_cb(namespace_sql,
							   query,
							   noop__var_binder,
							   noop__on_result_start,
							   on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	mnd_sprintf_free(query_str);
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_table::insert */
xmysqlnd_stmt *
xmysqlnd_table::insert(XMYSQLND_CRUD_TABLE_OP__INSERT * op)
{
	xmysqlnd_stmt* ret{nullptr};
	DBG_ENTER("xmysqlnd_table::opinsert");
	if (!op || FAIL == xmysqlnd_crud_table_insert__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_insert__is_initialized(op))
	{
		auto session = get_schema()->get_session();
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io,
																							session->data->stats,
																							session->data->error_info);
		struct st_xmysqlnd_msg__table_insert table_insert = msg_factory.get__table_insert(&msg_factory);
		if (PASS == table_insert.send_insert_request(&table_insert, xmysqlnd_crud_table_insert__get_protobuf_message(op)))
		{
			//ret = table_insert.read_response(&table_insert);
			auto session = get_schema()->get_session();
			xmysqlnd_stmt * stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != nullptr ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_table::opdelete */
xmysqlnd_stmt *
xmysqlnd_table::opdelete(XMYSQLND_CRUD_TABLE_OP__DELETE * op)
{
	xmysqlnd_stmt* ret{nullptr};
	DBG_ENTER("xmysqlnd_table::opdelete");
	if (!op || FAIL == xmysqlnd_crud_table_delete__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_delete__is_initialized(op))
	{
		auto session = get_schema()->get_session();
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == table_ud.send_delete_request(&table_ud, xmysqlnd_crud_table_delete__get_protobuf_message(op)))
		{
			//ret = table_ud.read_response(&table_ud);
			auto session = get_schema()->get_session();
			xmysqlnd_stmt * stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != nullptr ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_table::update */
xmysqlnd_stmt *
xmysqlnd_table::update(XMYSQLND_CRUD_TABLE_OP__UPDATE * op)
{
	xmysqlnd_stmt* ret{nullptr};
	DBG_ENTER("xmysqlnd_table::update");
	if (!op || FAIL == xmysqlnd_crud_table_update__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_update__is_initialized(op))
	{
		auto session = get_schema()->get_session();
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == table_ud.send_update_request(&table_ud, xmysqlnd_crud_table_update__get_protobuf_message(op)))
		{
			//ret = table_ud.read_response(&table_ud);
			auto session = get_schema()->get_session();
			xmysqlnd_stmt * stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != nullptr ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_table::select */
xmysqlnd_stmt *
xmysqlnd_table::select(XMYSQLND_CRUD_TABLE_OP__SELECT * op)
{
	xmysqlnd_stmt* stmt{nullptr};
	DBG_ENTER("xmysqlnd_table::select");
	if (!op || FAIL == xmysqlnd_crud_table_select__finalize_bind(op))
	{
		DBG_RETURN(stmt);
	}
	if (xmysqlnd_crud_table_select__is_initialized(op))
	{
		auto session = get_schema()->get_session();
		stmt = session->create_statement_object(session);
		if (FAIL == stmt->send_raw_message(stmt, xmysqlnd_crud_table_select__get_protobuf_message(op),
                session->get_data()->stats, session->get_data()->error_info))
		{
			xmysqlnd_stmt_free(stmt, session->get_data()->stats, session->get_data()->error_info);
			stmt = nullptr;
		}
	}
	DBG_RETURN(stmt);
}
/* }}} */

/* {{{ xmysqlnd_table::get_reference */
xmysqlnd_table *
xmysqlnd_table::get_reference()
{
	DBG_ENTER("xmysqlnd_table::get_reference");
	++refcount;
	DBG_INF_FMT("new_refcount=%u", refcount);
	DBG_RETURN(this);
}
/* }}} */


/* {{{ xmysqlnd_table::free_reference */
enum_func_status
xmysqlnd_table::free_reference(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_table::free_reference");
	DBG_INF_FMT("old_refcount=%u", refcount);
	if (!(--refcount)) {
		cleanup(stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_table::free_contents */
void
xmysqlnd_table::free_contents()
{
	const zend_bool pers = persistent;
	DBG_ENTER("xmysqlnd_table::free_contents");
	if (table_name.s) {
		mnd_pefree(table_name.s, pers);
		table_name.s = nullptr;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_table::dtor */
void
xmysqlnd_table::cleanup(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_table::dtor");
	free_contents();
	xmysqlnd_schema_free(schema, stats, error_info);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ xmysqlnd_table_create */
PHP_MYSQL_XDEVAPI_API xmysqlnd_table *
xmysqlnd_table_create(xmysqlnd_schema * schema,
						   const MYSQLND_CSTRING table_name,
						   const zend_bool persistent,
						   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						   MYSQLND_STATS * const stats,
						   MYSQLND_ERROR_INFO * const error_info)
{
	xmysqlnd_table* ret{nullptr};
	DBG_ENTER("xmysqlnd_table_create");
	if (table_name.s && table_name.l) {
		ret = object_factory->get_table(object_factory, schema, table_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->get_reference();
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_table_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_table_free(xmysqlnd_table * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_table_free");
	if (table) {
		table->free_reference(stats, error_info);
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
