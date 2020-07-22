/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
extern "C" {
#include <ext/json/php_json_parser.h>
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
#include "util/pb_utils.h"

namespace mysqlx {

namespace drv {

xmysqlnd_table::xmysqlnd_table(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const cur_obj_factory,
										   xmysqlnd_schema * const cur_schema,
										   const util::string_view& cur_table_name,
											zend_bool is_persistent)
{
	DBG_ENTER("xmysqlnd_table::st_xmysqlnd_table_data");
	if (!(schema = cur_schema->get_reference())) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::table_creation_failed);
	}
	persistent = is_persistent;
	table_name = cur_table_name;
	DBG_INF_FMT("name=[%d]%*s", table_name.length(), table_name.length(), table_name.c_str());

	object_factory = cur_obj_factory;
}

//------------------------------------------------------------------------------

struct table_or_view_var_binder_ctx
{
	util::string_view schema_name;
	util::string_view table_name;
	unsigned int counter;
};

static const enum_hnd_func_status table_op_var_binder(
	void* context,
	XMYSQLND_SESSION session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	DBG_ENTER("table_op_var_binder");

	table_or_view_var_binder_ctx* ctx = static_cast<table_or_view_var_binder_ctx*>(context);

	Mysqlx::Sql::StmtExecute& stmt_message = xmysqlnd_stmt_execute__get_pb_msg(stmt_execute);

	util::pb::Object* stmt_obj{util::pb::add_object_arg(stmt_message)};

	util::pb::add_field_to_object("schema", ctx->schema_name, stmt_obj);
	util::pb::add_field_to_object("pattern", ctx->table_name, stmt_obj);

	DBG_RETURN(HND_PASS);
}

struct table_or_view_op_ctx
{
	util::string_view expected_name;
	zval* exists;
};


const enum_hnd_func_status
table_or_view_exists_in_database_op(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
{
	table_or_view_op_ctx* ctx = static_cast<table_or_view_op_ctx*>(context);
	DBG_ENTER("table_or_view_exists_in_database_op");
	if (ctx && row) {
		const util::string_view object_name{ Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const util::string_view object_type{ Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if ((object_name == ctx->expected_name)
			&& (is_table_object_type(object_type) || is_view_object_type(object_type)))
		{
			ZVAL_TRUE(ctx->exists);
			DBG_RETURN(HND_PASS);
		}
	}
	DBG_RETURN(HND_AGAIN);
}

enum_func_status
xmysqlnd_table::exists_in_database(
		struct st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_table::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	constexpr util::string_view query = "list_objects";
	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	table_or_view_var_binder_ctx var_binder_ctx = {
		schema->get_name(),
		get_name(),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	table_or_view_op_ctx on_row_ctx = {
		get_name(),
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { table_or_view_exists_in_database_op, &on_row_ctx };

	ret = session->query_cb(
		namespace_mysqlx,
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

//------------------------------------------------------------------------------

const enum_hnd_func_status
check_is_view_op(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
{
	table_or_view_op_ctx* ctx = static_cast<table_or_view_op_ctx*>(context);
	DBG_ENTER("check_is_view_op");
	if (ctx && row) {
		const util::string_view object_name{ Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const util::string_view object_type{ Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if ((object_name == ctx->expected_name) && is_view_object_type(object_type)) {
			ZVAL_TRUE(ctx->exists);
			DBG_RETURN(HND_PASS);
		}
	}
	DBG_RETURN(HND_AGAIN);
}

enum_func_status
xmysqlnd_table::is_view(
	st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_table::is_view");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	constexpr util::string_view query = "list_objects";
	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	table_or_view_var_binder_ctx var_binder_ctx = {
		schema->get_name(),
		get_name(),
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	table_or_view_op_ctx on_row_ctx = {
		get_name(),
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { check_is_view_op, &on_row_ctx };

	ret = session->query_cb(
		namespace_mysqlx,
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

//------------------------------------------------------------------------------

struct st_table_sql_single_result_ctx
{
	zval* result;
};


const enum_hnd_func_status
table_sql_single_result_op_on_row(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
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
	mnd_sprintf(&query_str, 0, "SELECT COUNT(*) FROM %s.%s", schema->get_name().data(), get_name().data());
	if (!query_str) {
		DBG_RETURN(FAIL);
	}
	const util::string_view query{query_str, strlen(query_str)};

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

xmysqlnd_stmt *
xmysqlnd_table::insert(XMYSQLND_CRUD_TABLE_OP__INSERT * op)
{
	DBG_ENTER("xmysqlnd_table::opinsert");
	xmysqlnd_stmt *         stmt{nullptr};
	auto                    session = get_schema()->get_session();
	if (!op) {
		DBG_RETURN(stmt);
	}
	if ( FAIL == xmysqlnd_crud_table_insert__finalize_bind(op)) {
		DBG_RETURN(stmt);
	}
	if (xmysqlnd_crud_table_insert__is_initialized(op)) {
		st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
		struct st_xmysqlnd_msg__table_insert table_insert = msg_factory.get__table_insert(&msg_factory);
		if (PASS == table_insert.send_insert_request(&table_insert, xmysqlnd_crud_table_insert__get_protobuf_message(op)))
		{
			stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
		}
		DBG_INF(stmt != nullptr ? "PASS" : "FAIL");
	}

	DBG_RETURN(stmt);
}

xmysqlnd_stmt *
xmysqlnd_table::opdelete(XMYSQLND_CRUD_TABLE_OP__DELETE * op)
{
	DBG_ENTER("xmysqlnd_table::opdelete");
	xmysqlnd_stmt *         stmt{nullptr};
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data = &session->get_data()->ps_data;
	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() )
	{
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			 FAIL == xmysqlnd_crud_table_delete__finalize_bind(op))
		{
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_table_delete__is_initialized(op))
		{
			st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
			struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
			if (PASS == table_ud.send_delete_request(&table_ud, xmysqlnd_crud_table_delete__get_protobuf_message(op)))
			{
				stmt = session->create_statement_object(session);
				stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			}
			DBG_INF(stmt != nullptr ? "PASS" : "FAIL");
		}
	}
	else
	{
		auto res = ps_data->add_message( op->message, static_cast<uint32_t>(op->bound_values.size()) );

		if (FAIL == xmysqlnd_crud_table_delete__finalize_bind(op)){
			DBG_RETURN(stmt);
		}

		op->ps_message_id = res.second;
		ps_data->set_finalized_bind( res.second, true );

		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return opdelete(op);
				}
				DBG_RETURN(stmt);
			}
		}
		if (!xmysqlnd_crud_table_delete__is_initialized(op))
		{
			DBG_RETURN(stmt);
		}
		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bound_values ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}

	DBG_RETURN(stmt);
}

xmysqlnd_stmt *
xmysqlnd_table::update(XMYSQLND_CRUD_TABLE_OP__UPDATE * op)
{
	DBG_ENTER("xmysqlnd_table::update");
	xmysqlnd_stmt *         stmt{nullptr};
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data = &session->get_data()->ps_data;
	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() ) {
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			 FAIL == xmysqlnd_crud_table_update__finalize_bind(op))
		{
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_table_update__is_initialized(op))
		{
			st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
			struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
			if (PASS == table_ud.send_update_request(&table_ud, xmysqlnd_crud_table_update__get_protobuf_message(op)))
			{
				stmt = session->create_statement_object(session);
				stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			}
			DBG_INF(stmt != nullptr ? "PASS" : "FAIL");
		}
	}
	else
	{
		auto res = ps_data->add_message( op->message,  static_cast<uint32_t>(op->bound_values.size()));
		if (FAIL == xmysqlnd_crud_table_update__finalize_bind(op)){
			DBG_RETURN(stmt);
		}

		op->ps_message_id = res.second;
		ps_data->set_finalized_bind( res.second, true );
		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return update(op);
				}
				DBG_RETURN(stmt);
			}
		}
		if (!xmysqlnd_crud_table_update__is_initialized(op))
		{
			DBG_RETURN(stmt);
		}

		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bound_values ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}

	DBG_RETURN(stmt);
}

xmysqlnd_stmt *
xmysqlnd_table::select(XMYSQLND_CRUD_TABLE_OP__SELECT * op)
{
	DBG_ENTER("xmysqlnd_table::select");
	xmysqlnd_stmt *         stmt{nullptr};
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data = &session->get_data()->ps_data;
	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() )
	{
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			 FAIL == xmysqlnd_crud_table_select__finalize_bind(op))
		{
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_table_select__is_initialized(op))
		{
			stmt = session->create_statement_object(session);
			if (FAIL == stmt->send_raw_message(stmt, xmysqlnd_crud_table_select__get_protobuf_message(op),
					session->get_data()->stats, session->get_data()->error_info))
			{
				xmysqlnd_stmt_free(stmt, session->get_data()->stats, session->get_data()->error_info);
				stmt = nullptr;
			}
		}
	}
	else {
		auto res = ps_data->add_message( op->message,  static_cast<uint32_t>(op->bound_values.size()) );
		if (FAIL == xmysqlnd_crud_table_select__finalize_bind(op))
		{
			DBG_RETURN(stmt);
		}
		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return select(op);
				}
				DBG_RETURN(stmt);
			}
		}
		if (!xmysqlnd_crud_table_select__is_initialized(op))
		{
			DBG_RETURN(stmt);
		}

		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bound_values ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}
	DBG_RETURN(stmt);
}

xmysqlnd_table *
xmysqlnd_table::get_reference()
{
	DBG_ENTER("xmysqlnd_table::get_reference");
	++refcount;
	DBG_INF_FMT("new_refcount=%u", refcount);
	DBG_RETURN(this);
}

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

void
xmysqlnd_table::free_contents()
{
	DBG_ENTER("xmysqlnd_table::free_contents");
	table_name.clear();
	DBG_VOID_RETURN;
}

void
xmysqlnd_table::cleanup(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_table::dtor");
	free_contents();
	xmysqlnd_schema_free(schema, stats, error_info);

	DBG_VOID_RETURN;
}

PHP_MYSQL_XDEVAPI_API xmysqlnd_table *
xmysqlnd_table_create(xmysqlnd_schema * schema,
						   const util::string_view& table_name,
						   const zend_bool persistent,
						   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						   MYSQLND_STATS * const stats,
						   MYSQLND_ERROR_INFO * const error_info)
{
	xmysqlnd_table* ret{nullptr};
	DBG_ENTER("xmysqlnd_table_create");
	if (!table_name.empty()) {
		ret = object_factory->get_table(object_factory, schema, table_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->get_reference();
		}
	}
	DBG_RETURN(ret);
}

PHP_MYSQL_XDEVAPI_API void
xmysqlnd_table_free(xmysqlnd_table * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_table_free");
	if (table) {
		table->free_reference(stats, error_info);
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
