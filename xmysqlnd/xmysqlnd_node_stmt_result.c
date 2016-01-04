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


/* {{{ xmysqlnd_node_stmt_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, free_contents)(XMYSQLND_NODE_STMT_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::free_contents");
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result, dtor)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result::dtor");
	if (result) {
		result->data->m.free_contents(result);
		result->data->stmt->data->m.free_reference(result->data->stmt, stats, error_info);

		mnd_pefree(result->data, result->data->persistent);
		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt_result)
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, init),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, has_data),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, next),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, fetch_all),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result, eof),
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
	DBG_ENTER("xmysqlnd_node_stmt_free");
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
