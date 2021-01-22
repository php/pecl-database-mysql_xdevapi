/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_execution_status.h"
#include "mysqlx_exception.h"
#include "mysqlx_result.h"
#include "mysqlx_column_result.h"
#include "mysqlx_doc_result.h"
#include "mysqlx_row_result.h"
#include "mysqlx_sql_statement_result.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_session.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/functions.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_sql_statement_class_entry;
static zend_class_entry * mysqlx_statement_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, param)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement__execute, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, flags, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement__has_more_results, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement__get_result, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

st_mysqlx_statement::~st_mysqlx_statement()
{
	xmysqlnd_stmt_free(stmt, nullptr, nullptr);
	xmysqlnd_stmt_execute__destroy(stmt_execute);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

struct st_xmysqlnd_exec_with_cb_ctx
{
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fci_cache;
	} on_row;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fci_cache;
	} on_warning;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fci_cache;
	} on_error;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fci_cache;
	} on_rset_end;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fci_cache;
	} on_stmt_ok;
	zval* ctx{nullptr};
};

static const enum_hnd_func_status
exec_with_cb_handle_on_row(void * context,
						   xmysqlnd_stmt * const /*stmt*/,
						   const st_xmysqlnd_stmt_result_meta* const meta,
						   const zval * const row,
						   MYSQLND_STATS * const /*stats*/,
						   MYSQLND_ERROR_INFO * const /*error_info*/)
{
	enum_hnd_func_status ret{HND_AGAIN};
	st_xmysqlnd_exec_with_cb_ctx* ctx = (st_xmysqlnd_exec_with_cb_ctx*) context;
	DBG_ENTER("exec_with_cb_handle_on_row");
	if (ctx && row) {
		zval params[3];
		zval * user_context = &params[0];
		zval * row_container = &params[1];
		zval * meta_container = &params[2];
		zval return_value;
		const unsigned int col_count{meta->m->get_field_count(meta)};

		array_init_size(row_container, col_count);
		array_init_size(meta_container, col_count);
		for (unsigned int i{0}; i < col_count; ++i) {
			const XMYSQLND_RESULT_FIELD_META * field_meta = meta->m->get_field(meta, i);
			util::zvalue meta_field_obj = create_column_result(meta->m->get_field(meta, i));
			zend_hash_next_index_insert(Z_ARRVAL_P(meta_container), meta_field_obj.ptr());
			meta_field_obj.invalidate();

			if (field_meta->zend_hash_key.is_numeric == FALSE) {
				zend_hash_update(Z_ARRVAL_P(row_container), field_meta->zend_hash_key.sname, (zval *) &row[i]);
			} else {
				zend_hash_index_update(Z_ARRVAL_P(row_container), field_meta->zend_hash_key.key, (zval *) &row[i]);
			}
		}
		util::zvalue::copy_from_to(ctx->ctx, user_context);

		ZVAL_UNDEF(&return_value);
		ctx->on_row.fci.retval = &return_value;
		ctx->on_row.fci.params = params;
		ctx->on_row.fci.param_count = 3;

		if (zend_call_function(&ctx->on_row.fci, &ctx->on_row.fci_cache) == SUCCESS) {
			if (Z_TYPE(return_value) != IS_UNDEF) {
				if (Z_TYPE(return_value) == IS_LONG) {
					DBG_INF_FMT("retval=%ld", Z_LVAL(return_value));
					switch (Z_LVAL(return_value)) {
						case HND_PASS:
						case HND_FAIL:
						case HND_PASS_RETURN_FAIL:
						case HND_AGAIN:
							ret = static_cast<enum_hnd_func_status>(Z_LVAL(return_value));
							break;
						default:
							break;
					}
				}
				zval_ptr_dtor(&return_value);
			}
		} else {
			ret = HND_FAIL;
		}
		zval_ptr_dtor(&params[0]);
		zval_ptr_dtor(&params[1]);
		zval_ptr_dtor(&params[2]);
	}
#ifdef PHP_DEBUG
	DBG_INF_FMT("ret=%d", ret);
	switch (ret) {
		case HND_PASS:
			DBG_INF("HND_PASS");
			break;
		case HND_FAIL:
			DBG_INF("HND_FAIL");
			break;
		case HND_PASS_RETURN_FAIL:
			DBG_INF("HND_PASS_RETURN_FAIL");
			break;
		case HND_AGAIN:
			DBG_INF("HND_AGAIN");
			break;
		default:
			break;
	}
#endif
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
exec_with_cb_handle_on_warning(
	void * context,
	xmysqlnd_stmt * const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int code,
	const util::string_view& message)
{
	enum_hnd_func_status ret{HND_AGAIN};
	st_xmysqlnd_exec_with_cb_ctx* ctx = (st_xmysqlnd_exec_with_cb_ctx*) context;
	DBG_ENTER("exec_with_cb_handle_on_warning");
	if (ctx) {
		zval params[3];
		zval * user_context = &params[0];
		zval * code_zv = &params[1];
		zval * message_zv = &params[2];
		zval return_value;

		ZVAL_LONG(code_zv, code);
		ZVAL_STRINGL(message_zv, message.data(), message.length());

		util::zvalue::copy_from_to(ctx->ctx, user_context);

		ZVAL_UNDEF(&return_value);
		ctx->on_warning.fci.retval = &return_value;
		ctx->on_warning.fci.params = params;
		ctx->on_warning.fci.param_count = 3;

		if (zend_call_function(&ctx->on_warning.fci, &ctx->on_warning.fci_cache) == SUCCESS) {
			if (Z_TYPE(return_value) != IS_UNDEF) {
				zval_ptr_dtor(&return_value);
			}
		} else {
			ret = HND_FAIL;
		}
		zval_ptr_dtor(&params[0]);
		zval_ptr_dtor(&params[1]);
		zval_ptr_dtor(&params[2]);
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
exec_with_cb_handle_on_error(
	void * context,
	xmysqlnd_stmt * const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	enum_hnd_func_status ret{HND_PASS_RETURN_FAIL};
	st_xmysqlnd_exec_with_cb_ctx* ctx = (st_xmysqlnd_exec_with_cb_ctx*) context;
	DBG_ENTER("exec_with_cb_handle_on_error");
	if (ctx) {
		zval params[4];
		zval * user_context = &params[0];
		zval * code_zv = &params[1];
		zval * sql_state_zv = &params[2];
		zval * message_zv = &params[3];
		zval return_value;

		ZVAL_LONG(code_zv, code);
		ZVAL_STRINGL(sql_state_zv, sql_state.data(), sql_state.length());
		ZVAL_STRINGL(message_zv, message.data(), message.length());

		util::zvalue::copy_from_to(ctx->ctx, user_context);

		ZVAL_UNDEF(&return_value);
		ctx->on_error.fci.retval = &return_value;
		ctx->on_error.fci.params = params;
		ctx->on_error.fci.param_count = 4;

		if (zend_call_function(&ctx->on_error.fci, &ctx->on_error.fci_cache) == SUCCESS) {
			if (Z_TYPE(return_value) != IS_UNDEF) {
				zval_ptr_dtor(&return_value);
			}
		} else {
			ret = HND_FAIL;
		}
		zval_ptr_dtor(&params[0]);
		zval_ptr_dtor(&params[1]);
		zval_ptr_dtor(&params[2]);
		zval_ptr_dtor(&params[3]);
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
exec_with_cb_handle_on_resultset_end(void * context, xmysqlnd_stmt * const /*stmt*/, const zend_bool has_more)
{
	enum_hnd_func_status ret{HND_PASS_RETURN_FAIL};
	st_xmysqlnd_exec_with_cb_ctx* ctx = (st_xmysqlnd_exec_with_cb_ctx*) context;
	DBG_ENTER("exec_with_cb_handle_on_resultset_end");
	if (ctx) {
		zval params[2];
		zval * user_context = &params[0];
		zval * has_more_zv = &params[1];
		zval return_value;

		util::zvalue::copy_from_to(ctx->ctx, user_context);
		ZVAL_BOOL(has_more_zv, has_more);

		/* Add statement status here and pass it to the function */
		ZVAL_UNDEF(&return_value);
		ctx->on_rset_end.fci.retval = &return_value;
		ctx->on_rset_end.fci.params = params;
		ctx->on_rset_end.fci.param_count = 2;

		if (zend_call_function(&ctx->on_rset_end.fci, &ctx->on_rset_end.fci_cache) == SUCCESS) {
			if (Z_TYPE(return_value) != IS_UNDEF) {
				zval_ptr_dtor(&return_value);
			}
		} else {
			ret = HND_FAIL;
		}
		zval_ptr_dtor(&params[0]);
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
exec_with_cb_handle_on_statement_ok(void * context,
									xmysqlnd_stmt * const /*stmt*/,
									const st_xmysqlnd_stmt_execution_state* const exec_state)
{
	enum_hnd_func_status ret{HND_PASS};
	st_xmysqlnd_exec_with_cb_ctx* ctx = (st_xmysqlnd_exec_with_cb_ctx*) context;
	DBG_ENTER("exec_with_cb_handle_on_statement_ok");
	if (ctx) {
		zval params[2];
		zval* user_context = &params[0];
		zval* exec_status_zv = &params[1];
		zval return_value;

		util::zvalue::copy_from_to(ctx->ctx, user_context);

		ZVAL_UNDEF(exec_status_zv);
		create_execution_status(exec_state).move_to(exec_status_zv);
		if (Z_TYPE_P(exec_status_zv) != IS_UNDEF) {

			ZVAL_UNDEF(&return_value);
			ctx->on_stmt_ok.fci.retval = &return_value;
			ctx->on_stmt_ok.fci.params = params;
			ctx->on_stmt_ok.fci.param_count = 1;

			if (zend_call_function(&ctx->on_stmt_ok.fci, &ctx->on_stmt_ok.fci_cache) == SUCCESS) {
				if (Z_TYPE(return_value) != IS_UNDEF) {
					zval_ptr_dtor(&return_value);
				}
			} else {
				ret = HND_FAIL;
			}
		}
		zval_ptr_dtor(&params[0]);
		zval_ptr_dtor(&params[1]);
	}
	DBG_RETURN(ret);
}

#if 0
#ifndef FAST_ZPP
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f*", &fci, &fci_cache, &fci.params, &fci.param_count) == FAILURE) {
		return;
	}
#else
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_FUNC(fci, fci_cache)
		Z_PARAM_VARIADIC('*', fci.params, fci.param_count)
	ZEND_PARSE_PARAMETERS_END();
#endif

#endif


static enum_func_status
mysqlx_fetch_data_with_callback(st_mysqlx_statement* object, st_xmysqlnd_exec_with_cb_ctx* xmysqlnd_exec_with_cb_ctx)
{
	enum_func_status ret;
	zend_bool has_more_results{FALSE};
	xmysqlnd_stmt * stmt = object->stmt;
	const zend_bool on_rset_end_passed = ZEND_FCI_INITIALIZED(xmysqlnd_exec_with_cb_ctx->on_rset_end.fci);
	const zend_bool on_stmt_ok_passed = ZEND_FCI_INITIALIZED(xmysqlnd_exec_with_cb_ctx->on_stmt_ok.fci);

	const st_xmysqlnd_stmt_on_row_bind on_row = { exec_with_cb_handle_on_row, xmysqlnd_exec_with_cb_ctx };
	const st_xmysqlnd_stmt_on_warning_bind on_warning = { exec_with_cb_handle_on_warning, xmysqlnd_exec_with_cb_ctx };
	const st_xmysqlnd_stmt_on_error_bind on_error = { exec_with_cb_handle_on_error, xmysqlnd_exec_with_cb_ctx };
	const st_xmysqlnd_stmt_on_result_end_bind on_resultset_end = { on_rset_end_passed? exec_with_cb_handle_on_resultset_end : nullptr, xmysqlnd_exec_with_cb_ctx };
	const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok = { on_stmt_ok_passed? exec_with_cb_handle_on_statement_ok : nullptr, xmysqlnd_exec_with_cb_ctx };

	DBG_ENTER("mysqlx_fetch_data_with_callback");

	xmysqlnd_exec_with_cb_ctx->on_error.fci.params = xmysqlnd_exec_with_cb_ctx->on_row.fci.params;
	xmysqlnd_exec_with_cb_ctx->on_error.fci.param_count = xmysqlnd_exec_with_cb_ctx->on_row.fci.param_count;

	ret = stmt->read_one_result(stmt,
										on_row,
										on_warning,
										on_error,
										on_resultset_end,
										on_statement_ok,
										&has_more_results,
										nullptr,
										nullptr);
	object->has_more_results = has_more_results;
	object->in_execution = has_more_results;
	DBG_RETURN(ret);
}

bool
mysqlx_sql_statement_bind_one_param(util::raw_zval* object_zv, const util::zvalue& param)
{
	DBG_ENTER("mysqlx_sql_statement_bind_one_param");

	auto& data_object{ util::fetch_data_object<st_mysqlx_statement>(object_zv) };

	bool result = true;
	if (TRUE == data_object.in_execution) {
		php_error_docref(nullptr, E_WARNING, "Statement in execution. Please fetch all data first.");
		result = false;
	} else if (data_object.stmt_execute && FAIL == xmysqlnd_stmt_execute__bind_one_param_add(data_object.stmt_execute, param)) {
		result = false;
	}
	DBG_RETURN(result);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, bind)
{
	util::raw_zval* object_zv{nullptr};
	zval* param_zv{nullptr};

	DBG_ENTER("mysqlx_sql_statement::bind");
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oz",
												&object_zv, mysqlx_sql_statement_class_entry,
												&param_zv))
	{
		DBG_VOID_RETURN;
	}
	if (mysqlx_sql_statement_bind_one_param(object_zv, param_zv)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

static const enum_hnd_func_status
mysqlx_sql_stmt_on_warning(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int /*code*/,
	const util::string_view& /*message*/)
{
	DBG_ENTER("mysqlx_sql_stmt_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.length(), message.data());
	DBG_RETURN(HND_AGAIN);
}

static const enum_hnd_func_status
mysqlx_sql_stmt_on_error(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_sql_stmt_on_error");
	create_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

util::zvalue
mysqlx_sql_statement_get_results(st_mysqlx_statement* object)
{
	DBG_ENTER("mysqlx_sql_statement_get_results");
	util::zvalue result;
	xmysqlnd_stmt * stmt = object->stmt;
	if (PASS == object->send_query_status) {
		if (object->execute_flags & MYSQLX_EXECUTE_FLAG_ASYNC) {
			DBG_INF("ASYNC");
			result = (PASS == object->send_query_status);
		} else {
			const st_xmysqlnd_stmt_on_warning_bind on_warning = { mysqlx_sql_stmt_on_warning, nullptr };
			const st_xmysqlnd_stmt_on_error_bind on_error = { mysqlx_sql_stmt_on_error, nullptr };
			XMYSQLND_STMT_RESULT* stmt_result;
			if (object->execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
				stmt_result = stmt->get_buffered_result(stmt, &object->has_more_results, on_warning, on_error, nullptr, nullptr);
			} else {
				stmt_result = stmt->get_fwd_result(stmt, MYSQLX_EXECUTE_FWD_PREFETCH_COUNT, &object->has_more_rows_in_set, &object->has_more_results, on_warning, on_error, nullptr, nullptr);
			}

			DBG_INF_FMT("has_more_results=%s   has_more_rows_in_set=%s",
						object->has_more_results? "TRUE":"FALSE",
						object->has_more_rows_in_set? "TRUE":"FALSE");

			if (stmt_result) {
				result = create_sql_stmt_result(stmt_result, object);
			} else {
				RAISE_EXCEPTION_FETCH_FAIL();
				/* Or we should close the connection, rendering it unusable at this point ?*/
				object->send_query_status = FAIL;
			}
		}
	}
	DBG_RETURN(result);
}

util::zvalue
mysqlx_sql_statement_execute(
	const st_mysqlx_object* const mysqlx_object,
	const zend_long flags)
{
	st_mysqlx_statement* object = (st_mysqlx_statement*) mysqlx_object->ptr;
	DBG_ENTER("mysqlx_sql_statement_execute");

	util::zvalue result;

	if (!object || !object->stmt_execute) {
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		DBG_ERR_FMT("invalid object of class %s on %s::%d", ZSTR_VAL(mysqlx_object->zo.ce->name), __FILE__, __LINE__);
		DBG_RETURN(result);
	}

	if ((flags | MYSQLX_EXECUTE_ALL_FLAGS) != MYSQLX_EXECUTE_ALL_FLAGS) {
		util::ostringstream os;
		os << "Invalid flags. Unknown " << (flags - (flags | MYSQLX_EXECUTE_ALL_FLAGS));
		php_error_docref(nullptr, E_WARNING, "%s", os.str().c_str());
		DBG_RETURN(result);
	}
	DBG_INF_FMT("flags=%lu", flags);
	DBG_INF_FMT("%sSYNC", (flags & MYSQLX_EXECUTE_FLAG_ASYNC)? "A":"");
	DBG_INF_FMT("%s", (flags & MYSQLX_EXECUTE_FLAG_BUFFERED)? "BUFFERED":"FWD");

	if (TRUE == object->in_execution) {
		php_error_docref(nullptr, E_WARNING, "Statement in execution. Please fetch all data first.");
	} else {
		if (PASS == xmysqlnd_stmt_execute__finalize_bind(object->stmt_execute)) {
			xmysqlnd_stmt * stmt = object->stmt;
			object->execute_flags = flags;
			object->has_more_rows_in_set = FALSE;
			object->has_more_results = FALSE;
			object->send_query_status = stmt->send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(object->stmt_execute), nullptr, nullptr);

			result = mysqlx_sql_statement_get_results(object);
        }
	}
	DBG_RETURN(result);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, execute)
{
	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};
	util::raw_zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_sql_statement::execute");
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
												&object_zv, mysqlx_sql_statement_class_entry,
												&flags))
	{
		DBG_VOID_RETURN;
	}

	mysqlx_sql_statement_execute(Z_MYSQLX_P(object_zv), flags).move_to(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, hasMoreResults)
{
	DBG_ENTER("mysqlx_sql_statement::hasMoreResults");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_statement>(object_zv) };

	RETVAL_BOOL(data_object.stmt->has_more_results(data_object.stmt));
	DBG_INF_FMT("%s", Z_TYPE_P(return_value) == IS_TRUE? "YES":"NO");

	DBG_VOID_RETURN;
}

static void mysqlx_sql_statement_read_result(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry * const class_entry)
{
	DBG_ENTER("mysqlx_sql_statement_read_result");

	util::raw_zval* object_zv{nullptr};
	zend_bool use_callbacks{FALSE};
	st_xmysqlnd_exec_with_cb_ctx xmysqlnd_exec_with_cb_ctx{};
	if (ZEND_NUM_ARGS() == 0) {
		if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
													&object_zv, class_entry))
		{
			DBG_VOID_RETURN;
		}
	} else {
		if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Offff!f!z",
													&object_zv, class_entry,
													&xmysqlnd_exec_with_cb_ctx.on_row.fci, &xmysqlnd_exec_with_cb_ctx.on_row.fci_cache,
													&xmysqlnd_exec_with_cb_ctx.on_warning.fci, &xmysqlnd_exec_with_cb_ctx.on_warning.fci_cache,
													&xmysqlnd_exec_with_cb_ctx.on_error.fci, &xmysqlnd_exec_with_cb_ctx.on_error.fci_cache,
													&xmysqlnd_exec_with_cb_ctx.on_rset_end.fci, &xmysqlnd_exec_with_cb_ctx.on_rset_end.fci_cache,
													&xmysqlnd_exec_with_cb_ctx.on_stmt_ok.fci, &xmysqlnd_exec_with_cb_ctx.on_stmt_ok.fci_cache,
													&xmysqlnd_exec_with_cb_ctx.ctx))
		{
			DBG_VOID_RETURN;
		}
		use_callbacks = TRUE;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_statement>(object_zv) };

	RETVAL_FALSE;
	if (PASS == data_object.send_query_status) {
		xmysqlnd_stmt * stmt = data_object.stmt;

		if (use_callbacks) {
			RETVAL_BOOL(PASS == mysqlx_fetch_data_with_callback(&data_object, &xmysqlnd_exec_with_cb_ctx));
		} else {
			const st_xmysqlnd_stmt_on_warning_bind on_warning = { mysqlx_sql_stmt_on_warning, nullptr };
			const st_xmysqlnd_stmt_on_error_bind on_error = { mysqlx_sql_stmt_on_error, nullptr };
			XMYSQLND_STMT_RESULT * result;

			if (data_object.execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
				result = data_object.stmt->get_buffered_result(stmt, &data_object.has_more_results, on_warning, on_error, nullptr, nullptr);
			} else {
				result = data_object.stmt->get_fwd_result(stmt, MYSQLX_EXECUTE_FWD_PREFETCH_COUNT, &data_object.has_more_rows_in_set, &data_object.has_more_results, on_warning, on_error, nullptr, nullptr);
			}

			DBG_INF_FMT("result=%p  has_more_results=%s", result, data_object.has_more_results? "TRUE":"FALSE");
			if (result) {
				create_sql_stmt_result(result, &data_object).move_to(return_value);
			} else {
				RAISE_EXCEPTION_FETCH_FAIL();
				/* Or we should close the connection, rendering it unusable at this point ?*/
				data_object.send_query_status = FAIL;
			}
		}
	}

	DBG_VOID_RETURN;
}

/*     proto mixed mysqlx_sql_statement::readResult(object statement, callable on_row_cb, callable on_error_cb, callable on_rset_end, callable on_stmt_ok[, mixed cb_param]]) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, getResult)
{
	mysqlx_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, mysqlx_sql_statement_class_entry);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement, getNextResult)
{
	mysqlx_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, mysqlx_sql_statement_class_entry);
}

static const zend_function_entry mysqlx_sql_statement_methods[] = {
	PHP_ME(mysqlx_sql_statement, __construct, arginfo_mysqlx_sql_statement__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_sql_statement, bind,				arginfo_mysqlx_sql_statement__bind,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement, execute,			arginfo_mysqlx_sql_statement__execute,													ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement, hasMoreResults,	arginfo_mysqlx_sql_statement__has_more_results,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement, getResult,		arginfo_mysqlx_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement, getNextResult,	arginfo_mysqlx_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_sql_statement_handlers;
static HashTable mysqlx_sql_statement_properties;

const st_mysqlx_property_entry mysqlx_sql_statement_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};


static void
mysqlx_sql_statement_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_statement>(object);
}

static zend_object *
php_mysqlx_sql_statement_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_sql_statement_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_statement>(
		class_type,
		&mysqlx_object_sql_statement_handlers,
		&mysqlx_sql_statement_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_sql_statement_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_sql_statement_class_entry,
		"SqlStatement",
		mysqlx_std_object_handlers,
		mysqlx_object_sql_statement_handlers,
		php_mysqlx_sql_statement_object_allocator,
		mysqlx_sql_statement_free_storage,
		mysqlx_sql_statement_methods,
		mysqlx_sql_statement_properties,
		mysqlx_sql_statement_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_sql_statement_class_entry, "statement",	sizeof("statement") - 1, ZEND_ACC_PUBLIC);

	zend_declare_class_constant_long(mysqlx_sql_statement_class_entry, "EXECUTE_ASYNC", sizeof("EXECUTE_ASYNC") - 1, MYSQLX_EXECUTE_FLAG_ASYNC);
	zend_declare_class_constant_long(mysqlx_sql_statement_class_entry, "BUFFERED", sizeof("BUFFERED") - 1, MYSQLX_EXECUTE_FLAG_BUFFERED);
}

void
mysqlx_unregister_sql_statement_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_sql_statement_properties);
}

util::zvalue
create_sql_stmt(xmysqlnd_stmt* stmt, const std::string_view& namespace_, const util::string_view& query)
{
	DBG_ENTER("create_sql_stmt");

	util::zvalue sql_stmt_obj;
	st_mysqlx_statement& data_object{
		util::init_object<st_mysqlx_statement>(mysqlx_sql_statement_class_entry, sql_stmt_obj) };
	data_object.stmt = stmt;
	data_object.stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
	data_object.execute_flags = 0;
	data_object.send_query_status = FAIL;
	data_object.in_execution = FALSE;
	data_object.has_more_results = FALSE;
	data_object.has_more_rows_in_set = FALSE;

	DBG_RETURN(sql_stmt_obj);
}

/*********************************************************************/
util::zvalue
mysqlx_statement_execute_read_response(const st_mysqlx_object* const mysqlx_object,
											const zend_long flags,
											const enum mysqlx_result_type result_type)
{
	st_mysqlx_statement* object = (st_mysqlx_statement*) mysqlx_object->ptr;
	DBG_ENTER("mysqlx_statement_execute");

	util::zvalue return_value(false);

	if (!object) {
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		DBG_ERR_FMT("invalid object of class %s on %s::%d", ZSTR_VAL(mysqlx_object->zo.ce->name), __FILE__, __LINE__);
		DBG_RETURN(return_value);
	}

	if ((flags | MYSQLX_EXECUTE_ALL_FLAGS) != MYSQLX_EXECUTE_ALL_FLAGS) {
		util::ostringstream os;
		os << "Invalid flags. Unknown " << (flags - (flags | MYSQLX_EXECUTE_ALL_FLAGS));
		php_error_docref(nullptr, E_WARNING, "%s", os.str().c_str());

		DBG_RETURN(return_value);
	}
	DBG_INF_FMT("flags=%lu", flags);
	DBG_INF_FMT("%sSYNC", (flags & MYSQLX_EXECUTE_FLAG_ASYNC)? "A":"");
	DBG_INF_FMT("%s", (flags & MYSQLX_EXECUTE_FLAG_BUFFERED)? "BUFFERED":"FWD");

	if (TRUE == object->in_execution) {
		php_error_docref(nullptr, E_WARNING, "Statement in execution. Please fetch all data first.");
	} else {
		xmysqlnd_stmt * stmt = object->stmt;
		object->execute_flags = flags;
		object->has_more_rows_in_set = FALSE;
		object->has_more_results = FALSE;
		object->send_query_status = PASS;

		if (object->execute_flags & MYSQLX_EXECUTE_FLAG_ASYNC) {
			DBG_INF("ASYNC");
			return_value = (PASS == object->send_query_status);
		} else {
				const st_xmysqlnd_stmt_on_warning_bind on_warning = { mysqlx_sql_stmt_on_warning, nullptr };
				const st_xmysqlnd_stmt_on_error_bind on_error = { mysqlx_sql_stmt_on_error, nullptr };
				XMYSQLND_STMT_RESULT * result;
				if (object->execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
					result = stmt->get_buffered_result(stmt,
								&object->has_more_results,
								on_warning,
								on_error, nullptr, nullptr);
				} else {
					result = stmt->get_fwd_result(stmt,
								MYSQLX_EXECUTE_FWD_PREFETCH_COUNT,
								&object->has_more_rows_in_set,
								&object->has_more_results,
								on_warning, on_error, nullptr, nullptr);
				}

				DBG_INF_FMT("has_more_results=%s   has_more_rows_in_set=%s",
							object->has_more_results? "TRUE":"FALSE",
							object->has_more_rows_in_set? "TRUE":"FALSE");

				if (result) {
					switch(result_type)
					{
						case MYSQLX_RESULT:
							return_value = create_result(result);
							break;

						case MYSQLX_RESULT_DOC:
							return_value = create_doc_result(result);
							break;

						case MYSQLX_RESULT_ROW:
							return_value = create_row_result(result);
							break;

						case MYSQLX_RESULT_SQL:
							return_value = create_sql_stmt_result(result, object);
							break;

						default:
							assert(!"unknown result type!");
							return_value = false;
					}
				} else {
					RAISE_EXCEPTION_FETCH_FAIL();
					/* Or we should close the connection, rendering it unusable at this point ?*/
					object->send_query_status = FAIL;
				}
			}

	}
	DBG_RETURN(return_value);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_statement, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_statement, hasMoreResults)
{
	DBG_ENTER("mysqlx_statement::hasMoreResults");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_statement>(object_zv) };

	RETVAL_BOOL(data_object.stmt->has_more_results(data_object.stmt));
	DBG_INF_FMT("%s", Z_TYPE_P(return_value) == IS_TRUE? "YES":"NO");

	DBG_VOID_RETURN;
}

/*     proto mixed mysqlx_statement::readResult(object statement, callable on_row_cb, callable on_error_cb, callable on_rset_end, callable on_stmt_ok[, mixed cb_param]]) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_statement, getResult)
{
	mysqlx_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, mysqlx_statement_class_entry);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_statement, getNextResult)
{
	mysqlx_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, mysqlx_statement_class_entry);
}

static const zend_function_entry mysqlx_statement_methods[] = {
	PHP_ME(mysqlx_statement, __construct, arginfo_mysqlx_sql_statement__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_statement, hasMoreResults,	arginfo_mysqlx_sql_statement__has_more_results,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_statement, getResult,		arginfo_mysqlx_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_statement, getNextResult,	arginfo_mysqlx_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_statement_handlers;
static HashTable mysqlx_statement_properties;

const st_mysqlx_property_entry mysqlx_statement_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};


static void
mysqlx_statement_free_storage(zend_object * object)
{
	/* shows the intention that we do reuse code */
	mysqlx_sql_statement_free_storage(object);
}

static zend_object *
php_mysqlx_statement_object_allocator(zend_class_entry* class_type)
{
	/* shows the intention that we do reuse code */
	return php_mysqlx_sql_statement_object_allocator(class_type);
}

void
mysqlx_register_statement_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_statement_class_entry,
		"Statement",
		mysqlx_std_object_handlers,
		mysqlx_object_statement_handlers,
		php_mysqlx_statement_object_allocator,
		mysqlx_statement_free_storage,
		mysqlx_statement_methods,
		mysqlx_statement_properties,
		mysqlx_statement_property_entries);

	zend_declare_class_constant_long(mysqlx_statement_class_entry, "EXECUTE_ASYNC", sizeof("EXECUTE_ASYNC") - 1, MYSQLX_EXECUTE_FLAG_ASYNC);
	zend_declare_class_constant_long(mysqlx_statement_class_entry, "BUFFERED", sizeof("BUFFERED") - 1, MYSQLX_EXECUTE_FLAG_BUFFERED);
}

void
mysqlx_unregister_statement_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_statement_properties);
}

util::zvalue
create_stmt(xmysqlnd_stmt * stmt)
{
	DBG_ENTER("create_stmt");

	util::zvalue stmt_obj;
	st_mysqlx_statement& data_object{
		util::init_object<st_mysqlx_statement>(mysqlx_statement_class_entry, stmt_obj) };
	data_object.stmt = stmt;
	data_object.stmt_execute = nullptr;
	data_object.execute_flags = 0;
	data_object.send_query_status = FAIL;
	data_object.in_execution = FALSE;
	data_object.has_more_results = FALSE;
	data_object.has_more_rows_in_set = FALSE;

	DBG_RETURN(stmt_obj);
}

} // namespace devapi

} // namespace mysqlx
