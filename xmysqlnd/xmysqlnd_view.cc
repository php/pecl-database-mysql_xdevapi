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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_utils.h"
#include "xmysqlnd_view.h"

namespace mysqlx {

namespace drv {

/* {{{ View::create */
st_xmysqlnd_node_stmt* View::create(
	st_xmysqlnd_node_session* session,
	const st_xmysqlnd_pb_message_shell& pb_msg)
{
	XMYSQLND_NODE_STMT* ret = nullptr;
	DBG_ENTER("View::create");
	const st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
	st_xmysqlnd_msg__view_cmd view_create = msg_factory.get__view_create(&msg_factory);
	if (PASS == view_create.send_cmd_request(&view_create, pb_msg)) {
		st_xmysqlnd_node_stmt* stmt = session->m->create_statement_object(session);
		stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
		ret = stmt;
	}
	DBG_INF(ret != nullptr? "PASS":"FAIL");

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ View::alter */
st_xmysqlnd_node_stmt* View::alter(
	st_xmysqlnd_node_session* session,
	const st_xmysqlnd_pb_message_shell& pb_msg)
{
	XMYSQLND_NODE_STMT* ret = nullptr;
	DBG_ENTER("View::alter");
	const st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
	st_xmysqlnd_msg__view_cmd view_alter = msg_factory.get__view_alter(&msg_factory);
	if (PASS == view_alter.send_cmd_request(&view_alter, pb_msg)) {
		st_xmysqlnd_node_stmt* stmt = session->m->create_statement_object(session);
		stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
		ret = stmt;
	}
	DBG_INF(ret != nullptr? "PASS":"FAIL");

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ View::drop */
st_xmysqlnd_node_stmt* View::drop(
	st_xmysqlnd_node_session* session,
	const st_xmysqlnd_pb_message_shell& pb_msg)
{
	XMYSQLND_NODE_STMT* ret = nullptr;
	DBG_ENTER("View::create");
	const st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
	st_xmysqlnd_msg__view_cmd view_drop = msg_factory.get__view_drop(&msg_factory);
	if (PASS == view_drop.send_cmd_request(&view_drop, pb_msg)) {
		st_xmysqlnd_node_stmt* stmt = session->m->create_statement_object(session);
		stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
		ret = stmt;
	}
	DBG_INF(ret != nullptr? "PASS":"FAIL");

	DBG_RETURN(ret);
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
