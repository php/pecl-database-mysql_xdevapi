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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
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
#include "xmysqlnd_collection.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_utils.h"
#include "mysqlx_exception.h"
#include "util/exceptions.h"
#include "util/pb_utils.h"
#include "xmysqlnd_extension_plugin.h"

namespace mysqlx {

namespace drv {

xmysqlnd_collection::xmysqlnd_collection(
								xmysqlnd_schema * const cur_schema,
								const util::string_view& cur_collection_name,
								zend_bool is_persistent)
{
	DBG_ENTER("xmysqlnd_collection::xmysqlnd_collection");
	if (!(schema = cur_schema->get_reference())) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::schema_creation_failed);
	}
	persistent = is_persistent;
	collection_name = cur_collection_name;
	DBG_INF_FMT("name=[%d]%*s", collection_name.length(), collection_name.length(), collection_name.data());

}

struct st_collection_exists_in_database_var_binder_ctx
{
	util::string_view schema_name;
	util::string_view collection_name;
	unsigned int counter;
};

static const enum_hnd_func_status collection_op_var_binder(
	void* context,
	XMYSQLND_SESSION session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	DBG_ENTER("collection_op_var_binder");

	st_collection_exists_in_database_var_binder_ctx* ctx
		= static_cast<st_collection_exists_in_database_var_binder_ctx*>(context);

	Mysqlx::Sql::StmtExecute& stmt_message = xmysqlnd_stmt_execute__get_pb_msg(stmt_execute);

	util::pb::Object* stmt_obj{util::pb::add_object_arg(stmt_message)};

	util::pb::add_field_to_object("schema", ctx->schema_name, stmt_obj);
	util::pb::add_field_to_object("pattern", ctx->collection_name, stmt_obj);

	DBG_RETURN(HND_PASS);
}

struct collection_exists_in_database_ctx
{
	util::string_view expected_collection_name;
	zval* exists;
};


static const enum_hnd_func_status
collection_mysqlx_op_on_row(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
{
	collection_exists_in_database_ctx* ctx = static_cast<collection_exists_in_database_ctx*>(context);
	DBG_ENTER("collection_mysqlx_op_on_row");
	if (ctx && row) {
		const util::string_view object_name( Z_STRVAL(row[0]), Z_STRLEN(row[0]) );
		const util::string_view object_type( Z_STRVAL(row[1]), Z_STRLEN(row[1]) );

		if ((object_name == ctx->expected_collection_name)
			&& is_collection_object_type(object_type))
		{
			ZVAL_TRUE(ctx->exists);
		}
	}
	DBG_RETURN(HND_AGAIN);
}

enum_func_status
xmysqlnd_collection::exists_in_database(
	struct st_xmysqlnd_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_collection::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	constexpr util::string_view query = "list_objects";

	st_collection_exists_in_database_var_binder_ctx var_binder_ctx = {
		schema->get_name(),
		collection_name,
		0
	};
	const st_xmysqlnd_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	collection_exists_in_database_ctx on_row_ctx = {
		collection_name,
		exists
	};

	const st_xmysqlnd_session_on_row_bind on_row = { collection_mysqlx_op_on_row, &on_row_ctx };

	ret = schema->get_session()->query_cb(namespace_mysqlx,
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

struct st_collection_sql_single_result_ctx
{
	zval* result;
};


static const enum_hnd_func_status
collection_sql_single_result_op_on_row(
	void * context,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const XMYSQLND_STMT_RESULT_META * const /*meta*/,
	const zval * const row,
	MYSQLND_STATS * const /*stats*/,
	MYSQLND_ERROR_INFO * const /*error_info*/)
{
	st_collection_sql_single_result_ctx* ctx = (st_collection_sql_single_result_ctx*) context;
	DBG_ENTER("collection_sql_single_result_op_on_row");
	if (ctx && row) {
		ZVAL_COPY_VALUE(ctx->result, &row[0]);
	}
	DBG_RETURN(HND_AGAIN);
}

enum_func_status
xmysqlnd_collection::count(
	struct st_xmysqlnd_session_on_error_bind on_error,
	zval* counter)
{
	DBG_ENTER("xmysqlnd_collection::count");
	ZVAL_LONG(counter, 0);

	enum_func_status ret;

	xmysqlnd_schema * schema = get_schema();
	auto session = schema->get_session();

	char* query_str{nullptr};
	mnd_sprintf(&query_str, 0, "SELECT COUNT(*) FROM %s.%s", schema->get_name().data(), get_name().data());
	if (!query_str) {
		DBG_RETURN(FAIL);
	}
	const util::string_view query{query_str};

 	st_collection_sql_single_result_ctx on_row_ctx = {
		counter
	};

	const st_xmysqlnd_session_on_row_bind on_row = { collection_sql_single_result_op_on_row, &on_row_ctx };

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
xmysqlnd_collection::add(XMYSQLND_CRUD_COLLECTION_OP__ADD * crud_op)
{
	DBG_ENTER("xmysqlnd_collection::add");
	xmysqlnd_stmt*                         ret{nullptr};
	XMYSQLND_SESSION                       session{ get_schema()->get_session() };
	if( xmysqlnd_crud_collection_add__finalize_bind(crud_op) == PASS ) {
		st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
		st_xmysqlnd_msg__collection_add collection_add = msg_factory.get__collection_add(&msg_factory);
		enum_func_status request_ret = collection_add.send_request(&collection_add,
											xmysqlnd_crud_collection_add__get_protobuf_message(crud_op));
		if (PASS == request_ret) {
			xmysqlnd_stmt * stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
	} else {
		devapi::RAISE_EXCEPTION(err_msg_add_doc);
	}

	DBG_RETURN(ret);
}

xmysqlnd_stmt *
xmysqlnd_collection::remove(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * op)
{
	DBG_ENTER("xmysqlnd_collection::remove");
	xmysqlnd_stmt *         stmt{nullptr};
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data{ &session->get_data()->ps_data };
	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() ) {
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			 FAIL == xmysqlnd_crud_collection_remove__finalize_bind(op)) {
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_collection_remove__is_initialized(op)) {
			st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
			struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
			if (PASS == collection_ud.send_delete_request(&collection_ud, xmysqlnd_crud_collection_remove__get_protobuf_message(op))) {
				stmt = session->create_statement_object(session);
				stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			}
			DBG_INF(stmt != nullptr? "PASS":"FAIL");
		}
	}else{
		auto res = ps_data->add_message( op->message, static_cast<uint32_t>(op->bindings.size()));
		if (!op || FAIL == xmysqlnd_crud_collection_remove__finalize_bind(op)) {
			DBG_RETURN(stmt);
		}

		op->ps_message_id = res.second;
		ps_data->set_finalized_bind( res.second, true );

		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return remove(op);
				}
				DBG_RETURN(stmt);
			}
		}

		if (!xmysqlnd_crud_collection_remove__is_initialized(op)) {
			DBG_RETURN(stmt);
		}

		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bindings.get_bound_values() ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}

	DBG_RETURN(stmt);
}

xmysqlnd_stmt *
xmysqlnd_collection::modify(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * op)
{
	DBG_ENTER("xmysqlnd_collection::modify");
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data = &session->get_data()->ps_data;
	xmysqlnd_stmt *         stmt{nullptr};

	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() ) {
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			 !xmysqlnd_crud_collection_modify__finalize_bind(op)) {
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_collection_modify__is_initialized(op)) {
			st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
			struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
			if (PASS == collection_ud.send_update_request(&collection_ud, xmysqlnd_crud_collection_modify__get_protobuf_message(op))) {
				stmt = session->create_statement_object(session);
				stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			}
		}
		DBG_INF(stmt != nullptr? "PASS":"FAIL");
	} else {
		auto res = ps_data->add_message( op->message, static_cast<uint32_t>(op->bindings.size()) );
		if (!xmysqlnd_crud_collection_modify__finalize_bind(op)) {
			DBG_RETURN(stmt);
		}
		op->ps_message_id = res.second;
		ps_data->set_finalized_bind( res.second, true );

		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return modify(op);
				}
				DBG_RETURN(stmt);
			}
		}

		if ( !xmysqlnd_crud_collection_modify__is_initialized(op) ) {
			DBG_RETURN(stmt);
		}

		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bindings.get_bound_values() ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}

	DBG_RETURN(stmt);
}

xmysqlnd_stmt*
xmysqlnd_collection::find(XMYSQLND_CRUD_COLLECTION_OP__FIND * op)
{
	xmysqlnd_stmt*          stmt{nullptr};
	DBG_ENTER("xmysqlnd_collection::find");
	auto                    session = get_schema()->get_session();
	drv::Prepare_stmt_data* ps_data = &session->get_data()->ps_data;
	if (!op) {
		DBG_RETURN(stmt);
	}
	if( false == ps_data->is_ps_supported() ) {
		if ( !ps_data->is_bind_finalized( op->ps_message_id ) &&
			  FAIL == xmysqlnd_crud_collection_find__finalize_bind(op)) {
			DBG_RETURN(stmt);
		}
		if (xmysqlnd_crud_collection_find__is_initialized(op)) {
			auto session = get_schema()->get_session();
			stmt = session->create_statement_object(session);
			if (FAIL == stmt->send_raw_message(stmt,
											   xmysqlnd_crud_collection_find__get_protobuf_message(op),
											   session->data->stats, session->data->error_info)) {
				xmysqlnd_stmt_free(stmt, session->data->stats, session->data->error_info);
				stmt = nullptr;
			}
		}
	} else {
		auto res = ps_data->add_message( op->message, static_cast<uint32_t>(op->bindings.size()) );
		if ( FAIL == xmysqlnd_crud_collection_find__finalize_bind(op) ) {
			DBG_RETURN(stmt);
		}
		op->ps_message_id = res.second;
		ps_data->set_finalized_bind( res.second, true );
		if( res.first ) {
			if( !ps_data->send_prepare_msg( res.second ) ) {
				if( ps_data->is_ps_supported() == false ) {
					return find(op);
				}
				DBG_RETURN(stmt);
			}
		}
		if( ps_data->prepare_msg_delivered( res.second ) &&
			ps_data->bind_values( res.second, op->bindings.get_bound_values() ) ) {
			stmt = ps_data->send_execute_msg( res.second );
		}
	}
	DBG_RETURN(stmt);
}

xmysqlnd_collection *
xmysqlnd_collection::get_reference()
{
	DBG_ENTER("xmysqlnd_collection::get_reference");
	++refcount;
	DBG_INF_FMT("new_refcount=%u", refcount);
	DBG_RETURN(this);
}

enum_func_status
xmysqlnd_collection::free_reference(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_collection::free_reference");
	DBG_INF_FMT("old_refcount=%u", refcount);
	if (!(--refcount)) {
		cleanup(stats, error_info);
	}
	DBG_RETURN(ret);
}

void
xmysqlnd_collection::free_contents()
{
	DBG_ENTER("xmysqlnd_collection::free_contents");
	collection_name.clear();
	DBG_VOID_RETURN;
}

void
xmysqlnd_collection::cleanup(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_collection::cleanup");
	free_contents();
	xmysqlnd_schema_free(schema, stats, error_info);

	DBG_VOID_RETURN;
}

PHP_MYSQL_XDEVAPI_API xmysqlnd_collection *
xmysqlnd_collection_create(xmysqlnd_schema * schema,
								const util::string_view& collection_name,
								const zend_bool persistent,
								const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								MYSQLND_STATS * const stats,
								MYSQLND_ERROR_INFO * const error_info)
{
	xmysqlnd_collection* ret{nullptr};
	DBG_ENTER("xmysqlnd_collection_create");
	if (!collection_name.empty()) {
		ret = object_factory->get_collection(object_factory, schema, collection_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->get_reference();
		}
	}
	DBG_RETURN(ret);
}

PHP_MYSQL_XDEVAPI_API void
xmysqlnd_collection_free(xmysqlnd_collection * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_collection_free");
	DBG_INF_FMT("collection=%p",
				collection);
	if (collection) {
		collection->free_reference(stats, error_info);
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
