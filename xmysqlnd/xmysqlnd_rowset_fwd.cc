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
#include "xmysqlnd_rowset_fwd.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_rowset_fwd::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, init)(XMYSQLND_ROWSET_FWD * const result,
										   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
										   const size_t prefetch_rows,
										   XMYSQLND_NODE_STMT * const stmt,
										   MYSQLND_STATS * const stats,
										   MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::init");
	result->stmt = stmt->data->m.get_reference(stmt);
	result->prefetch_rows = prefetch_rows;
	DBG_RETURN(result->stmt? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, next)(XMYSQLND_ROWSET_FWD * const result,
										   MYSQLND_STATS * const stats,
										   MYSQLND_ERROR_INFO * const error_info)
{
	const zend_bool no_more_on_the_line = !result->stmt->data->msg_stmt_exec.reader_ctx.has_more_rows_in_set;
	DBG_ENTER("xmysqlnd_rowset_fwd::next");
	DBG_INF_FMT("row_cursor="MYSQLND_LLU_SPEC"  row_count="MYSQLND_LLU_SPEC, result->row_cursor, result->row_count);

	if ((result->row_count - result->row_cursor) == 1 && !no_more_on_the_line) {
		DBG_INF_FMT("We have to prefetch %u row(s)", result->prefetch_rows);
		if (result->row_count) {
			/* Remove what we have */
			result->m.free_rows_contents(result, stats, error_info);
		}
		result->stmt->data->read_ctx.prefetch_counter = result->stmt->data->read_ctx.fwd_prefetch_count;
		/* read rows */
		if (FAIL == result->stmt->data->msg_stmt_exec.read_response(&result->stmt->data->msg_stmt_exec, NULL)) {
			DBG_RETURN(FAIL);
		}
	} else {
		++result->row_cursor;
		DBG_INF("No prefetch");
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::fetch_current */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_current)(XMYSQLND_ROWSET_FWD * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset_fwd::fetch_current");
	++result->total_fetched;
	ret = result->m.fetch_one(result, result->row_cursor, row, stats, error_info);
	DBG_INF_FMT("%s", PASS == ret? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_one)(XMYSQLND_ROWSET_FWD * const result,
												const size_t row_cursor,
												zval * row,
												MYSQLND_STATS * const stats,
												MYSQLND_ERROR_INFO * const error_info)
{
	const unsigned int field_count = result->meta->m->get_field_count(result->meta);
	const size_t row_count = result->row_count;
	DBG_ENTER("xmysqlnd_rowset_fwd::fetch_one");
	DBG_INF_FMT("row_cursor="MYSQLND_LLU_SPEC"  row_count="MYSQLND_LLU_SPEC, result->row_cursor, result->row_count);
	if (row_cursor >= row_count || !result->rows[row_cursor]) {
		DBG_RETURN(FAIL);
	}
	array_init_size(row, field_count);
	if (field_count) {
		zval * const row_cursor_zv = result->rows[row_cursor];
		unsigned int col = 0;
		for (;col < field_count; ++col) {
			const XMYSQLND_RESULT_FIELD_META * field_meta = result->meta->m->get_field(result->meta, col);
			zval * const zv = &row_cursor_zv[col];

			Z_TRY_ADDREF_P(zv);

			if (field_meta->zend_hash_key.is_numeric == FALSE) {
				zend_hash_update(Z_ARRVAL_P(row), field_meta->zend_hash_key.sname, zv);
			} else {
				zend_hash_index_update(Z_ARRVAL_P(row), field_meta->zend_hash_key.key, zv);
			}
		}
	}
	++result->total_fetched;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_all)(XMYSQLND_ROWSET_FWD * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	size_t row_cursor = 0;
	DBG_ENTER("xmysqlnd_rowset_fwd::fetch_all");

	/* read the rest. If this was the first, then we will prefetch everything, otherwise we will read whatever is left */
	if (FAIL == result->stmt->data->msg_stmt_exec.read_response(&result->stmt->data->msg_stmt_exec, NULL)) {
		DBG_RETURN(FAIL);
	}

	array_init_size(set, result->row_count);
	for (;row_cursor < result->row_count; ++row_cursor) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == result->m.fetch_one(result, row_cursor, &row, stats, error_info)) {
			zend_hash_next_index_insert(Z_ARRVAL_P(set), &row);
		}
	}

	if (result->row_count) {
		result->total_fetched += result->row_count;

		/* Remove what we have, as we don't need it anymore */
		result->m.free_rows_contents(result, stats, error_info);
	}
	DBG_INF_FMT("total_row_count="MYSQLND_LLU_SPEC, result->total_row_count);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::rewind */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, rewind)(XMYSQLND_ROWSET_FWD * const result)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::rewind");
	if (result->total_fetched == 0 && result->row_cursor == 0) {
		DBG_RETURN(PASS);
	} else {
		php_error_docref(NULL, E_WARNING, "rewind() not possible with a forward only result set. Use a buffered result instead");
		DBG_RETURN(FAIL);
	}
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, eof)(const XMYSQLND_ROWSET_FWD * const result)
{
	const zend_bool no_more_prefetched = result->row_cursor >= result->row_count;
	const zend_bool no_more_on_the_line = !result->stmt->data->msg_stmt_exec.reader_ctx.has_more_rows_in_set;
	DBG_ENTER("xmysqlnd_rowset_fwd::eof");
	DBG_INF_FMT("no_more_prefetched=%s", no_more_prefetched? "TRUE":"FALSE");
	DBG_INF_FMT("no_more_on_the_line=%s", no_more_on_the_line? "TRUE":"FALSE");
	DBG_INF_FMT("eof=%s", no_more_on_the_line && no_more_prefetched ? "TRUE":"FALSE");
	DBG_RETURN(no_more_on_the_line && no_more_prefetched ? TRUE:FALSE);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::create_row */
static zval *
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, create_row)(XMYSQLND_ROWSET_FWD * const result,
												 const XMYSQLND_NODE_STMT_RESULT_META * const meta,
												 MYSQLND_STATS * const stats,
												 MYSQLND_ERROR_INFO * const error_info)
{
	const unsigned int column_count = meta->m->get_field_count(meta);
	zval * row = static_cast<zval*>(mnd_pecalloc(column_count, sizeof(zval), result->persistent));
	DBG_ENTER("xmysqlnd_rowset_fwd::create_row");
	DBG_INF_FMT("row=%p", row);
	DBG_RETURN(row);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::destroy_row */
static void
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, destroy_row)(XMYSQLND_ROWSET_FWD * const result,
												  zval * row,
												  MYSQLND_STATS * const stats,
												  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::destroy_row");
	DBG_INF_FMT("row=%p", row);
	if (row) {
		mnd_pefree(row, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, add_row)(XMYSQLND_ROWSET_FWD * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::add_row");
	DBG_INF_FMT("row=%p", row);

	if (!result->rows || result->rows_allocated == result->row_count) {
		result->rows_allocated += result->prefetch_rows;
		result->rows = static_cast<zval**>(mnd_perealloc(result->rows, result->rows_allocated * sizeof(zval*), result->persistent));
	}

	if (row) {
		result->rows[result->row_count++] = row;
		++result->total_row_count;
	}

	DBG_INF_FMT("row_count=%u  rows_allocated=%u  total_row_count=%u", (uint) result->row_count,
																	   (uint) result->rows_allocated,
																	   (uint) result->total_row_count);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, get_row_count)(const XMYSQLND_ROWSET_FWD * const result)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::get_row_count");
	DBG_INF_FMT("count=%u", (uint) result->row_count);
	DBG_RETURN(result->row_count);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::attach_meta */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, attach_meta)(XMYSQLND_ROWSET_FWD * const result, XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::attach_meta");
	if (meta) {
		if (result->meta) {
			xmysqlnd_node_stmt_result_meta_free(result->meta, stats, error_info);
		}
		result->meta = meta;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::free_rows_contents */
static void
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_rows_contents)(XMYSQLND_ROWSET_FWD * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::free_rows_contents");
	DBG_INF_FMT("rows=%p  meta=%p", result->rows, result->meta);

	if (result->rows && result->meta) {
		const unsigned int col_count = result->meta->m->get_field_count(result->meta);
		unsigned int row;
		unsigned int col;

		DBG_INF_FMT("Freeing %u rows with %u columns each", result->row_count, col_count);

		for (row = 0; row < result->row_count; ++row) {
			for (col = 0; col < col_count; ++col) {
				zval_ptr_dtor(&(result->rows[row][col]));
			}
			result->m.destroy_row(result, result->rows[row], stats, error_info);
			result->rows[row] = NULL;
		}
		result->row_count = 0;
		result->row_cursor = 0;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::free_rows */
static void
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_rows)(XMYSQLND_ROWSET_FWD * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::free_rows");
	DBG_INF_FMT("rows=%p  meta=%p", result->rows, result->meta);

	if (result->rows) {
		const zend_bool pers = result->persistent;

		result->m.free_rows_contents(result, stats, error_info);

		mnd_pefree(result->rows, pers);
		result->rows = NULL;

		result->rows_allocated = 0;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_contents)(XMYSQLND_ROWSET_FWD * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::free_contents");

	result->m.free_rows(result, stats, error_info);

	if (result->meta) {
		result->meta = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_rowset_fwd, dtor)(XMYSQLND_ROWSET_FWD * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd::dtor");
	if (result) {
		result->m.free_contents(result, stats, error_info);
		if (result->stmt) {
			result->stmt->data->m.free_reference(result->stmt, stats, error_info);
		}

		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_rowset_fwd)
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, init),

	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, next),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_current),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_one),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, rewind),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, eof),

	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, create_row),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, destroy_row),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, add_row),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, get_row_count),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_rows_contents),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_rows),

	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, attach_meta),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, free_contents),
	XMYSQLND_METHOD(xmysqlnd_rowset_fwd, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_rowset_fwd);

/* {{{ xmysqlnd_rowset_fwd_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_ROWSET_FWD *
xmysqlnd_rowset_fwd_create(const size_t prefetch_rows,
						   XMYSQLND_NODE_STMT * stmt,
						   const zend_bool persistent,
						   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						   MYSQLND_STATS * stats,
						   MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_ROWSET_FWD * result = NULL;
	DBG_ENTER("xmysqlnd_rowset_fwd_create");
	result = object_factory->get_rowset_fwd(object_factory, prefetch_rows, stmt, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_rowset_fwd_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_rowset_fwd_free(XMYSQLND_ROWSET_FWD * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_fwd_free");
	DBG_INF_FMT("result=%p", result);
	if (result) {
		result->m.dtor(result, stats, error_info);
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
