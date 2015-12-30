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
#include "xmysqlnd_priv.h" // XMYSQLND_INC_SESSION_STATISTIC_W_VALUE3
#include "xmysqlnd_node_query.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"

/* {{{ xmysqlnd_node_query::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query, init)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_node_query::read_metadata */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query, send_query)(XMYSQLND_NODE_QUERY * const result, const MYSQLND_CSTRING query, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	const MYSQLND_CSTRING namespace_par = {"sql", sizeof("sql") - 1};
	MYSQLND_VIO * vio = result->data->session->io.vio;
	XMYSQLND_PFC * pfc = result->data->session->io.pfc;
	const XMYSQLND_L3_IO io = {vio, pfc};
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&io, stats, error_info);
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_query::send_query");

	result->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
	ret = result->data->msg_stmt_exec.send_request(&result->data->msg_stmt_exec, namespace_par, query, FALSE);
	DBG_INF_FMT("%s", ret == PASS? "PASS":"FAIL");

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_query::read_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query, read_result)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_query::read_result");

	ret = result->data->msg_stmt_exec.read_response(&result->data->msg_stmt_exec, result->data->session, NULL);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_query::skip_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query, skip_result)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_query::skip_result");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_query::eof */
static zend_bool
XMYSQLND_METHOD(xmysqlnd_node_query, eof)(const XMYSQLND_NODE_QUERY * const result)
{
	DBG_ENTER("xmysqlnd_node_query::eof");
	DBG_RETURN(result->data->state == XNODE_QR_EOF);
}
/* }}} */


/* {{{ xmysqlnd_node_query::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_query, free_contents)(XMYSQLND_NODE_QUERY * const result)
{
	DBG_ENTER("xmysqlnd_node_query::free_contents");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_query::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_query, dtor)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_query::dtor");
	if (result) {
		result->data->m.free_contents(result);
		result->data->session->m->free_reference(result->data->session);

		mnd_pefree(result->data, result->data->persistent);
		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_query)
	XMYSQLND_METHOD(xmysqlnd_node_query, init),
	XMYSQLND_METHOD(xmysqlnd_node_query, send_query),
	XMYSQLND_METHOD(xmysqlnd_node_query, read_result),
	XMYSQLND_METHOD(xmysqlnd_node_query, skip_result),
	XMYSQLND_METHOD(xmysqlnd_node_query, eof),
	XMYSQLND_METHOD(xmysqlnd_node_query, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_query, dtor),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_query_init */
PHPAPI XMYSQLND_NODE_QUERY *
xmysqlnd_node_query_init(XMYSQLND_NODE_SESSION_DATA * session, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_QUERY * result = NULL;
	DBG_ENTER("xmysqlnd_node_query_init");
	result = factory->get_node_query(session, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_query_free */
PHPAPI void
xmysqlnd_node_query_free(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_query_free");
	if (result) {
		if (!stats) {
			stats = result->data->session->stats;
		}
		if (!error_info) {
			error_info = result->data->session->error_info;
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
