/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_rowset.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_wireprotocol.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_node_stmt::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, init)(XMYSQLND_NODE_STMT * const stmt,
										  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										  XMYSQLND_NODE_SESSION * const session,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt::init");
	if (!(stmt->data->session = session->m->get_reference(session))) {
		return FAIL;
	}
	stmt->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::send_raw_message */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, send_raw_message)(XMYSQLND_NODE_STMT * const stmt,
													  const struct st_xmysqlnd_pb_message_shell message_shell,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info)
{
	MYSQLND_VIO * vio = stmt->data->session->data->io.vio;
	XMYSQLND_PFC * pfc = stmt->data->session->data->io.pfc;
	const XMYSQLND_L3_IO io = {vio, pfc};
	/* pass stmt->data->session->data->io directly ?*/
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&io, stats, error_info);
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt::send_raw_message");

	stmt->data->partial_read_started = FALSE;
	stmt->data->msg_stmt_exec = msg_factory.get__collection_read(&msg_factory);

	ret = stmt->data->msg_stmt_exec.send_execute_request(&stmt->data->msg_stmt_exec, message_shell);

	DBG_INF_FMT("send_request returned %s", PASS == ret? "PASS":"FAIL");

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::create_rowset_fwd */
static XMYSQLND_ROWSET *
XMYSQLND_METHOD(xmysqlnd_node_stmt, create_rowset_fwd)(void * context)
{
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	XMYSQLND_ROWSET * result;
	DBG_ENTER("xmysqlnd_node_stmt::create_rowset_fwd");
	result = xmysqlnd_rowset_create(XMYSQLND_TYPE_ROWSET_FWD_ONLY, ctx->fwd_prefetch_count, ctx->stmt, ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::create_rowset_buffered */
static XMYSQLND_ROWSET *
XMYSQLND_METHOD(xmysqlnd_node_stmt, create_rowset_buffered)(void * context)
{
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	XMYSQLND_ROWSET * result;
	DBG_ENTER("xmysqlnd_node_stmt::create_rowset_buffered");
	result = xmysqlnd_rowset_create(XMYSQLND_TYPE_ROWSET_BUFFERED, (size_t)~0, ctx->stmt, ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::create_meta */
static XMYSQLND_NODE_STMT_RESULT_META *
XMYSQLND_METHOD(xmysqlnd_node_stmt, create_meta)(void * context)
{
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	XMYSQLND_NODE_STMT_RESULT_META * meta;
	DBG_ENTER("xmysqlnd_node_stmt::create_meta");
	meta = xmysqlnd_node_stmt_result_meta_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(meta);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::create_meta_field */
static XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_node_stmt, create_meta_field)(void * context)
{
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	XMYSQLND_RESULT_FIELD_META * field;
	DBG_ENTER("xmysqlnd_node_stmt::create_meta_field");
	field = xmysqlnd_result_field_meta_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(field);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_meta_field */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_row_field)(void * context,
														  const MYSQLND_CSTRING buffer,
														  const unsigned int idx,
														  const func_xmysqlnd_wireprotocol__row_field_decoder decoder)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_row_field");
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
				DBG_INF_FMT("fwd_prefetch_count="MYSQLND_LLU_SPEC" prefetch_counter="MYSQLND_LLU_SPEC, ctx->fwd_prefetch_count, ctx->prefetch_counter);
				ctx->rowset->m.add_row(ctx->rowset, ctx->current_row, ctx->stats, ctx->error_info);
				if (ctx->fwd_prefetch_count && !--ctx->prefetch_counter) {
					ret = HND_PASS; /* Otherwise it is HND_AGAIN */
				}
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_meta_field */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_meta_field)(void * context, struct st_xmysqlnd_result_field_meta * field)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_meta_field");
	DBG_INF_FMT("rowset=%p  meta=%p", ctx->rowset, ctx->meta);
	if (!ctx->meta) {
		ctx->meta = xmysqlnd_node_stmt_result_meta_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->meta) {
		ctx->meta->m->add_field(ctx->meta, field, ctx->stats, ctx->error_info);
	}
	DBG_INF_FMT("rowset=%p  meta=%p", ctx->rowset, ctx->meta);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_warning */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_warning)(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_warning");
	if (ctx->on_warning.handler) {
		ret = ctx->on_warning.handler(ctx->on_warning.ctx, ctx->stmt, level, code, message);
	}
	if (!ctx->warnings) {
		ctx->warnings = xmysqlnd_warning_list_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->warnings) {
		ctx->warnings->m->add_warning(ctx->warnings, level, code, message);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_error */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_error)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_PASS_RETURN_FAIL;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_error");
	if (ctx->on_error.handler) {
		ret = ctx->on_error.handler(ctx->on_error.ctx, ctx->stmt, code, sql_state, message);
	} else if (ctx->error_info) {
		SET_CLIENT_ERROR(ctx->error_info, code, sql_state.s, message.s);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_exec_state_change */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_exec_state_change)(void * context, const enum xmysqlnd_execution_state_type type, const size_t value)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_exec_state_change");
	if (!ctx->exec_state) {
		ctx->exec_state = xmysqlnd_stmt_execution_state_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
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
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_trx_state_change */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_trx_state_change)(void * context, const enum xmysqlnd_transaction_state_type type)
{
#if 0
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
#endif
	const enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_trx_state_change");
	DBG_INF_FMT("type=%s", type == TRX_STATE_COMMITTED? "COMMITTED":(type == TRX_STATE_ROLLEDBACK? "ROLLED BACK":"n/a"));
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_resultset_end */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_resultset_end)(void * context, const zend_bool has_more)
{
	enum_hnd_func_status ret = HND_AGAIN;
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_resultset_end");
	DBG_INF_FMT("on_resultset_end.handler=%p", ctx->on_resultset_end.handler);
	if (ctx->on_resultset_end.handler) {
		ret = ctx->on_resultset_end.handler(ctx->on_resultset_end.ctx, ctx->stmt, has_more);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_statement_ok */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_statement_ok)(void * context)
{
	enum_hnd_func_status ret = HND_PASS;
	const struct st_xmysqlnd_node_stmt_bind_ctx * const ctx = (const struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_statement_ok");
	DBG_INF_FMT("on_statement_ok.handler=%p", ctx->on_statement_ok.handler);
	if (ctx->on_statement_ok.handler) {
		ret = ctx->on_statement_ok.handler(ctx->on_row.ctx, ctx->stmt, ctx->exec_state);
	}
	DBG_RETURN(ret);
}
/* }}} */



/* {{{ xmysqlnd_node_stmt::read_one_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, read_one_result)(XMYSQLND_NODE_STMT * const stmt,
													 const struct st_xmysqlnd_node_stmt_on_row_bind on_row,
													 const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
													 const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
													 const struct st_xmysqlnd_node_stmt_on_result_end_bind on_resultset_end,
													 const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok,
													 zend_bool * const has_more_results,
													 MYSQLND_STATS * const stats,
													 MYSQLND_ERROR_INFO * const error_info)
{
	struct st_xmysqlnd_node_stmt_bind_ctx create_ctx =
	{
		stmt,
		stats,
		error_info,
		stmt->data->m.create_rowset_buffered,
		0,		/* fwd_prefetch_count */
		0,		/* prefetch_counter */
		NULL,	/* current_row */
		NULL,	/* rowset */
		NULL,	/* meta */
		NULL,	/* result */
		NULL,	/* warnings */
		NULL,	/* exec_state */
		on_row,
		on_warning,
		on_error,
		on_resultset_end,
		on_statement_ok,
	};
	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = { stmt->data->m.create_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_row_field_bind handler_on_row_field_msg = { on_row.handler? stmt->data->m.handler_on_row_field : NULL, &create_ctx };
	const struct st_xmysqlnd_on_meta_field_bind handler_on_meta_field_msg = { stmt->data->m.handler_on_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_warning_bind handler_on_warning_msg = { on_warning.handler? stmt->data->m.handler_on_warning : NULL, &create_ctx };
	const struct st_xmysqlnd_on_error_bind handler_on_error_msg = { on_error.handler? stmt->data->m.handler_on_error : NULL , on_error.handler? &create_ctx : NULL };
	const struct st_xmysqlnd_on_execution_state_change_bind handler_on_exec_state_change = { stmt->data->m.handler_on_exec_state_change, &create_ctx };
	const struct st_xmysqlnd_on_session_var_change_bind handler_on_session_var_change = { NULL, NULL };
	const struct st_xmysqlnd_on_trx_state_change_bind handler_on_trx_state_change = { stmt->data->m.handler_on_trx_state_change, &create_ctx };
	const struct st_xmysqlnd_on_stmt_execute_ok_bind handler_on_stmt_execute_ok = { on_resultset_end.handler? stmt->data->m.handler_on_statement_ok : NULL, &create_ctx };
	const struct st_xmysqlnd_on_resultset_end_bind handler_on_resultset_end = { on_statement_ok.handler? stmt->data->m.handler_on_resultset_end : NULL, &create_ctx };

	DBG_ENTER("xmysqlnd_node_stmt::read_one_result");
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
	if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
													create_meta_field,
													handler_on_row_field_msg,
													handler_on_meta_field_msg,
													handler_on_warning_msg,
													handler_on_error_msg,
													handler_on_exec_state_change,
													handler_on_session_var_change,
													handler_on_trx_state_change,
													handler_on_stmt_execute_ok,
													handler_on_resultset_end)) {
		DBG_RETURN(FAIL);
	}

	if (FAIL == stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, NULL)) {
		DBG_RETURN(FAIL);
	}
	*has_more_results = stmt->data->msg_stmt_exec.reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	if (create_ctx.rowset) {
		xmysqlnd_rowset_free(create_ctx.rowset, stats, error_info);
		create_ctx.rowset = NULL;
	}
	if (create_ctx.meta) {
		xmysqlnd_node_stmt_result_meta_free(create_ctx.meta, stats, error_info);
		create_ctx.meta = NULL;
	}
	if (create_ctx.exec_state) {
		xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
		create_ctx.exec_state = NULL;
	}
	if (create_ctx.warnings) {
		xmysqlnd_warning_list_free(create_ctx.warnings);
		create_ctx.warnings = NULL;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::read_all_results */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, read_all_results)(XMYSQLND_NODE_STMT * const stmt,
													  const struct st_xmysqlnd_node_stmt_on_row_bind on_row,
													  const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
													  const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
													  const struct st_xmysqlnd_node_stmt_on_result_start_bind on_result_start,
													  const struct st_xmysqlnd_node_stmt_on_result_end_bind on_resultset_end,
													  const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	zend_bool has_more;
	DBG_ENTER("xmysqlnd_node_stmt::read_all_results");
	do {
		has_more = FALSE;
		if (on_result_start.handler) {
			on_result_start.handler(on_result_start.ctx, stmt);
		}
		ret = stmt->data->m.read_one_result(stmt, on_row,
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
/* }}} */


/* {{{ xmysqlnd_node_stmt::has_more_results */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt, has_more_results)(const XMYSQLND_NODE_STMT * const stmt)
{
	DBG_ENTER("xmysqlnd_node_stmt::has_more_results");
	DBG_INF_FMT("has_more=%s", stmt->data->msg_stmt_exec.reader_ctx.has_more_results? "TRUE":"FALSE");
	DBG_RETURN(stmt->data->msg_stmt_exec.reader_ctx.has_more_results);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_buffered_result */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_buffered_result)(XMYSQLND_NODE_STMT * const stmt,
														 zend_bool * const has_more_results,
														 const struct st_xmysqlnd_node_stmt_on_warning_bind handler_on_warning,
														 const struct st_xmysqlnd_node_stmt_on_error_bind handler_on_error,
														 MYSQLND_STATS * const stats,
														 MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	struct st_xmysqlnd_node_stmt_bind_ctx create_ctx =
	{
		stmt,
		stats,
		error_info,
		stmt->data->m.create_rowset_buffered,
		0,		/* fwd_prefetch_count */
		0,		/* prefetch_counter */
		NULL,	/* current_row */
		NULL,	/* rowset */
		NULL,	/* meta */
		NULL,	/* result */
		NULL,	/* warnings */
		NULL,	/* exec_state */
		{ NULL, NULL },	/* on_row */
		handler_on_warning,
		handler_on_error,
		{ NULL, NULL },	/* on_resultset_end */
		{ NULL, NULL },	/* on_statement_ok */
	};

	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = {
		stmt->data->m.create_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_row_field_bind on_row_field = {
		stmt->data->m.handler_on_row_field, &create_ctx };
	const struct st_xmysqlnd_on_meta_field_bind on_meta_field = {
		stmt->data->m.handler_on_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_warning_bind on_warning = {
		handler_on_warning.handler? stmt->data->m.handler_on_warning : NULL, &create_ctx };
	const struct st_xmysqlnd_on_error_bind on_error = {
		(handler_on_error.handler || error_info) ? stmt->data->m.handler_on_error : NULL, &create_ctx };
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = {
		stmt->data->m.handler_on_exec_state_change, &create_ctx };
	const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change = { NULL, NULL };
	const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = {
		stmt->data->m.handler_on_trx_state_change, &create_ctx };
	const struct st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { NULL, NULL };
	const struct st_xmysqlnd_on_resultset_end_bind on_resultset_end = { NULL, NULL };
	DBG_ENTER("xmysqlnd_node_stmt::get_buffered_result");

	/*
	  Maybe we can inject a callbacks that creates `meta` on demand, but we still DI it.
	  This way we don't pre-create `meta` and in case of UPSERT we don't waste cycles.
	  For now, we just pre-create.
	*/
	if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
													create_meta_field,
													on_row_field,
													on_meta_field,
													on_warning,
													on_error,
													on_exec_state_change,
													on_session_var_change,
													on_trx_state_change,
													on_stmt_execute_ok,
													on_resultset_end)) {
		DBG_RETURN(NULL);
	}

	if (FAIL == stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, NULL)) {
		DBG_RETURN(NULL);
	}
	*has_more_results = stmt->data->msg_stmt_exec.reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	result = xmysqlnd_node_stmt_result_create(stmt->data->persistent, stmt->data->object_factory, stats, error_info);
	if (result) {
		result->m.attach_rowset(result, create_ctx.rowset, stats, error_info);
		result->m.attach_meta(result, create_ctx.meta, stats, error_info);
		result->m.attach_execution_state(result, create_ctx.exec_state);
		result->m.attach_warning_list(result, create_ctx.warnings);
	} else {
		if (create_ctx.rowset) {
			xmysqlnd_rowset_free(create_ctx.rowset, stats, error_info);
			create_ctx.rowset = NULL;
		}
		if (create_ctx.meta) {
			xmysqlnd_node_stmt_result_meta_free(create_ctx.meta, stats, error_info);
			create_ctx.meta = NULL;
		}
		if (create_ctx.exec_state) {
			xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
			create_ctx.exec_state = NULL;
		}
		if (create_ctx.warnings) {
			xmysqlnd_warning_list_free(create_ctx.warnings);
			create_ctx.warnings = NULL;
		}
	}
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_fwd_result */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_fwd_result)(XMYSQLND_NODE_STMT * const stmt,
													const size_t rows,
													zend_bool * const has_more_rows_in_set,
													zend_bool * const has_more_results,
													const struct st_xmysqlnd_node_stmt_on_warning_bind handler_on_warning,
													const struct st_xmysqlnd_node_stmt_on_error_bind handler_on_error,
													MYSQLND_STATS * const stats,
													MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = { stmt->data->m.create_meta_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_row_field_bind on_row_field = { stmt->data->m.handler_on_row_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_meta_field_bind on_meta_field = { stmt->data->m.handler_on_meta_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_warning_bind on_warning = { handler_on_warning.handler? stmt->data->m.handler_on_warning : NULL, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_error_bind on_error = { (handler_on_error.handler || error_info) ? stmt->data->m.handler_on_error : NULL, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { stmt->data->m.handler_on_exec_state_change, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change = { NULL, NULL };
	const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = { NULL, NULL };
	const struct st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { NULL, NULL };
	const struct st_xmysqlnd_on_resultset_end_bind on_resultset_end = { NULL, NULL };
	DBG_ENTER("xmysqlnd_node_stmt::get_fwd_result");
	DBG_INF_FMT("rows="MYSQLND_LLU_SPEC, rows);

	if (FALSE == stmt->data->partial_read_started) {
		stmt->data->read_ctx.stmt = stmt;
		stmt->data->read_ctx.stats = stats;
		stmt->data->read_ctx.error_info = error_info;
		stmt->data->read_ctx.create_rowset = stmt->data->m.create_rowset_fwd;
		stmt->data->read_ctx.current_row = NULL;
		stmt->data->read_ctx.rowset = NULL;
		stmt->data->read_ctx.meta = NULL;
		stmt->data->read_ctx.result = xmysqlnd_node_stmt_result_create(stmt->data->persistent, stmt->data->object_factory, stats, error_info);
		stmt->data->read_ctx.warnings = xmysqlnd_warning_list_create(stmt->persistent, stmt->data->object_factory, stats, error_info);
		stmt->data->read_ctx.exec_state = xmysqlnd_stmt_execution_state_create(stmt->persistent, stmt->data->object_factory, stats, error_info);
		stmt->data->read_ctx.on_warning = handler_on_warning;
		stmt->data->read_ctx.on_error = handler_on_error;

		if (!(result = stmt->data->read_ctx.result)) {
			DBG_RETURN(NULL);
		}
		result->m.attach_execution_state(result, stmt->data->read_ctx.exec_state);
		result->m.attach_warning_list(result, stmt->data->read_ctx.warnings);

		if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
														create_meta_field,
														on_row_field,
														on_meta_field,
														on_warning,
														on_error,
														on_exec_state_change,
														on_session_var_change,
														on_trx_state_change,
														on_stmt_execute_ok,
														on_resultset_end))
		{
			xmysqlnd_node_stmt_result_free(stmt->data->read_ctx.result, stats, error_info);
			DBG_RETURN(NULL);
		}
		stmt->data->partial_read_started = TRUE;
	}
	/*
	  We can't be sure about more rows in the set, so we speculate if rows == 0.
	  If rows > 0, then we will read at least 1 row and we will be sure
	*/
	*has_more_rows_in_set = TRUE;
	*has_more_results = FALSE;

	stmt->data->read_ctx.fwd_prefetch_count = rows;
	stmt->data->read_ctx.prefetch_counter = rows;

	if (rows) {
		if (FAIL == stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, NULL)) {
			DBG_RETURN(NULL);
		}
		*has_more_rows_in_set = stmt->data->msg_stmt_exec.reader_ctx.has_more_rows_in_set;
		*has_more_results = stmt->data->msg_stmt_exec.reader_ctx.has_more_results;
	}
	DBG_INF_FMT("current_rowset=%p  has_more_results=%s has_more_rows_in_set=%s",
				 stmt->data->read_ctx.rowset, *has_more_results? "TRUE":"FALSE", *has_more_rows_in_set? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", stmt->data->read_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", stmt->data->read_ctx.warnings);

	result = stmt->data->read_ctx.result;
	result->m.attach_rowset(result, stmt->data->read_ctx.rowset, stats, error_info);
	result->m.attach_meta(result, stmt->data->read_ctx.meta, stats, error_info);

	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::skip_one_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_one_result)(XMYSQLND_NODE_STMT * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	struct st_xmysqlnd_node_stmt_bind_ctx create_ctx = { stmt, stats, error_info };
	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = { NULL, NULL };
	const struct st_xmysqlnd_on_row_field_bind on_row_field = { NULL, NULL };
	const struct st_xmysqlnd_on_meta_field_bind on_meta_field = { NULL, NULL };
	const struct st_xmysqlnd_on_warning_bind on_warning = { NULL, NULL };
	const struct st_xmysqlnd_on_error_bind on_error = { NULL, NULL };
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { stmt->data->m.handler_on_exec_state_change, &create_ctx };
	const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change = { NULL, NULL };
	const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = { NULL, NULL };
	const struct st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok = { NULL, NULL };
	const struct st_xmysqlnd_on_resultset_end_bind on_resultset_end = { NULL, NULL };

	DBG_ENTER("xmysqlnd_node_stmt::skip_one_result");
	if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
													create_meta_field,
													on_row_field,
													on_meta_field,
													on_warning,
													on_error,
													on_exec_state_change,
													on_session_var_change,
													on_trx_state_change,
													on_stmt_execute_ok,
													on_resultset_end)) {
		DBG_RETURN(FAIL);
	}

	if (FAIL == stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, NULL)) {
		DBG_RETURN(FAIL);
	}
	*has_more_results = stmt->data->msg_stmt_exec.reader_ctx.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	if (create_ctx.exec_state) {
		xmysqlnd_stmt_execution_state_free(create_ctx.exec_state);
		create_ctx.exec_state = NULL;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::skip_all_results */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_all_results)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	zend_bool has_more;
	DBG_ENTER("xmysqlnd_node_stmt::skip_all_results");
	do {
		ret = stmt->data->m.skip_one_result(stmt, &has_more, stats, error_info);
	} while (PASS == ret && has_more == TRUE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_reference */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_reference)(XMYSQLND_NODE_STMT * const stmt)
{
	DBG_ENTER("xmysqlnd_node_stmt::get_reference");
	++stmt->data->refcount;
	DBG_INF_FMT("stmt=%p new_refcount=%u", stmt, stmt->data->refcount);
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, free_reference)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_stmt::free_reference");
	DBG_INF_FMT("stmt=%p old_refcount=%u", stmt, stmt->data->refcount);
	if (!(--stmt->data->refcount)) {
		stmt->data->m.dtor(stmt, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt, free_contents)(XMYSQLND_NODE_STMT * const stmt)
{
	DBG_ENTER("xmysqlnd_node_stmt::free_contents");
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt, dtor)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt::dtor");
	if (stmt) {
		stmt->data->m.free_contents(stmt);
		stmt->data->session->m->free_reference(stmt->data->session);

		mnd_pefree(stmt->data, stmt->data->persistent);
		mnd_pefree(stmt, stmt->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt)
	XMYSQLND_METHOD(xmysqlnd_node_stmt, init),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, send_raw_message),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, read_one_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, read_all_results),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, has_more_results),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, get_buffered_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, get_fwd_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_one_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_all_results),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, create_rowset_fwd),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, create_rowset_buffered),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, create_meta),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, create_meta_field),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_row_field),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_meta_field),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_warning),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_error),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_exec_state_change),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_trx_state_change),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_statement_ok),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_resultset_end),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, dtor),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_stmt);

/* {{{ xmysqlnd_node_stmt_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_STMT *
xmysqlnd_node_stmt_create(XMYSQLND_NODE_SESSION * session,
						  const zend_bool persistent,
						  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						  MYSQLND_STATS * const stats,
						  MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_create");
	stmt = object_factory->get_node_stmt(object_factory, session, persistent, stats, error_info);
	if (stmt) {
		stmt = stmt->data->m.get_reference(stmt);
	}
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_stmt_free(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_free");
	DBG_INF_FMT("stmt=%p  stmt->data=%p  dtor=%p", stmt, stmt? stmt->data:NULL, stmt? stmt->data->m.dtor:NULL);
	if (stmt) {
		if (!stats) {
			stats = stmt->data->session->data->stats;
		}
		if (!error_info) {
			error_info = stmt->data->session->data->error_info;
		}
		stmt->data->m.free_reference(stmt, stats, error_info);
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
