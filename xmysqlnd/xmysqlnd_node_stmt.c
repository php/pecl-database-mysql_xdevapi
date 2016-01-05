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
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"

/* {{{ xmysqlnd_node_stmt::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, init)(XMYSQLND_NODE_STMT * const stmt,
										  XMYSQLND_NODE_SESSION_DATA * const session,
										  const MYSQLND_CSTRING query,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt::init");
	if (!(stmt->data->session = session->m->get_reference(session))) {
		return FAIL;
	}
	stmt->data->m.get_reference(stmt);
	stmt->data->query = mnd_dup_cstring(query, stmt->data->persistent);
	DBG_INF_FMT("query=[%d]%*s", stmt->data->query.l, stmt->data->query.l, stmt->data->query.s);
	DBG_RETURN(PASS);
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

	stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
	ret = stmt->data->msg_stmt_exec.send_request(&stmt->data->msg_stmt_exec, namespace_par, mnd_str2c(stmt->data->query), FALSE);
	DBG_INF_FMT("%s", ret == PASS? "PASS":"FAIL");

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::read_result */
static struct st_xmysqlnd_node_stmt_result *
XMYSQLND_METHOD(xmysqlnd_node_stmt, read_result)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_STMT_RESULT * result = NULL;
	XMYSQLND_NODE_STMT_RESULT_META * meta = NULL;
	DBG_ENTER("xmysqlnd_node_stmt::read_result");

	result = xmysqlnd_node_stmt_result_init(stmt, stmt->persistent, &stmt->data->session->object_factory, stats, error_info);
	meta = xmysqlnd_node_stmt_result_meta_init(stmt->persistent, &stmt->data->session->object_factory, stats, error_info);
	if (!meta || !result) {
		SET_OOM_ERROR(error_info);
		DBG_RETURN(NULL);
	}

	/*
	  Maybe we can inject a callbacks that creates `meta` on demand, but we still DI it.
	  This way we don't pre-create `meta` and in case of UPSERT we don't waste cycles.
	  For now, we just pre-create.
	*/

	if (PASS == stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, stmt->data->session, result, meta, NULL)) {
		if (!meta->m->get_field_count(meta))
		{
			/* Short path, an UPSERT statement */
			xmysqlnd_node_stmt_result_meta_free(meta, stats, error_info);
			meta = NULL;
		} else {
			result->data->m.attach_meta(result, meta, stats, error_info);
		}
	}
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::skip_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_result)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_node_stmt::skip_result");
	ret = stmt->data->msg_stmt_exec.read_response(&stmt->data->msg_stmt_exec, NULL, NULL, NULL, NULL);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::get_reference */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_stmt, get_reference)(XMYSQLND_NODE_STMT * const stmt)
{
	DBG_ENTER("xmysqlnd_node_stmt::get_reference");
	++stmt->data->refcount;
	DBG_INF_FMT("session=%p new_refcount=%u", stmt, stmt->data->refcount);
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
	DBG_ENTER("xmysqlnd_node_stmt::free_contents");
	if (stmt->data->query.s) {
		mnd_pefree(stmt->data->query.s, stmt->data->persistent);
	}
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


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt)
	XMYSQLND_METHOD(xmysqlnd_node_stmt, init),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, send_query),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, read_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, skip_result),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt, dtor),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_stmt_init */
PHPAPI XMYSQLND_NODE_STMT *
xmysqlnd_node_stmt_init(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING query, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_init");
	if (query.s && query.l) {
		stmt = factory->get_node_stmt(factory, session, query, persistent, stats, error_info);	
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
