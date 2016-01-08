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
#include "xmysqlnd_node_stmt_result_buffered.h"
#include "xmysqlnd_node_stmt_result_fwd.h"
#include "xmysqlnd_node_stmt_result_meta.h"


/* {{{ xmysqlnd_node_stmt_result::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init)(XMYSQLND_NODE_STMT_RESULT * const result,
												 MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
												 enum xmysqlnd_result_type type,
												 const size_t prefetch_rows,
												 XMYSQLND_NODE_STMT * const stmt,
												 MYSQLND_STATS * const stats,
												 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::init");
	if (type == XMYSQLND_RESULT_FWD_ONLY) {
		result->fwd = xmysqlnd_node_stmt_result_fwd_init(prefetch_rows, stmt, result->persistent, factory, stats, error_info);
	} else {
		result->buffered = xmysqlnd_node_stmt_result_buffered_init(stmt, result->persistent, factory, stats, error_info);
	}
	result->warnings = xmysqlnd_warning_list_init(result->persistent, factory, stats, error_info);
	result->exec_state = xmysqlnd_stmt_execution_state_init(result->persistent, factory, stats, error_info);
	DBG_RETURN((result->buffered || result->fwd) && result->warnings && result->exec_state? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::next");
	if (result->fwd) {
		ret = result->fwd->m.next(result->fwd, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.next(result->buffered, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_current)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	if (result->fwd) {
		ret = result->fwd->m.fetch_current(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_current(result->buffered, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one)(XMYSQLND_NODE_STMT_RESULT * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	if (result->fwd) {
		ret = result->fwd->m.fetch_one(result->fwd, row_cursor, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_one(result->buffered, row_cursor, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_all");
	if (result->fwd) {
		ret = result->fwd->m.fetch_all(result->fwd, set, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_all(result->buffered, set, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	zend_bool ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::eof");
	if (result->fwd) {
		ret = result->fwd->m.eof(result->fwd);
	} else if (result->buffered) {
		ret = result->buffered->m.eof(result->buffered);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::create_row */
static zval *
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, create_row)(XMYSQLND_NODE_STMT_RESULT * const result,
													   const XMYSQLND_NODE_STMT_RESULT_META * const meta,
													   MYSQLND_STATS * const stats,
													   MYSQLND_ERROR_INFO * const error_info)
{
	zval * ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::create_row");
	if (result->fwd) {
		ret = result->fwd->m.create_row(result->fwd, meta, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.create_row(result->buffered, meta, stats, error_info);
	}
	DBG_RETURN(ret);
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
	if (result->fwd) {
		result->fwd->m.destroy_row(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.destroy_row(result->buffered, row, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::add_row");
	if (result->fwd) {
		ret = result->fwd->m.add_row(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.add_row(result->buffered, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	size_t ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::get_row_count");
	if (result->fwd) {
		ret = result->fwd->m.get_row_count(result->fwd);
	} else if (result->buffered) {
		ret = result->buffered->m.get_row_count(result->buffered);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::attach_meta */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_meta)(XMYSQLND_NODE_STMT_RESULT * const result, XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt_result::attach_meta");
	if (result->fwd) {
		ret = result->fwd->m.attach_meta(result->fwd, meta, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.attach_meta(result->buffered, meta, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows_contents");
	if (result->fwd) {
		result->fwd->m.free_rows_contents(result->fwd, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.free_rows_contents(result->buffered, stats, error_info);	
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows");
	if (result->fwd) {
		result->fwd->m.free_rows(result->fwd, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.free_rows(result->buffered, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_contents");
	if (result->fwd) {
		xmysqlnd_node_stmt_result_fwd_free(result->fwd, stats, error_info);
	} else if (result->buffered) {
		xmysqlnd_node_stmt_result_buffered_free(result->buffered, stats, error_info);
	}
	/*
	   This might not be the proper place because after this the object cannot be reused only can be dtored.
	   We might need just to free the contents of the warnings, not dtor them
	*/
	if (result->warnings) {
		xmysqlnd_warning_list_free(result->warnings);
		result->warnings = NULL;
	}
	/* Valid for the state too */
	if (result->exec_state) {
		xmysqlnd_stmt_execution_state_free(result->exec_state);
		result->exec_state = NULL;
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
		result->m.free_contents(result, stats, error_info);

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
xmysqlnd_node_stmt_result_init(enum xmysqlnd_result_type type, const size_t prefetch_rows, XMYSQLND_NODE_STMT * stmt, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_result_init");
	result = factory->get_node_stmt_result(factory, type, prefetch_rows, stmt, persistent, stats, error_info);
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
		result->m.dtor(result, stats, error_info);
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
