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
#include "xmysqlnd_node_query_result.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_protocol_dumper.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"

/* {{{ xmysqlnd_node_query_result::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query_result, init)(XMYSQLND_NODE_QUERY_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_node_query_result::read_result */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_query_result, read_result)(XMYSQLND_NODE_QUERY_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_query_result::read_result");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_query_result::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_query_result, free_contents)(XMYSQLND_NODE_QUERY_RESULT * const result)
{
	DBG_ENTER("xmysqlnd_node_query_result::free_contents");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_query_result::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_query_result, dtor)(XMYSQLND_NODE_QUERY_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_query_result::dtor");
	if (result) {
		result->data->m.free_contents(result);
		result->data->session->m->free_reference(result->data->session);

		mnd_pefree(result->data, result->data->persistent);
		mnd_pefree(result, result->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_query_result)
	XMYSQLND_METHOD(xmysqlnd_node_query_result, init),
	XMYSQLND_METHOD(xmysqlnd_node_query_result, read_result),
	XMYSQLND_METHOD(xmysqlnd_node_query_result, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_query_result, dtor),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_query_result_init */
PHPAPI XMYSQLND_NODE_QUERY_RESULT *
xmysqlnd_node_query_result_init(XMYSQLND_NODE_SESSION_DATA * session, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_QUERY_RESULT * result = NULL;
	DBG_ENTER("xmysqlnd_node_query_result_init");
	result = factory->get_node_query_result(session, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_query_result_free */
PHPAPI void
xmysqlnd_node_query_result_free(XMYSQLND_NODE_QUERY_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_query_result_free");
	if (result) {
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
