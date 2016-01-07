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

/* {{{ xmysqlnd_node_stmt_result::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init)(XMYSQLND_NODE_STMT_RESULT * const result,
												 MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
												 XMYSQLND_NODE_STMT * const stmt,
												 MYSQLND_STATS * const stats,
												 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::init");
	result->data->stmt = stmt->data->m.get_reference(stmt);
	result->data->warnings = xmysqlnd_warning_list_init(result->data->persistent, factory, stats, error_info);
	result->data->exec_state = xmysqlnd_stmt_execution_state_init(result->data->persistent, factory, stats, error_info);
	DBG_RETURN(result->data->stmt && result->data->warnings && result->data->exec_state? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::next");
	if (result->data->row_cursor >= result->data->row_count) {
		DBG_RETURN(FAIL);
	}
	++result->data->row_cursor;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_current)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	if (!result || !result->data) {
		DBG_INF("FAIL");
		DBG_RETURN(ret);	
	}
	ret = result->data->m.fetch_one(result, result->data->row_cursor, row, stats, error_info);
	DBG_INF_FMT("%s", PASS == ret? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one)(XMYSQLND_NODE_STMT_RESULT * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const unsigned int field_count = result->data->meta->m->get_field_count(result->data->meta);
	const size_t row_count = result->data->row_count;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	if (row_cursor >= row_count || !result->data->rows[row_cursor]) {
		DBG_RETURN(FAIL);
	}
	array_init_size(row, field_count);
	if (field_count) {
		zval * const row_cursor_zv = result->data->rows[row_cursor];
		unsigned int col = 0;
		for (;col < field_count; ++col) {
			const XMYSQLND_RESULT_FIELD_META * field_meta = result->data->meta->m->get_field(result->data->meta, col);
			zval * const zv = &row_cursor_zv[col];

			Z_TRY_ADDREF_P(zv);

			if (field_meta->zend_hash_key.is_numeric == FALSE) {
				zend_hash_update(Z_ARRVAL_P(row), field_meta->zend_hash_key.sname, zv);
			} else {
				zend_hash_index_update(Z_ARRVAL_P(row), field_meta->zend_hash_key.key, zv);
			}
		}
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const size_t row_count = result->data->row_count;
	size_t row_cursor = 0;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_all");
	array_init_size(set, row_count);
	for (;row_cursor < row_count; ++row_cursor) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == result->data->m.fetch_one(result, row_cursor, &row, stats, error_info)) {
			zend_hash_next_index_insert(Z_ARRVAL_P(set), &row);
		}
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::eof");
	DBG_INF_FMT("%s", result->data->row_cursor >= result->data->row_count? "TRUE":"FALSE");
	DBG_RETURN(result->data->row_cursor >= result->data->row_count? TRUE:FALSE);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::create_row */
static zval *
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, create_row)(XMYSQLND_NODE_STMT_RESULT * const result,
													   const XMYSQLND_NODE_STMT_RESULT_META * const meta,
													   MYSQLND_STATS * const stats,
													   MYSQLND_ERROR_INFO * const error_info)
{
	const unsigned int column_count = meta->m->get_field_count(meta);
	zval * row = mnd_pecalloc(column_count, sizeof(zval), result->data->persistent);
	DBG_ENTER("xmysqlnd_node_stmt_result::create_row");
	DBG_RETURN(row);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::destroy_row */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, destroy_row)(XMYSQLND_NODE_STMT_RESULT * const result,
														zval * row,
														MYSQLND_STATS * const stats,
														MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::destroy_row");
	if (row) {
		mnd_pefree(row, result->data->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const XMYSQLND_NODE_STMT_RESULT_META * const meta = result->data->meta;
	const unsigned int column_count = meta->m->get_field_count(meta);
	unsigned int i = 0;
	DBG_ENTER("xmysqlnd_node_stmt_result::add_row");
	if (!result->data->rows || result->data->rows_allocated == result->data->row_count) {
		result->data->rows_allocated = ((result->data->rows_allocated + 2) * 5)/ 3;
		result->data->rows = mnd_perealloc(result->data->rows, result->data->rows_allocated * sizeof(zval*), result->data->persistent);
	}
	if (row) {
		result->data->rows[result->data->row_count++] = row;
		for (; i < column_count; i++) {
			zval_ptr_dtor(&(row[i]));
		}
	}
	DBG_INF_FMT("row_count=%u  rows_allocated=%u", (uint) result->data->row_count, (uint) result->data->rows_allocated);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::empty_rows */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, empty_rows)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::empty_rows");

	result->data->m.free_rows_contents(result, stats, error_info);
	DBG_INF_FMT("row_count=%u  rows_allocated=%u", (uint) result->data->row_count, (uint) result->data->rows_allocated);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_row_count");
	DBG_RETURN(result->data->row_count);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::attach_meta */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_meta)(XMYSQLND_NODE_STMT_RESULT * const result, XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::attach_meta");
	if (meta) {
		if (result->data->meta) {
			xmysqlnd_node_stmt_result_meta_free(result->data->meta, stats, error_info);
		}
		result->data->meta = meta;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows_contents");
	DBG_INF_FMT("rows=%p  meta=%p", result->data->rows, result->data->meta);

	if (result->data->rows && result->data->meta) {
		const unsigned int col_count = result->data->meta->m->get_field_count(result->data->meta);
		unsigned int row;
		unsigned int col;

		DBG_INF_FMT("Freeing %u rows with %u columns each", result->data->row_count, col_count);

		for (row = 0; row < result->data->row_count; ++row) {
			for (col = 0; col < col_count; ++col) {
				zval_ptr_dtor(&(result->data->rows[row][col]));
			}
			result->data->m.destroy_row(result, result->data->rows[row], stats, error_info);
			result->data->rows[row] = NULL;
		}
		result->data->row_count = 0;
		result->data->row_cursor = 0;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows");
	DBG_INF_FMT("rows=%p  meta=%p", result->data->rows, result->data->meta);

	if (result->data->rows) {
		const zend_bool pers = result->data->persistent;

		result->data->m.free_rows_contents(result, stats, error_info);

		mnd_pefree(result->data->rows, pers);
		result->data->rows = NULL;

		result->data->rows_allocated = 0;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_contents");

	result->data->m.free_rows(result, stats, error_info);

	if (result->data->meta) {
		xmysqlnd_node_stmt_result_meta_free(result->data->meta, stats, error_info);
		result->data->meta = NULL;
	}

	if (result->data->warnings) {
		xmysqlnd_warning_list_free(result->data->warnings);
		result->data->warnings = NULL;
	}
	if (result->data->exec_state) {
		xmysqlnd_stmt_execution_state_free(result->data->exec_state);
		result->data->exec_state = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, dtor)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::dtor");
	if (result) {
		result->data->m.free_contents(result, stats, error_info);
		if (result->data->stmt) {
			result->data->stmt->data->m.free_reference(result->data->stmt, stats, error_info);
		}

		mnd_pefree(result->data, result->data->persistent);
		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt_result)
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_current),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, create_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, destroy_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_meta),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, dtor),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_stmt_result_init */
PHPAPI XMYSQLND_NODE_STMT_RESULT *
xmysqlnd_node_stmt_result_init(XMYSQLND_NODE_STMT * stmt, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_result_init");
	result = factory->get_node_stmt_result(factory, stmt, persistent, stats, error_info);	
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_free */
PHPAPI void
xmysqlnd_node_stmt_result_free(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_free");
	DBG_INF_FMT("result=%p", result);
	if (result) {
		if (!stats) {
			stats = result->data->stmt->data->session->stats;
		}
		if (!error_info) {
			error_info = result->data->stmt->data->session->error_info;
		}
		result->data->m.dtor(result, stats, error_info);
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
