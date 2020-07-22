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
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_rowset.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_wireprotocol.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_object.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_exception.h"
#include "xmysqlnd_zval2any.h"

namespace mysqlx {

namespace drv {

xmysqlnd_stmt::xmysqlnd_stmt(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const obj_factory,
										  XMYSQLND_SESSION cur_session)
{
	DBG_ENTER("xmysqlnd_stmt::init");
	session = cur_session;
	object_factory = obj_factory;
}

enum_func_status
xmysqlnd_stmt::send_raw_message(xmysqlnd_stmt * const stmt,
													  const st_xmysqlnd_pb_message_shell message_shell,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info)
{
	/* pass stmt->session->data->io directly ?*/
	st_xmysqlnd_message_factory msg_factory{ stmt->session->data->create_message_factory() };
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_stmt::send_raw_message");

	stmt->partial_read_started = FALSE;
	stmt->get_msg_stmt_exec() = msg_factory.get__collection_read(&msg_factory);

	ret = stmt->get_msg_stmt_exec().send_execute_request(&stmt->get_msg_stmt_exec(), message_shell);

	DBG_INF_FMT("send_request returned %s", PASS == ret? "PASS":"FAIL");

	DBG_RETURN(PASS);
}

static XMYSQLND_ROWSET * create_rowset_fwd(void * context)
{
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	XMYSQLND_ROWSET * result;
	DBG_ENTER("xmysqlnd_stmt::create_rowset_fwd");
	result = xmysqlnd_rowset_create(XMYSQLND_TYPE_ROWSET_FWD_ONLY,
									ctx->fwd_prefetch_count,
									ctx->stmt,
									ctx->stmt->get_persistent(),
									ctx->stmt->object_factory,
									ctx->stats,
									ctx->error_info);
	DBG_RETURN(result);
}

static XMYSQLND_ROWSET * create_rowset_buffered(void * context)
{
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	XMYSQLND_ROWSET * result;
	DBG_ENTER("xmysqlnd_stmt::create_rowset_buffered");
	result = xmysqlnd_rowset_create(XMYSQLND_TYPE_ROWSET_BUFFERED, (size_t)~0, ctx->stmt, ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(result);
}

static XMYSQLND_STMT_RESULT_META *
XMYSQLND_METHOD(xmysqlnd_stmt, create_meta)(void * context)
{
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	XMYSQLND_STMT_RESULT_META * meta;
	DBG_ENTER("xmysqlnd_stmt::create_meta");
	meta = xmysqlnd_stmt_result_meta_create(ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(meta);
}

static XMYSQLND_RESULT_FIELD_META * create_meta_field(void * context)
{
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	XMYSQLND_RESULT_FIELD_META * field;
	DBG_ENTER("xmysqlnd_stmt::create_meta_field");
	field = xmysqlnd_result_field_meta_create(ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(field);
}

static const enum_hnd_func_status handler_on_row_field(void * context,
														  const util::string_view& buffer,
														  const unsigned int idx,
														  const func_xmysqlnd_wireprotocol__row_field_decoder decoder)
{
	st_xmysqlnd_stmt_bind_ctx* const ctx = (st_xmysqlnd_stmt_bind_ctx*) context;
	enum_hnd_func_status ret{HND_AGAIN};

	DBG_ENTER("xmysqlnd_stmt::handler_on_row_field");
	DBG_INF_FMT("rowset=%p  meta=%p  on_row.handler=%p", ctx->rowset, ctx->meta, ctx->on_row.handler);
	if (!ctx->rowset && ctx->meta) {
		ctx->rowset = ctx->create_rowset(ctx);
		if (ctx->rowset) {
			ctx->rowset->m.attach_meta(ctx->rowset, ctx->meta, ctx->stats, ctx->error_info);
		}
	}
	if (ctx->rowset) {
		if (idx == 0) {
			ctx->current_row = ctx->rowset->m.create_row(ctx->rowset, ctx->meta, ctx->stats, ctx->error_info);
		}
		decoder(buffer, ctx->meta->m->get_field(ctx->meta, idx), idx, &ctx->current_row[idx]);

		if ((idx + 1) == ctx->meta->m->get_field_count(ctx->meta)) {
			if (ctx->on_row.handler) {
				ret = ctx->on_row.handler(ctx->on_row.ctx, ctx->stmt, ctx->meta, ctx->current_row, ctx->stats, ctx->error_info);
				ret = HND_AGAIN; /* for now we don't allow fetching to be suspended and continued later */

				ctx->rowset->m.destroy_row(ctx->rowset, ctx->current_row, ctx->stats, ctx->error_info);
			} else {
				DBG_INF_FMT("fwd_prefetch_count=" MYSQLX_LLU_SPEC " prefetch_counter=" MYSQLX_LLU_SPEC, ctx->fwd_prefetch_count, ctx->prefetch_counter);
				ctx->rowset->m.add_row(ctx->rowset, ctx->current_row, ctx->stats, ctx->error_info);
				if (ctx->fwd_prefetch_count && !--ctx->prefetch_counter) {
					ret = HND_PASS; /* Otherwise it is HND_AGAIN */
				}
			}
		}
	}

	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_meta_field(void * context, st_xmysqlnd_result_field_meta* field)
{
	st_xmysqlnd_stmt_bind_ctx* const ctx = (st_xmysqlnd_stmt_bind_ctx*) context;
	enum_hnd_func_status ret{HND_AGAIN};

	DBG_ENTER("xmysqlnd_stmt::handler_on_meta_field");
	DBG_INF_FMT("rowset=%p  meta=%p", ctx->rowset, ctx->meta);
	if (!ctx->meta) {
		ctx->meta = xmysqlnd_stmt_result_meta_create(ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->meta) {
		ctx->meta->m->add_field(ctx->meta, field, ctx->stats, ctx->error_info);
	}
	DBG_INF_FMT("rowset=%p  meta=%p", ctx->rowset, ctx->meta);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_warning(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const util::string_view& message)
{
	st_xmysqlnd_stmt_bind_ctx* const ctx = (st_xmysqlnd_stmt_bind_ctx*) context;
	enum_hnd_func_status ret{HND_AGAIN};

	DBG_ENTER("xmysqlnd_stmt::handler_on_warning");
	if (ctx->on_warning.handler) {
		ret = ctx->on_warning.handler(ctx->on_warning.ctx, ctx->stmt, level, code, message);
	}
	if (!ctx->warnings) {
		ctx->warnings = xmysqlnd_warning_list_create(ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->warnings) {
		ctx->warnings->add_warning(level, code, message);
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_error(void * context, const unsigned int code, const util::string_view& sql_state, const util::string_view& message)
{
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	enum_hnd_func_status ret{HND_PASS_RETURN_FAIL};
	DBG_ENTER("xmysqlnd_stmt::handler_on_error");
	if (ctx->on_error.handler) {
		ret = ctx->on_error.handler(ctx->on_error.ctx, ctx->stmt, code, sql_state, message);
	} else if (ctx->error_info) {
		SET_CLIENT_ERROR(ctx->error_info, code, sql_state.data(), message.data());
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_exec_state_change(void * context, const enum xmysqlnd_execution_state_type type, const size_t value)
{
	st_xmysqlnd_stmt_bind_ctx* const ctx = (st_xmysqlnd_stmt_bind_ctx*) context;
	enum_hnd_func_status ret{HND_AGAIN};

	DBG_ENTER("xmysqlnd_stmt::handler_on_exec_state_change");
	if (!ctx->exec_state) {
		ctx->exec_state = xmysqlnd_stmt_execution_state_create(ctx->stmt->get_persistent(), ctx->stmt->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->exec_state) {
		switch (type) {
			case EXEC_STATE_GENERATED_INSERT_ID:
				ctx->exec_state->m->set_last_insert_id(ctx->exec_state, value);
				break;
			case EXEC_STATE_ROWS_AFFECTED:
				ctx->exec_state->m->set_affected_items_count(ctx->exec_state, value);
				break;
			case EXEC_STATE_ROWS_FOUND:
				ctx->exec_state->m->set_found_items_count(ctx->exec_state, value);
				break;
			case EXEC_STATE_ROWS_MATCHED:
				ctx->exec_state->m->set_matched_items_count(ctx->exec_state, value);
				break;
			case EXEC_STATE_NONE:
				break;
		}
	}

	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_generated_doc_ids(void * context, const util::string& id)
{
	st_xmysqlnd_stmt_bind_ctx* const ctx = static_cast<st_xmysqlnd_stmt_bind_ctx*>( context );
	enum_hnd_func_status ret{HND_AGAIN};
	DBG_ENTER("xmysqlnd_stmt::handler_on_generated_doc_ids");
	ctx->exec_state->m->add_generated_doc_id(ctx->exec_state, id);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_trx_state_change(void* /*context*/, const enum xmysqlnd_transaction_state_type type)
{
#if 0
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
#endif
	const enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_stmt::handler_on_trx_state_change");
	DBG_INF_FMT("type=%s", type == TRX_STATE_COMMITTED? "COMMITTED":(type == TRX_STATE_ROLLEDBACK? "ROLLED BACK":"n/a"));
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_resultset_end(void * context, const zend_bool has_more)
{
	enum_hnd_func_status ret{HND_AGAIN};
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	DBG_ENTER("xmysqlnd_stmt::handler_on_resultset_end");
	DBG_INF_FMT("on_resultset_end.handler=%p", ctx->on_resultset_end.handler);
	if (ctx->on_resultset_end.handler) {
		ret = ctx->on_resultset_end.handler(ctx->on_resultset_end.ctx, ctx->stmt, has_more);
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
handler_on_statement_ok(void * context)
{
	enum_hnd_func_status ret{HND_PASS};
	const st_xmysqlnd_stmt_bind_ctx* const ctx = (const st_xmysqlnd_stmt_bind_ctx* ) context;
	DBG_ENTER("xmysqlnd_stmt::handler_on_statement_ok");
	DBG_INF_FMT("on_statement_ok.handler=%p", ctx->on_statement_ok.handler);
	if (ctx->on_statement_ok.handler) {
		ret = ctx->on_statement_ok.handler(ctx->on_row.ctx, ctx->stmt, ctx->exec_state);
	}
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_stmt::read_one_result(xmysqlnd_stmt * const stmt,
													 const st_xmysqlnd_stmt_on_row_bind on_row,
													 const st_xmysqlnd_stmt_on_warning_bind on_warning,
													 const st_xmysqlnd_stmt_on_error_bind on_error,
													 const st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
													 const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
													 zend_bool * const has_more_results,
													 MYSQLND_STATS * const stats,
													 MYSQLND_ERROR_INFO * const error_info)
{
	st_xmysqlnd_stmt_bind_ctx create_ctx =
	{
		stmt,
		stats,
		error_info,
		create_rowset_buffered,
		0,		/* fwd_prefetch_count */
		0,		/* prefetch_counter */
		nullptr,	/* current_row */
		nullptr,	/* rowset */
		nullptr,	/* meta */
		nullptr,	/* result */
		nullptr,	/* warnings */
		nullptr,	/* exec_state */
		on_row,
		on_warning,
		on_error,
		on_resultset_end,
		on_statement_ok,
	};
	const st_xmysqlnd_meta_field_create_bind create_meta_field_bind = { create_meta_field, &create_ctx };
	const st_xmysqlnd_on_row_field_bind handler_on_row_field_msg = { on_row.handler? handler_on_row_field : nullptr, &create_ctx };
	const st_xmysqlnd_on_meta_field_bind handler_on_meta_field_msg = { handler_on_meta_field, &create_ctx };
	const st_xmysqlnd_on_warning_bind handler_on_warning_msg = { on_warning.handler? handler_on_warning : nullptr, &create_ctx };
	const st_xmysqlnd_on_error_bind handler_on_error_msg = { on_error.handler? handler_on_error : nullptr , on_error.handler? &create_ctx : nullptr };
	const st_xmysqlnd_on_generated_doc_ids_bind handler_on_generated_doc_ids_msg = {handler_on_generated_doc_ids, &read_ctx };
	const st_xmysqlnd_on_execution_state_change_bind handler_on_exec_state_change_msg = { handler_on_exec_state_change, &create_ctx };
	const st_xmysqlnd_on_session_var_change_bind handler_on_session_var_change = { nullptr, nullptr };
	const st_xmysqlnd_on_trx_state_change_bind handler_on_trx_state_change_msg = { handler_on_trx_state_change, &create_ctx };
	const st_xmysqlnd_on_stmt_execute_ok_bind handler_on_stmt_execute_ok = { on_resultset_end.handler? handler_on_statement_ok : nullptr, &create_ctx };
	const st_xmysqlnd_on_resultset_end_bind handler_on_resultset_end_msg = { on_statement_ok.handler? handler_on_resultset_end : nullptr, &create_ctx };

	DBG_ENTER("xmysqlnd_stmt::read_one_result");
	DBG_INF_FMT("on_row.handler=%p", on_row.handler);
	DBG_INF_FMT("on_warning.handler=%p", on_warning.handler);
	DBG_INF_FMT("on_error.handler=%p", on_error.handler);
	DBG_INF_FMT("on_resultset_end.handler=%p", on_resultset_end.handler);
	DBG_INF_FMT("on_statement_ok.handler=%p", on_statement_ok.handler);

	/*
	  Maybe we can inject a callbacks that creates `meta` on demand, but we still DI it.
	  This way we don't pre-create `meta` and in case of UPSERT we don't waste cycles.
	  For now, we just pre-create.
	*/
	if (FAIL == stmt->get_msg_stmt_exec().init_read(&stmt->get_msg_stmt_exec(),
													create_meta_field_bind,
													handler_on_row_field_msg,
													handler_on_meta_field_msg,
													handler_on_warning_msg,
													handler_on_error_msg,
													handler_on_generated_doc_ids_msg,
													handler_on_exec_state_change_msg,
													handler_on_session_var_change,
													handler_on_trx_state_change_msg,
													handler_on_stmt_execute_ok,
													handler_on_resultset_end_msg)) {
		DBG_RETURN(FAIL);
	}

	if (FAIL == stmt->get_msg_stmt_exec().read_response(&stmt->get_msg_stmt_exec(), nullptr)) {
		DBG_RETURN(FAIL);
	}
	*has_more_results = stmt->get_msg_stmt_exec().reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	if (create_ctx.rowset) {
		xmysqlnd_rowset_free(create_ctx.rowset, stats, error_info);
		create_ctx.rowset = nullptr;
	}
	if (create_ctx.meta) {
		xmysqlnd_stmt_result_meta_free(create_ctx.meta, stats, error_info);
		create_ctx.meta = nullptr;
	}
	if (create_ctx.exec_state) {
		xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
		create_ctx.exec_state = nullptr;
	}
	if (create_ctx.warnings) {
		xmysqlnd_warning_list_free(create_ctx.warnings);
		create_ctx.warnings = nullptr;
	}
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_stmt::read_all_results(xmysqlnd_stmt * const stmt,
													  const st_xmysqlnd_stmt_on_row_bind on_row,
													  const st_xmysqlnd_stmt_on_warning_bind on_warning,
													  const st_xmysqlnd_stmt_on_error_bind on_error,
													  const st_xmysqlnd_stmt_on_result_start_bind on_result_start,
													  const st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
													  const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret{FAIL};
	zend_bool has_more;
	DBG_ENTER("xmysqlnd_stmt::read_all_results");
	do {
		has_more = FALSE;
		if (on_result_start.handler) {
			on_result_start.handler(on_result_start.ctx, stmt);
		}
		ret = stmt->read_one_result(stmt, on_row,
							on_warning,
							on_error,
							on_resultset_end,
							on_statement_ok,
							&has_more,
							stats,
							error_info);

	} while (ret == PASS && has_more == TRUE);
	DBG_RETURN(ret);
}

zend_bool
xmysqlnd_stmt::has_more_results(xmysqlnd_stmt * stmt)
{
	DBG_ENTER("xmysqlnd_stmt::has_more_results");
	DBG_INF_FMT("has_more=%s", stmt->get_msg_stmt_exec().reader_ctx.has_more_results? "TRUE":"FALSE");
	DBG_RETURN(stmt->get_msg_stmt_exec().reader_ctx.has_more_results);
}

XMYSQLND_STMT_RESULT *
xmysqlnd_stmt::get_buffered_result(xmysqlnd_stmt * const stmt,
														 zend_bool * const has_more_results,
														 const st_xmysqlnd_stmt_on_warning_bind handler_on_warning_bind,
														 const st_xmysqlnd_stmt_on_error_bind handler_on_error_bind,
														 MYSQLND_STATS * const stats,
														 MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_STMT_RESULT* result{nullptr};
	struct st_xmysqlnd_stmt_bind_ctx create_ctx =
	{
		stmt,
		stats,
		error_info,
		create_rowset_buffered,
		0,		/* fwd_prefetch_count */
		0,		/* prefetch_counter */
		nullptr,	/* current_row */
		nullptr,	/* rowset */
		nullptr,	/* meta */
		nullptr,	/* result */
		nullptr,	/* warnings */
		nullptr,	/* exec_state */
		{ nullptr, nullptr },	/* on_row */
		handler_on_warning_bind,
		handler_on_error_bind,
		{ nullptr, nullptr },	/* on_resultset_end */
		{ nullptr, nullptr },	/* on_statement_ok */
	};

	const st_xmysqlnd_meta_field_create_bind create_meta_field_bind = {
		create_meta_field, &create_ctx };
	const st_xmysqlnd_on_row_field_bind on_row_field = {
		handler_on_row_field, &create_ctx };
	const st_xmysqlnd_on_meta_field_bind on_meta_field = {
		handler_on_meta_field, &create_ctx };
	const st_xmysqlnd_on_warning_bind on_warning = {
		handler_on_warning_bind.handler? handler_on_warning : nullptr, &create_ctx };
	const st_xmysqlnd_on_error_bind on_error = {
		(handler_on_error_bind.handler || error_info) ? handler_on_error : nullptr, &create_ctx };
	const st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = {
		handler_on_exec_state_change, &create_ctx };
	const st_xmysqlnd_on_generated_doc_ids_bind handler_on_generated_doc_ids_bind = {
		handler_on_generated_doc_ids, &read_ctx };
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change = { nullptr, nullptr };
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = {
		handler_on_trx_state_change, &create_ctx };
	const st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { nullptr, nullptr };
	const st_xmysqlnd_on_resultset_end_bind on_resultset_end = { nullptr, nullptr };
	DBG_ENTER("xmysqlnd_stmt::get_buffered_result");

	/*
	  Maybe we can inject a callbacks that creates `meta` on demand, but we still DI it.
	  This way we don't pre-create `meta` and in case of UPSERT we don't waste cycles.
	  For now, we just pre-create.
	*/
	if (FAIL == stmt->get_msg_stmt_exec().init_read(&stmt->get_msg_stmt_exec(),
													create_meta_field_bind,
													on_row_field,
													on_meta_field,
													on_warning,
													on_error,
													handler_on_generated_doc_ids_bind,
													on_exec_state_change,
													on_session_var_change,
													on_trx_state_change,
													on_stmt_execute_ok,
													on_resultset_end)) {
		DBG_RETURN(nullptr);
	}

	if (FAIL == stmt->get_msg_stmt_exec().read_response(&stmt->get_msg_stmt_exec(), nullptr)) {
		DBG_RETURN(nullptr);
	}
	*has_more_results = stmt->get_msg_stmt_exec().reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	result = xmysqlnd_stmt_result_create(stmt->get_persistent(), stmt->object_factory, stats, error_info);
	if (result) {
		result->m.attach_rowset(result, create_ctx.rowset, stats, error_info);
		result->m.attach_meta(result, create_ctx.meta, stats, error_info);
		result->m.attach_execution_state(result, create_ctx.exec_state);
		result->m.attach_warning_list(result, create_ctx.warnings);
	} else {
		if (create_ctx.rowset) {
			xmysqlnd_rowset_free(create_ctx.rowset, stats, error_info);
			create_ctx.rowset = nullptr;
		}
		if (create_ctx.meta) {
			xmysqlnd_stmt_result_meta_free(create_ctx.meta, stats, error_info);
			create_ctx.meta = nullptr;
		}
		if (create_ctx.exec_state) {
			xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
			create_ctx.exec_state = nullptr;
		}
		if (create_ctx.warnings) {
			xmysqlnd_warning_list_free(create_ctx.warnings);
			create_ctx.warnings = nullptr;
		}
	}
	DBG_RETURN(result);
}

XMYSQLND_STMT_RESULT *
xmysqlnd_stmt::get_fwd_result(xmysqlnd_stmt * const stmt,
													const size_t rows,
													zend_bool * const has_more_rows_in_set,
													zend_bool * const has_more_results,
													const st_xmysqlnd_stmt_on_warning_bind handler_on_warning_bind,
													const st_xmysqlnd_stmt_on_error_bind handler_on_error_bind,
													MYSQLND_STATS * const stats,
													MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_STMT_RESULT* result{nullptr};
	const st_xmysqlnd_meta_field_create_bind create_meta_field_bind = { create_meta_field, &read_ctx };
	const st_xmysqlnd_on_row_field_bind on_row_field = { handler_on_row_field, &read_ctx };
	const st_xmysqlnd_on_meta_field_bind on_meta_field = { handler_on_meta_field, &read_ctx };
	const st_xmysqlnd_on_warning_bind on_warning = { handler_on_warning_bind.handler? handler_on_warning : nullptr, &read_ctx };
	const st_xmysqlnd_on_error_bind on_error = { (handler_on_error_bind.handler || error_info) ? handler_on_error : nullptr, &read_ctx };
	const st_xmysqlnd_on_generated_doc_ids_bind handler_on_generated_doc_ids_bind = {  handler_on_generated_doc_ids, &read_ctx };
	const st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { handler_on_exec_state_change, &read_ctx };
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change = { nullptr, nullptr };
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = { nullptr, nullptr };
	const st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { nullptr, nullptr };
	const st_xmysqlnd_on_resultset_end_bind on_resultset_end = { nullptr, nullptr };
	DBG_ENTER("xmysqlnd_stmt::get_fwd_result");
	DBG_INF_FMT("rows=" MYSQLX_LLU_SPEC, rows);

	if (FALSE == stmt->partial_read_started) {
		read_ctx.stmt = stmt;
		read_ctx.stats = stats;
		read_ctx.error_info = error_info;
		read_ctx.create_rowset = create_rowset_fwd;
		read_ctx.current_row = nullptr;
		read_ctx.rowset = nullptr;
		read_ctx.meta = nullptr;
		read_ctx.result = xmysqlnd_stmt_result_create(stmt->get_persistent(), stmt->object_factory, stats, error_info);
		read_ctx.warnings = xmysqlnd_warning_list_create(stmt->get_persistent(), stmt->object_factory, stats, error_info);
		read_ctx.exec_state = xmysqlnd_stmt_execution_state_create(stmt->get_persistent(), stmt->object_factory, stats, error_info);
		read_ctx.on_warning = handler_on_warning_bind;
		read_ctx.on_error = handler_on_error_bind;

		if ((result = read_ctx.result) == nullptr) {
			DBG_RETURN(nullptr);
		}
		result->m.attach_execution_state(result, read_ctx.exec_state);
		result->m.attach_warning_list(result, read_ctx.warnings);

		if (FAIL == stmt->get_msg_stmt_exec().init_read(&stmt->get_msg_stmt_exec(),
														create_meta_field_bind,
														on_row_field,
														on_meta_field,
														on_warning,
														on_error,
														handler_on_generated_doc_ids_bind,
														on_exec_state_change,
														on_session_var_change,
														on_trx_state_change,
														on_stmt_execute_ok,
														on_resultset_end))
		{
			xmysqlnd_stmt_result_free(read_ctx.result, stats, error_info);
			DBG_RETURN(nullptr);
		}
		stmt->partial_read_started = TRUE;
	}
	/*
	  We can't be sure about more rows in the set, so we speculate if rows == 0.
	  If rows > 0, then we will read at least 1 row and we will be sure
	*/
	*has_more_rows_in_set = TRUE;
	*has_more_results = FALSE;

	read_ctx.fwd_prefetch_count = rows;
	read_ctx.prefetch_counter = rows;

	if (rows) {
		if (FAIL == stmt->get_msg_stmt_exec().read_response(&stmt->get_msg_stmt_exec(), nullptr)) {
			DBG_RETURN(nullptr);
		}
		*has_more_rows_in_set = stmt->get_msg_stmt_exec().reader_ctx.has_more_rows_in_set;
		*has_more_results = stmt->get_msg_stmt_exec().reader_ctx.has_more_results;
	}
	DBG_INF_FMT("current_rowset=%p  has_more_results=%s has_more_rows_in_set=%s",
				 read_ctx.rowset, *has_more_results? "TRUE":"FALSE", *has_more_rows_in_set? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", read_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", read_ctx.warnings);

	result = read_ctx.result;
	result->m.attach_rowset(result, read_ctx.rowset, stats, error_info);
	result->m.attach_meta(result, read_ctx.meta, stats, error_info);

	DBG_RETURN(result);
}

enum_func_status
xmysqlnd_stmt::skip_one_result(xmysqlnd_stmt * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	struct st_xmysqlnd_stmt_bind_ctx create_ctx = { stmt, stats, error_info };
	const st_xmysqlnd_meta_field_create_bind create_meta_field = { nullptr, nullptr };
	const st_xmysqlnd_on_row_field_bind on_row_field = { nullptr, nullptr };
	const st_xmysqlnd_on_meta_field_bind on_meta_field = { nullptr, nullptr };
	const st_xmysqlnd_on_warning_bind on_warning = { nullptr, nullptr };
	const st_xmysqlnd_on_error_bind on_error = { nullptr, nullptr };
	const st_xmysqlnd_on_generated_doc_ids_bind handler_on_generated_doc_ids_bind = {handler_on_generated_doc_ids, &read_ctx };
	const st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { handler_on_exec_state_change, &create_ctx };
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change = { nullptr, nullptr };
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = { nullptr, nullptr };
	const st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { nullptr, nullptr };
	const st_xmysqlnd_on_resultset_end_bind on_resultset_end = { nullptr, nullptr };

	DBG_ENTER("xmysqlnd_stmt::skip_one_result");
	if (FAIL == stmt->get_msg_stmt_exec().init_read(&stmt->get_msg_stmt_exec(),
													create_meta_field,
													on_row_field,
													on_meta_field,
													on_warning,
													on_error,
													handler_on_generated_doc_ids_bind,
													on_exec_state_change,
													on_session_var_change,
													on_trx_state_change,
													on_stmt_execute_ok,
													on_resultset_end)) {
		DBG_RETURN(FAIL);
	}

	if (FAIL == stmt->get_msg_stmt_exec().read_response(&stmt->get_msg_stmt_exec(), nullptr)) {
		DBG_RETURN(FAIL);
	}
	*has_more_results = stmt->get_msg_stmt_exec().reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	if (create_ctx.exec_state) {
		xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
		create_ctx.exec_state = nullptr;
	}
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_stmt::skip_all_results(xmysqlnd_stmt * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	zend_bool has_more;
	DBG_ENTER("xmysqlnd_stmt::skip_all_results");
	do {
		ret = stmt->skip_one_result(stmt, &has_more, stats, error_info);
	} while (PASS == ret && has_more == TRUE);
	DBG_RETURN(ret);
}

xmysqlnd_stmt *
xmysqlnd_stmt::get_reference(xmysqlnd_stmt * const stmt)
{
	DBG_ENTER("xmysqlnd_stmt::get_reference");
	++refcount;
	DBG_INF_FMT("new_refcount=%u", refcount);
	DBG_RETURN(stmt);
}

enum_func_status
xmysqlnd_stmt::free_reference(xmysqlnd_stmt * const stmt)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_stmt::free_reference");
	DBG_INF_FMT("old_refcount=%u", refcount);
	if (!(--refcount)) {
		cleanup(stmt);
	}
	DBG_RETURN(ret);
}

void
xmysqlnd_stmt::free_contents(xmysqlnd_stmt * const /*stmt*/)
{
	DBG_ENTER("xmysqlnd_stmt::free_contents");
	DBG_VOID_RETURN;
}

void
xmysqlnd_stmt::cleanup(xmysqlnd_stmt * const stmt)
{
	DBG_ENTER("xmysqlnd_stmt::cleanup");
	free_contents(stmt);
	DBG_VOID_RETURN;
}

xmysqlnd_stmt *
xmysqlnd_stmt_create(XMYSQLND_SESSION session,
						  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						  MYSQLND_STATS * const stats,
						  MYSQLND_ERROR_INFO * const error_info)
{
	xmysqlnd_stmt* stmt{nullptr};
	DBG_ENTER("xmysqlnd_stmt_create");
	stmt = object_factory->get_stmt(object_factory, session, false, stats, error_info);
	if (stmt) {
		stmt = stmt->get_reference(stmt);
	}
	DBG_RETURN(stmt);
}

void
xmysqlnd_stmt_free(xmysqlnd_stmt* const stmt, MYSQLND_STATS* stats, MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_stmt_free");
	if (stmt) {
		stmt->free_reference(stmt);
	}
	DBG_VOID_RETURN;
}

Prepare_stmt_data::Prepare_stmt_data() :
	next_ps_id{ DEFAULT_PS_ID },
    ps_supported{ true }
{

}

size_t
Prepare_stmt_data::get_ps_entry( const google::protobuf::Message& msg )
{
	const auto serialized_msg = msg.SerializeAsString();
	size_t idx{ 0 };
	for( const auto& elem : ps_db )
	{
		if( elem.serialized_message == serialized_msg )
		{
			return idx;
		}
		++idx;
	}
	return ps_db.size() + 1;
}

size_t
Prepare_stmt_data::get_ps_entry( const uint32_t msg_id )
{
	size_t idx{ 0 };
	for( const auto& elem : ps_db )
	{
		if( elem.msg_id == msg_id )
		{
			return idx;
		}
		++idx;
	}
	return ps_db.size() + 1;
}

template<>
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
											Mysqlx::Crud::Insert* msg )
{
	one_msg->set_allocated_insert( msg );
	one_msg->set_type( Mysqlx::Prepare::Prepare_OneOfMessage_Type::Prepare_OneOfMessage_Type_INSERT );
}

template<>
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
											Mysqlx::Crud::Find* msg )
{
	one_msg->set_allocated_find( msg );
	one_msg->set_type( Mysqlx::Prepare::Prepare_OneOfMessage_Type::Prepare_OneOfMessage_Type_FIND );

}

template<>
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
											Mysqlx::Crud::Update* msg )
{
	one_msg->set_allocated_update( msg );
	one_msg->set_type( Mysqlx::Prepare::Prepare_OneOfMessage_Type::Prepare_OneOfMessage_Type_UPDATE );

}

template<>
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
											Mysqlx::Crud::Delete* msg )
{
	one_msg->set_allocated_delete_( msg );
	one_msg->set_type( Mysqlx::Prepare::Prepare_OneOfMessage_Type::Prepare_OneOfMessage_Type_DELETE );

}

template<>
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
                                            Mysqlx::Sql::StmtExecute* msg )
{
    one_msg->set_allocated_stmt_execute( msg );
    one_msg->set_type( Mysqlx::Prepare::Prepare_OneOfMessage_Type::Prepare_OneOfMessage_Type_STMT );

}

void
Prepare_stmt_data::assign_session( XMYSQLND_SESSION session_obj )
{
	session = session_obj;
}

bool
Prepare_stmt_data::send_prepare_msg( uint32_t message_id )
{
	st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
	Mysqlx::Prepare::Prepare prep_msg;
	size_t                   db_idx = get_ps_entry(message_id);
	bool                     res{ true };
	if( db_idx < ps_db.size() ) {
        ps_deliver_message_code = 0;
		prep_msg = ps_db[ db_idx ].prepare_msg;
		st_xmysqlnd_msg__prepare_prepare prepare_prepare = msg_factory.get__prepare_prepare(&msg_factory);
		enum_func_status request_ret = prepare_prepare.send_prepare_request(&prepare_prepare,
											get_protobuf_msg(&prep_msg, COM_PREPARE_PREPARE));
		if( PASS == request_ret ) {
			drv::xmysqlnd_stmt * stmt = session->create_statement_object(session);
			stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
			if( get_prepare_resp( stmt ) ) {
				ps_db[ db_idx ].delivered_ps = true;
                if( ps_deliver_message_code != 0 ) {
                    ps_db.erase( ps_db.begin() + db_idx );
                    res = false;
                }
			} else {
				res = false;
			}
		}
	} else {
		res = false;
	}
	return res;
}

void
Prepare_stmt_data::set_ps_server_error( const uint32_t message_code )
{
    ps_deliver_message_code = message_code;
}

bool
Prepare_stmt_data::get_prepare_resp( drv::xmysqlnd_stmt * stmt )
{
	st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
	st_xmysqlnd_msg__prepare_prepare prepare_prepare = msg_factory.get__prepare_prepare(&msg_factory);
	st_xmysqlnd_on_error_bind on_error = {
		prepare_st_on_error_handler,
		static_cast<void*>(this)
	};
	prepare_prepare.init_read(&prepare_prepare, on_error);
	prepare_prepare.read_response(&prepare_prepare);
	return ps_supported;
}

void Prepare_stmt_data::add_limit_expr_mutable_arg(
			Mysqlx::Prepare::Execute& execute_msg,
			const int32_t value
)
{
	Mysqlx::Datatypes::Scalar * scalar;
	Mysqlx::Datatypes::Any * any;

	scalar = new Mysqlx::Datatypes::Scalar;
	any = new Mysqlx::Datatypes::Any;

	any->set_type( Mysqlx::Datatypes::Any_Type::Any_Type_SCALAR );
	scalar->set_type( Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_SINT );
	scalar->set_v_signed_int( value );
	any->set_allocated_scalar(scalar);
	execute_msg.mutable_args()->AddAllocated(any);
}

xmysqlnd_stmt *
Prepare_stmt_data::send_execute_msg(
			uint32_t message_id
)
{
	size_t db_idx = get_ps_entry( message_id );
	if( db_idx > ps_db.size() || ps_db[ db_idx ].delivered_ps == false ) {
		return nullptr;
	}
	xmysqlnd_stmt * stmt{ nullptr };
	Mysqlx::Prepare::Execute execute_msg;
	execute_msg.set_stmt_id( message_id );
	auto& ps_entry = ps_db[ db_idx ];

	const Mysqlx::Datatypes::Scalar* null_value{nullptr};
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator begin = ps_entry.bound_values.begin();
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator end = ps_entry.bound_values.end();
	const std::vector<Mysqlx::Datatypes::Scalar*>::const_iterator index = std::find(begin, end, null_value);
	execute_msg.clear_args();
	if (index == end) {
		std::vector<Mysqlx::Datatypes::Scalar*>::iterator it = begin;
		for (; it != end; ++it) {
            Mysqlx::Datatypes::Any*  any = new Mysqlx::Datatypes::Any;
            Mysqlx::Datatypes::Scalar* scalar = new Mysqlx::Datatypes::Scalar;
			scalar->CopyFrom(**it);
            any->set_type( Mysqlx::Datatypes::Any_Type::Any_Type_SCALAR );
            any->set_allocated_scalar( scalar );
            execute_msg.mutable_args()->AddAllocated(any);
		}
	}

	if( ps_entry.has_row_count ) {
		add_limit_expr_mutable_arg( execute_msg, static_cast<int32_t>(ps_entry.row_count) );
	}
	if( ps_entry.has_offset ) {
		add_limit_expr_mutable_arg( execute_msg, static_cast<int32_t>(ps_entry.offset) );
	}

	st_xmysqlnd_message_factory msg_factory{ session->data->create_message_factory() };
	st_xmysqlnd_msg__prepare_execute prepare_execute = msg_factory.get__prepare_execute(&msg_factory);
	enum_func_status request_ret = prepare_execute.send_execute_request(&prepare_execute,
										get_protobuf_msg(&execute_msg,COM_PREPARE_EXECUTE));
	if( PASS == request_ret ) {
		stmt = session->create_statement_object(session);
		stmt->get_msg_stmt_exec() = msg_factory.get__sql_stmt_execute(&msg_factory);
	}
	return stmt;
}

bool Prepare_stmt_data::bind_values(
			uint32_t message_id,
			std::vector<Mysqlx::Datatypes::Scalar*> bound_values
)
{
	size_t db_idx = get_ps_entry( message_id );
	if( db_idx > ps_db.size() ) {
		return false;
	}
	ps_db[ db_idx ].bound_values = std::move(bound_values);
	return true;
}

bool Prepare_stmt_data::bind_values(
			uint32_t message_id,
			zval* params,
			unsigned int params_allocated
)
{
	enum_func_status ret{PASS};
	std::vector<Mysqlx::Datatypes::Scalar*> converted_params;
	for( unsigned int i{0}; i < params_allocated; ++i ) {
		Mysqlx::Datatypes::Any arg;
		ret = zval2any(&(params[i]), arg);
		if( FAIL == ret ) {
			break;
		}
		Mysqlx::Datatypes::Scalar * new_param = new Mysqlx::Datatypes::Scalar;
		new_param->CopyFrom(arg.scalar());
		converted_params.push_back( new_param );
	}
	return ret == PASS;
}

bool Prepare_stmt_data::prepare_msg_delivered( const uint32_t message_id )
{
	size_t db_idx = get_ps_entry( message_id );
	if( db_idx > ps_db.size() ) {
		return false;
	}
	return ps_db[ db_idx ].delivered_ps;
}

void Prepare_stmt_data::set_supported_ps( bool supported )
{
	ps_supported = supported;
}

bool Prepare_stmt_data::is_ps_supported() const
{
	return ps_supported;
}

bool Prepare_stmt_data::is_bind_finalized( const uint32_t message_id )
{
	auto idx = get_ps_entry( message_id );
	if( idx > ps_db.size() ) {
		return false;
	}
	return ps_db[ idx ].is_bind_finalized;
}

void Prepare_stmt_data::set_finalized_bind(
			const uint32_t message_id,
			const bool finalized
)
{
	const auto idx = get_ps_entry( message_id );
	if( idx > ps_db.size() ) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error);
	}
	ps_db[ idx ].is_bind_finalized = finalized;
}

const enum_hnd_func_status prepare_st_on_error_handler(void * context,
												 const unsigned int code,
												 const util::string_view& sql_state,
												 const util::string_view& message)
{
	DBG_ENTER("prepare_st_on_error_handler");
	static const uint32_t unknown_message_code { 1047 };
	Prepare_stmt_data* const ctx = static_cast< Prepare_stmt_data* >(context);
    ctx->set_ps_server_error( code );
	if( code == unknown_message_code ) {
		DBG_INF_FMT("Disabling support for prepare statement, not supported by server.");
		ctx->set_supported_ps( false );
	} else {
		mysqlx::devapi::mysqlx_new_exception(code, sql_state, message);
		DBG_RETURN(HND_PASS_RETURN_FAIL);
	}

	DBG_RETURN(HND_PASS);
}

template<>
void Prepare_stmt_data::handle_limit_expr(
			Prepare_statement_entry& prepare,
			Mysqlx::Crud::Insert* msg,
			uint32_t bound_values_count
)
{
	//Nothing to do here
}

template< typename MSG_T >
void common_handle_limit_expr(
			Prepare_statement_entry& prepare,
			MSG_T * msg,
			uint32_t bound_values_count
)
{
	if( msg->has_limit() ) {
		Mysqlx::Expr::Expr *      row_count_expr;
		Mysqlx::Expr::Expr *      offset_expr;
		Mysqlx::Crud::LimitExpr * limit_expr;
		try{
			limit_expr = new Mysqlx::Crud::LimitExpr;
		} catch( std::bad_alloc& /*xa*/ ) {
			throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error);
		}
		if( msg->limit().has_row_count() ) {
			prepare.row_count = msg->limit().row_count();
			prepare.has_row_count = true;
			try{
				row_count_expr = new Mysqlx::Expr::Expr;
			} catch( std::bad_alloc& /*xa*/ ) {
				throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error);
			}
			row_count_expr->set_type( Mysqlx::Expr::Expr_Type::Expr_Type_PLACEHOLDER );
			row_count_expr->set_position( bound_values_count++ );
			limit_expr->set_allocated_row_count( row_count_expr );
		}
		if( msg->limit().has_offset()) {
			prepare.offset = msg->limit().offset();
			prepare.has_offset = true;
			try{
				offset_expr = new Mysqlx::Expr::Expr;
			} catch( std::bad_alloc& /*xa*/ ) {
				throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error);
			}
			offset_expr->set_type( Mysqlx::Expr::Expr_Type::Expr_Type_PLACEHOLDER );
			offset_expr->set_position( bound_values_count );
			limit_expr->set_allocated_offset( offset_expr );
		}
		msg->clear_limit();
		msg->set_allocated_limit_expr( limit_expr );
	}
}

template<>
void Prepare_stmt_data::handle_limit_expr(
			Prepare_statement_entry& prepare,
			Mysqlx::Crud::Update* msg,
			uint32_t bound_values_count
)
{
	common_handle_limit_expr( prepare, msg, bound_values_count );
}

template<>
void Prepare_stmt_data::handle_limit_expr(
			Prepare_statement_entry& prepare,
			Mysqlx::Crud::Find* msg,
			uint32_t bound_values_count
)
{
	common_handle_limit_expr( prepare, msg, bound_values_count );
}

template<>
void Prepare_stmt_data::handle_limit_expr(
			Prepare_statement_entry& prepare,
			Mysqlx::Crud::Delete* msg,
			uint32_t bound_values_count
)
{
	common_handle_limit_expr( prepare, msg, bound_values_count );
}

template<>
void Prepare_stmt_data::handle_limit_expr(
            Prepare_statement_entry& prepare,
            Mysqlx::Sql::StmtExecute* msg,
            uint32_t bound_values_count
)
{
    //Nothing to do here.
}

} // namespace drv

} // namespace mysqlx
