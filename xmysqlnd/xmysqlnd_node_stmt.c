/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_rowset.h"


/* {{{ xmysqlnd_node_stmt::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, init)(XMYSQLND_NODE_STMT * const stmt,
										  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										  XMYSQLND_NODE_SESSION_DATA * const session,
										  const MYSQLND_CSTRING query,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt::init");
	if (!(stmt->data->session = session->m->get_reference(session))) {
		return FAIL;
	}
	stmt->data->query = mnd_dup_cstring(query, stmt->data->persistent);
	DBG_INF_FMT("query=[%d]%*s", stmt->data->query.l, stmt->data->query.l, stmt->data->query.s);

	stmt->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::bind_one_param */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, bind_one_param)(XMYSQLND_NODE_STMT * const stmt, const unsigned int param_no, zval * param_zv)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt::bind_one_param");
	if (!stmt->data->params || param_no >= stmt->data->params_allocated) {
		stmt->data->params = mnd_perealloc(stmt->data->params, (param_no + 1) * sizeof(zval), stmt->data->persistent);
		if (!stmt->data->params) {
			DBG_RETURN(FAIL);
		}
		/* Now we have a hole between the last allocated and the new param_no which is not zeroed. Zero it! */
		memset(&stmt->data->params[stmt->data->params_allocated], 0, (param_no - stmt->data->params_allocated + 1) * sizeof(zval));

		stmt->data->params_allocated = param_no + 1;
	}
	zval_ptr_dtor(&stmt->data->params[param_no]);

	ZVAL_COPY_VALUE(&stmt->data->params[param_no], param_zv);
	Z_TRY_ADDREF(stmt->data->params[param_no]);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::send_query */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, send_query)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const MYSQLND_CSTRING namespace_par = {"sql", sizeof("sql") - 1};
	MYSQLND_VIO * vio = stmt->data->session->io.vio;
	XMYSQLND_PFC * pfc = stmt->data->session->io.pfc;
	const XMYSQLND_L3_IO io = {vio, pfc};
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&io, stats, error_info);
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt::send_query");

	stmt->data->partial_read_started = FALSE;
	stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
	ret = stmt->data->msg_stmt_exec.send_request(&stmt->data->msg_stmt_exec,
												 namespace_par,
												 mnd_str2c(stmt->data->query),
												 FALSE,
												 stmt->data->params,
												 stmt->data->params_allocated);
	DBG_INF_FMT("send_request returned %s", PASS == ret? "PASS":"FAIL");

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::create_rowset_fwd */
static XMYSQLND_ROWSET *
XMYSQLND_METHOD(xmysqlnd_node_stmt, create_rowset_fwd)(void * context)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
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
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
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
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
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
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	XMYSQLND_RESULT_FIELD_META * field;
	DBG_ENTER("xmysqlnd_node_stmt::create_meta_field");
	field = xmysqlnd_result_field_meta_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	DBG_RETURN(field);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_meta_field */
static enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_row_field)(void * context, const MYSQLND_CSTRING buffer, const unsigned int idx, const func_xmysqlnd_wireprotocol__row_field_decoder decoder)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_row_field");
	DBG_INF_FMT("rowset=%p  meta=%p", ctx->rowset, ctx->meta);
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
//		ctx->current_row

		if ((idx + 1) == ctx->meta->m->get_field_count(ctx->meta)) {
			ctx->rowset->m.add_row(ctx->rowset, ctx->current_row, ctx->stats, ctx->error_info);
		}
//		ctx->meta->m->add_field(ctx->meta, field, ctx->stats, ctx->error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_meta_field */
static enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_meta_field)(void * context, struct st_xmysqlnd_result_field_meta * field)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
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
static enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_warning)(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_warning");
	if (!ctx->warnings) {
		ctx->warnings = xmysqlnd_warning_list_create(ctx->stmt->persistent, ctx->stmt->data->object_factory, ctx->stats, ctx->error_info);
	}
	if (ctx->warnings) {
		ctx->warnings->m->add_warning(ctx->warnings, level, code, message);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_warning */
static enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_on_exec_state_change)(void * context, const enum xmysqlnd_execution_state_type type, const size_t value)
{
	struct st_xmysqlnd_node_stmt_bind_ctx * ctx = (struct st_xmysqlnd_node_stmt_bind_ctx *) context;
	enum_hnd_func_status ret = HND_AGAIN;

	DBG_ENTER("xmysqlnd_node_stmt::handler_on_on_exec_state_change");
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


/* {{{ xmysqlnd_node_stmt::has_more_results */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt, has_more_results)(const XMYSQLND_NODE_STMT * const stmt)
{
	DBG_ENTER("xmysqlnd_node_stmt::has_more_results");
	DBG_INF_FMT("has_more=%s", stmt->data->msg_stmt_exec.has_more_results? "TRUE":"FALSE");
	DBG_RETURN(stmt->data->msg_stmt_exec.has_more_results);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_buffered_result */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_buffered_result)(XMYSQLND_NODE_STMT * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	struct st_xmysqlnd_node_stmt_bind_ctx create_ctx = { stmt, stats, error_info, stmt->data->m.create_rowset_buffered, 0, NULL, NULL, NULL, NULL, NULL };
	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = { stmt->data->m.create_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_row_field_bind on_row_field = { stmt->data->m.handler_on_row_field, &create_ctx };
	const struct st_xmysqlnd_on_meta_field_bind on_meta_field = { stmt->data->m.handler_on_meta_field, &create_ctx };
	const struct st_xmysqlnd_on_warning_bind on_warning = { stmt->data->m.handler_on_warning, &create_ctx };
	const struct st_xmysqlnd_on_error_bind on_error = { NULL, NULL };
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { stmt->data->m.handler_on_on_exec_state_change, &create_ctx };
	const struct st_xmysqlnd_on_session_variable_change_bind on_session_variable_change = { NULL, NULL };
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
													on_session_variable_change)) {
		DBG_RETURN(NULL);
	}

	stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, (size_t)~0, NULL);
	*has_more_results = stmt->data->msg_stmt_exec.has_more_results;
	DBG_INF_FMT("rowset     =%p  has_more=%s", create_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", create_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", create_ctx.warnings);

	result = xmysqlnd_node_stmt_result_create(stmt->data->persistent, stmt->data->object_factory, stats, error_info);
	if (result) {
		result->m.attach_rowset(result, create_ctx.rowset, stats, error_info);
		result->m.attach_execution_state(result, create_ctx.exec_state);
		result->m.attach_warning_list(result, create_ctx.warnings);
	} else {
		if (create_ctx.rowset) {
			xmysqlnd_rowset_free(create_ctx.rowset, stats, error_info);
			create_ctx.rowset = NULL;
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
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_fwd_result)(XMYSQLND_NODE_STMT * const stmt, const size_t rows, zend_bool * const has_more_rows_in_set, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	const struct st_xmysqlnd_meta_field_create_bind create_meta_field = { stmt->data->m.create_meta_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_row_field_bind on_row_field = { stmt->data->m.handler_on_row_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_meta_field_bind on_meta_field = { stmt->data->m.handler_on_meta_field, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_warning_bind on_warning = { stmt->data->m.handler_on_warning, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_error_bind on_error = { NULL, NULL };
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { stmt->data->m.handler_on_on_exec_state_change, &stmt->data->read_ctx };
	const struct st_xmysqlnd_on_session_variable_change_bind on_session_variable_change = { NULL, NULL };
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
		stmt->data->read_ctx.warnings = NULL;
		stmt->data->read_ctx.exec_state = NULL;

		if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
														create_meta_field,
														on_row_field,
														on_meta_field,
														on_warning,
														on_error,
														on_exec_state_change,
														on_session_variable_change)) {
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

	if (rows) {
		stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, rows, NULL);
		*has_more_rows_in_set = stmt->data->msg_stmt_exec.has_more_rows_in_set;
		*has_more_results = stmt->data->msg_stmt_exec.has_more_results;
	}
	DBG_INF_FMT("current_rowset         =%p  has_more=%s", stmt->data->read_ctx.rowset, *has_more_results? "TRUE":"FALSE");
	DBG_INF_FMT("exec_state =%p", stmt->data->read_ctx.exec_state);
	DBG_INF_FMT("warnings   =%p", stmt->data->read_ctx.warnings);

	result = xmysqlnd_node_stmt_result_create(stmt->data->persistent, stmt->data->object_factory, stats, error_info);
	if (result) {
		result->m.attach_rowset(result, stmt->data->read_ctx.rowset, stats, error_info);
		result->m.attach_execution_state(result, stmt->data->read_ctx.exec_state);
		result->m.attach_warning_list(result, stmt->data->read_ctx.warnings);
	} else {
		if (stmt->data->read_ctx.rowset) {
			xmysqlnd_rowset_free(stmt->data->read_ctx.rowset, stats, error_info);
			stmt->data->read_ctx.rowset = NULL;
		}
		if (stmt->data->read_ctx.exec_state) {
			xmysqlnd_stmt_execution_state_free(stmt->data->read_ctx.exec_state);
			stmt->data->read_ctx.exec_state = NULL;
		}
		if (stmt->data->read_ctx.warnings) {
			xmysqlnd_warning_list_free(stmt->data->read_ctx.warnings);
			stmt->data->read_ctx.warnings = NULL;
		}
	}

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
	const struct st_xmysqlnd_on_execution_state_change_bind on_exec_state_change = { stmt->data->m.handler_on_on_exec_state_change, &create_ctx };
	const struct st_xmysqlnd_on_session_variable_change_bind on_session_variable_change = { NULL, NULL };

	DBG_ENTER("xmysqlnd_node_stmt::skip_one_result");
	if (FAIL == stmt->data->msg_stmt_exec.init_read(&stmt->data->msg_stmt_exec,
													create_meta_field,
													on_row_field,
													on_meta_field,
													on_warning,
													on_error,
													on_exec_state_change,
													on_session_variable_change)) {
		DBG_RETURN(FAIL);
	}

	stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, (size_t)~0, NULL);
	*has_more_results = stmt->data->msg_stmt_exec.has_more_results;
	DBG_INF_FMT("has_more=%s", *has_more_results? "TRUE":"FALSE");
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
XMYSQLND_METHOD(xmysqlnd_node_stmt, free_reference)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
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
	const zend_bool pers = stmt->data->persistent;
	DBG_ENTER("xmysqlnd_node_stmt::free_contents");
	if (stmt->data->query.s) {
		mnd_pefree(stmt->data->query.s, pers);
		stmt->data->query.s = NULL;
	}
	if (stmt->data->params) {
		unsigned int i = 0;
		for (; i < stmt->data->params_allocated; ++i) {
			zval_ptr_dtor(&stmt->data->params[i]);
		}
		mnd_pefree(stmt->data->params, pers);
		stmt->data->params = NULL;
	}
	stmt->data->params_allocated = 0;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt, dtor)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
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

	XMYSQLND_METHOD(xmysqlnd_node_stmt, bind_one_param),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, send_query),

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
	XMYSQLND_METHOD(xmysqlnd_node_stmt, handler_on_on_exec_state_change),

	XMYSQLND_METHOD(xmysqlnd_node_stmt, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_stmt);

/* {{{ xmysqlnd_node_stmt_create */
PHPAPI XMYSQLND_NODE_STMT *
xmysqlnd_node_stmt_create(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING query, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_create");
	if (query.s && query.l) {
		stmt = object_factory->get_node_stmt(object_factory, session, query, persistent, stats, error_info);
		if (stmt) {
			stmt = stmt->data->m.get_reference(stmt);
		}
	}
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_free */
PHPAPI void
xmysqlnd_node_stmt_free(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_free");
	DBG_INF_FMT("stmt=%p  stmt->data=%p  dtor=%p", stmt, stmt? stmt->data:NULL, stmt? stmt->data->m.dtor:NULL);
	if (stmt) {
		if (!stats) {
			stats = stmt->data->session->stats;
		}
		if (!error_info) {
			error_info = stmt->data->session->error_info;
		}
		stmt->data->m.free_reference(stmt, stats, error_info);
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
