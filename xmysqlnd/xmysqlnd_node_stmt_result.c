/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#include <php.h>
#undef ERROR
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_rowset_buffered.h"
#include "xmysqlnd_rowset_fwd.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_rowset.h"

/* {{{ xmysqlnd_node_stmt_result::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init)(XMYSQLND_NODE_STMT_RESULT * const result,
												 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
												 MYSQLND_STATS * const stats,
												 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::init");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::next");
	if (result->rowset) {
		ret = result->rowset->m.next(result->rowset, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_current */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_current)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_current");
	if (result->rowset) {
		ret = result->rowset->m.fetch_current(result->rowset, row, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one)(XMYSQLND_NODE_STMT_RESULT * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one");
	if (result->rowset) {
		ret = result->rowset->m.fetch_one(result->rowset, row_cursor, row, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_one_c */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one_c)(XMYSQLND_NODE_STMT_RESULT * const result, const size_t row_cursor, zval ** row, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_one_c");
	if (result->rowset) {
		ret = result->rowset->m.fetch_one_c(result->rowset, row_cursor, row, duplicate, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_all");
	if (result->rowset) {
		ret = result->rowset->m.fetch_all(result->rowset, set, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::fetch_all_c */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all_c)(XMYSQLND_NODE_STMT_RESULT * const result, zval ** set, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::fetch_all_c");
	if (result->rowset) {
		ret = result->rowset->m.fetch_all_c(result->rowset, set, duplicate, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::rewind */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, rewind)(XMYSQLND_NODE_STMT_RESULT * const result)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::rewind");
	if (result->rowset) {
		ret = result->rowset->m.rewind(result->rowset);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	zend_bool ret = TRUE;
	DBG_ENTER("xmysqlnd_node_stmt_result::eof");
	if (result->rowset) {
		ret = result->rowset->m.eof(result->rowset);
	}
	DBG_INF(ret == TRUE? "TRUE":"FALSE");
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
	zval * ret = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_result::create_row");
	if (result->rowset) {
		ret = result->rowset->m.create_row(result->rowset, meta, stats, error_info);
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
	if (result->rowset) {
		result->rowset->m.destroy_row(result->rowset, row, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_stmt_result::add_row");
	if (result->rowset) {
		ret = result->rowset->m.add_row(result->rowset, row, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result)
{
	size_t ret = 0;
	DBG_ENTER("xmysqlnd_node_stmt_result::get_row_count");
	if (result->rowset) {
		ret = result->rowset->m.get_row_count(result->rowset);
	}
	DBG_INF_FMT("rows="MYSQLND_LLU_SPEC, ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::attach_rowset */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_rowset)(XMYSQLND_NODE_STMT_RESULT * const result, XMYSQLND_ROWSET * const rowset, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::attach_rowset");
	DBG_INF_FMT("current_rowset=%p   rowset=%p", result->rowset, rowset);
	if (result->rowset && result->rowset != rowset) {
		xmysqlnd_rowset_free(result->rowset, stats, error_info);
	}
	if (rowset) {
		DBG_INF_FMT("rows=%u", (uint) rowset->m.get_row_count(rowset));
	}
	result->rowset = rowset;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::attach_execution_state */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_execution_state)(XMYSQLND_NODE_STMT_RESULT * const result, XMYSQLND_STMT_EXECUTION_STATE * const exec_state)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_stmt_result::attach_execution_state");
	DBG_INF_FMT("current_exec_state=%p   exec_state=%p", result->exec_state, exec_state);
	if (exec_state) {
		if (result->exec_state && result->exec_state != exec_state) {
			xmysqlnd_stmt_execution_state_free(result->exec_state);
			result->exec_state = NULL;
		}
		result->exec_state = exec_state;
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::attach_warning_list */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_warning_list)(XMYSQLND_NODE_STMT_RESULT * const result, XMYSQLND_WARNING_LIST * const warning_list)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_stmt_result::attach_warning_list");
	DBG_INF_FMT("current_warnings=%p   warnings=%p", result->warnings, warning_list);
	if (warning_list) {
		if (result->warnings && result->warnings != warning_list) {
			xmysqlnd_warning_list_free(result->warnings);
			result->warnings = NULL;
		}
		result->warnings = warning_list;
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_reference */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_reference)(XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::get_reference");
	++result->refcount;
	DBG_INF_FMT("result=%p new_refcount=%u", result, result->refcount);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_reference)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_stmt_result::free_reference");
	DBG_INF_FMT("result=%p old_refcount=%u", result, result->refcount);
	if (!(--result->refcount)) {
		result->m.dtor(result, stats, error_info);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows_contents");
	if (result->rowset) {
		result->rowset->m.free_rows_contents(result->rowset, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_rows */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_rows");
	if (result->rowset) {
		result->rowset->m.free_rows(result->rowset, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_contents");
	if (result->rowset) {
		xmysqlnd_rowset_free(result->rowset, stats, error_info);
		result->rowset = NULL;
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


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt_result)
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_current),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_one_c),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all_c),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, rewind),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, create_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, destroy_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, add_row),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_row_count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_rows),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_rowset),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_execution_state),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, attach_warning_list),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_reference),

	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_stmt_result);

/* {{{ xmysqlnd_node_stmt_result_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_STMT_RESULT *
xmysqlnd_node_stmt_result_create(const zend_bool persistent,
								 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								 MYSQLND_STATS * stats,
								 MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_result_create");
	result = object_factory->get_node_stmt_result(object_factory, persistent, stats, error_info);
	if (result) {
		result = result->m.get_reference(result);
	}
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_stmt_result_free(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_free");
	DBG_INF_FMT("result=%p", result);
	if (result) {
		result->m.free_reference(result, stats, error_info);
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
