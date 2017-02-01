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
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
}
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

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_rowset::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, init)(XMYSQLND_ROWSET * const result,
									   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
									   const enum xmysqlnd_rowset_type type,
									   const size_t prefetch_rows,
									   XMYSQLND_NODE_STMT * const stmt,
									   MYSQLND_STATS * const stats,
									   MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::init");
	switch (type) {
		case XMYSQLND_TYPE_ROWSET_FWD_ONLY:
			result->fwd = xmysqlnd_rowset_fwd_create(prefetch_rows, stmt, result->persistent, factory, stats, error_info);
			if (result->fwd) {
				ret = PASS;
			}
			break;
		case XMYSQLND_TYPE_ROWSET_BUFFERED:
			result->buffered = xmysqlnd_rowset_buffered_create(stmt, result->persistent, factory, stats, error_info);
			if (result->buffered) {
				ret = PASS;
			}
			break;
		/* no default to get compilation warning if new type is added */
	}
	if (PASS == ret) {
		result->type = type;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::next */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, next)(XMYSQLND_ROWSET * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::next");
	if (result->fwd) {
		ret = result->fwd->m.next(result->fwd, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.next(result->buffered, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::fetch_current */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, fetch_current)(XMYSQLND_ROWSET * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::fetch_current");
	if (result->fwd) {
		ret = result->fwd->m.fetch_current(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_current(result->buffered, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::fetch_one */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, fetch_one)(XMYSQLND_ROWSET * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::fetch_one");
	if (result->fwd) {
		ret = result->fwd->m.fetch_one(result->fwd, row_cursor, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_one(result->buffered, row_cursor, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::fetch_one_c */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, fetch_one_c)(XMYSQLND_ROWSET * const result, const size_t row_cursor, zval ** row, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::fetch_one_c");
	if (result->buffered) {
		ret = result->buffered->m.fetch_one_c(result->buffered, row_cursor, row, duplicate, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::fetch_all */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, fetch_all)(XMYSQLND_ROWSET * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::fetch_all");
	if (result->fwd) {
		ret = result->fwd->m.fetch_all(result->fwd, set, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.fetch_all(result->buffered, set, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::fetch_all_c */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, fetch_all_c)(XMYSQLND_ROWSET * const result, zval ** set, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::fetch_all_c");
	if (result->buffered) {
		ret = result->buffered->m.fetch_all_c(result->buffered, set, duplicate, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::rewind */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, rewind)(XMYSQLND_ROWSET * const result)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::rewind");
	if (result->fwd) {
		ret = result->fwd->m.rewind(result->fwd);
	} else if (result->buffered) {
		ret = result->buffered->m.rewind(result->buffered);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_rowset, eof)(const XMYSQLND_ROWSET * const result)
{
	zend_bool ret;
	DBG_ENTER("xmysqlnd_rowset::eof");
	if (result->fwd) {
		ret = result->fwd->m.eof(result->fwd);
	} else if (result->buffered) {
		ret = result->buffered->m.eof(result->buffered);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::create_row */
static zval *
XMYSQLND_METHOD(xmysqlnd_rowset, create_row)(XMYSQLND_ROWSET * const result,
											 const XMYSQLND_NODE_STMT_RESULT_META * const meta,
											 MYSQLND_STATS * const stats,
											 MYSQLND_ERROR_INFO * const error_info)
{
	zval * ret;
	DBG_ENTER("xmysqlnd_rowset::create_row");
	if (result->fwd) {
		ret = result->fwd->m.create_row(result->fwd, meta, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.create_row(result->buffered, meta, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::destroy_row */
static void
XMYSQLND_METHOD(xmysqlnd_rowset, destroy_row)(XMYSQLND_ROWSET * const result,
											  zval * row,
											  MYSQLND_STATS * const stats,
											  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_rowset::destroy_row");
	if (result->fwd) {
		result->fwd->m.destroy_row(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.destroy_row(result->buffered, row, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset::add_row */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, add_row)(XMYSQLND_ROWSET * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::add_row");
	if (result->fwd) {
		ret = result->fwd->m.add_row(result->fwd, row, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.add_row(result->buffered, row, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::get_row_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_rowset, get_row_count)(const XMYSQLND_ROWSET * const result)
{
	size_t ret;
	DBG_ENTER("xmysqlnd_rowset::get_row_count");
	if (result->fwd) {
		ret = result->fwd->m.get_row_count(result->fwd);
	} else if (result->buffered) {
		ret = result->buffered->m.get_row_count(result->buffered);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::attach_meta */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_rowset, attach_meta)(XMYSQLND_ROWSET * const result, XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_rowset::attach_meta");
	if (result->fwd) {
		ret = result->fwd->m.attach_meta(result->fwd, meta, stats, error_info);
	} else if (result->buffered) {
		ret = result->buffered->m.attach_meta(result->buffered, meta, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_rowset::free_rows_contents */
static void
XMYSQLND_METHOD(xmysqlnd_rowset, free_rows_contents)(XMYSQLND_ROWSET * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset::free_rows_contents");
	if (result->fwd) {
		result->fwd->m.free_rows_contents(result->fwd, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.free_rows_contents(result->buffered, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset::free_rows */
static void
XMYSQLND_METHOD(xmysqlnd_rowset, free_rows)(XMYSQLND_ROWSET * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset::free_rows");
	if (result->fwd) {
		result->fwd->m.free_rows(result->fwd, stats, error_info);
	} else if (result->buffered) {
		result->buffered->m.free_rows(result->buffered, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_rowset, free_contents)(XMYSQLND_ROWSET * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset::free_contents");
	if (result->fwd) {
		xmysqlnd_rowset_fwd_free(result->fwd, stats, error_info);
	} else if (result->buffered) {
		xmysqlnd_rowset_buffered_free(result->buffered, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_rowset::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_rowset, dtor)(XMYSQLND_ROWSET * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset::dtor");
	if (result) {
		result->m.free_contents(result, stats, error_info);

		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_rowset)
	XMYSQLND_METHOD(xmysqlnd_rowset, init),

	XMYSQLND_METHOD(xmysqlnd_rowset, next),
	XMYSQLND_METHOD(xmysqlnd_rowset, fetch_current),
	XMYSQLND_METHOD(xmysqlnd_rowset, fetch_one),
	XMYSQLND_METHOD(xmysqlnd_rowset, fetch_one_c),
	XMYSQLND_METHOD(xmysqlnd_rowset, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_rowset, fetch_all_c),
	XMYSQLND_METHOD(xmysqlnd_rowset, rewind),
	XMYSQLND_METHOD(xmysqlnd_rowset, eof),

	XMYSQLND_METHOD(xmysqlnd_rowset, create_row),
	XMYSQLND_METHOD(xmysqlnd_rowset, destroy_row),
	XMYSQLND_METHOD(xmysqlnd_rowset, add_row),
	XMYSQLND_METHOD(xmysqlnd_rowset, get_row_count),
	XMYSQLND_METHOD(xmysqlnd_rowset, free_rows_contents),
	XMYSQLND_METHOD(xmysqlnd_rowset, free_rows),

	XMYSQLND_METHOD(xmysqlnd_rowset, attach_meta),

	XMYSQLND_METHOD(xmysqlnd_rowset, free_contents),
	XMYSQLND_METHOD(xmysqlnd_rowset, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_rowset);

/* {{{ xmysqlnd_rowset_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_ROWSET *
xmysqlnd_rowset_create(const enum xmysqlnd_rowset_type type,
					   const size_t prefetch_rows,
					   XMYSQLND_NODE_STMT * stmt,
					   const zend_bool persistent,
					   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
					   MYSQLND_STATS * stats,
					   MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_ROWSET * result = NULL;
	DBG_ENTER("xmysqlnd_rowset_create");
	result = object_factory->get_rowset(object_factory, type, prefetch_rows, stmt, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_rowset_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_rowset_free(XMYSQLND_ROWSET * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_rowset_free");
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
