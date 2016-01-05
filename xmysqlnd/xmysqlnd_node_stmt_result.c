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
#include "ext/mysqlnd/mysqlnd_connection.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "ext/mysqlnd/mysqlnd_wireprotocol.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"

/* {{{ xmysqlnd_node_stmt_result::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init)(XMYSQLND_NODE_STMT_RESULT * const result,
												 XMYSQLND_NODE_STMT * const stmt,
												 MYSQLND_STATS * const stats,
												 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::init");
	if (!(result->data->stmt = stmt->data->m.get_reference(stmt))) {
		return FAIL;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_affected_items_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_affected_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_affected_items_count");
	DBG_RETURN(result->data->affected_items);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_last_insert_id */
static int64_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_last_insert_id)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_last_insert_id");
	DBG_RETURN(result->data->last_insert_id);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_warning_count */
static unsigned int
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_warning_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_warning_count");
	DBG_RETURN(result->data->warning_count);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_warning */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_warning)(const XMYSQLND_NODE_STMT_RESULT * const result, unsigned int offset)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_warning");
	DBG_RETURN(offset < result->data->warning_count? result->data->warnings[offset]:NULL);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_affected_items_count");
	DBG_RETURN(result->data->row_count);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::has_data */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, has_data)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::has_data");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::next");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_all");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::eof");
	DBG_RETURN(PASS);
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


#include <ext/standard/php_var.h> /* var_dump() */

/* {{{ xmysqlnd_node_stmt_result::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, const XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const unsigned int column_count = meta->m->get_field_count(meta);
	unsigned int i = 0;
	DBG_ENTER("xmysqlnd_node_stmt_result::add_row");
	if (!result->data->rows || result->data->rows_allocated == result->data->row_count) {
		result->data->rows_allocated = ((result->data->rows_allocated + 2) * 5)/ 3;
		result->data->rows = mnd_perealloc(result->data->rows, result->data->rows_allocated * sizeof(zval), result->data->persistent);
	}
	if (row) {
		result->data->rows[result->data->row_count++] = row;
		for (; i < column_count; i++) {
			php_var_dump(&(row[i]), 1);
			zval_ptr_dtor(&(row[i]));
		}
	}
	DBG_INF_FMT("row_count=%u  rows_allocated=%u", (uint) result->data->row_count, (uint) result->data->rows_allocated);
	DBG_RETURN(PASS);
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


/* {{{ xmysqlnd_node_stmt_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_contents");
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
		}
		mnd_pefree(result->data->rows, result->data->persistent);
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
		result->data->stmt->data->m.free_reference(result->data->stmt, stats, error_info);

		mnd_pefree(result->data, result->data->persistent);
		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt_result)
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_affected_items_count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_last_insert_id),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_warning_count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_warning),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, has_data),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, create_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, destroy_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row),
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
	result = factory->get_node_stmt_result(stmt, persistent, stats, error_info);	
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
