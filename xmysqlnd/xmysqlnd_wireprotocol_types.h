/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_WIREPROTOCOL_TYPES_H
#define XMYSQLND_WIREPROTOCOL_TYPES_H

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_expect.pb.h"
#include "proto_gen/mysqlx_notice.pb.h"

#include "util/types.h"

namespace mysqlx {

namespace drv {

enum xmysqlnd_client_message_type
{
	COM_CAPABILITIES_GET	= Mysqlx::ClientMessages_Type_CON_CAPABILITIES_GET,
	COM_CAPABILITIES_SET	= Mysqlx::ClientMessages_Type_CON_CAPABILITIES_SET,
	COM_CONN_CLOSE			= Mysqlx::ClientMessages_Type_CON_CLOSE,
	COM_AUTH_START			= Mysqlx::ClientMessages_Type_SESS_AUTHENTICATE_START,
	COM_AUTH_CONTINUE		= Mysqlx::ClientMessages_Type_SESS_AUTHENTICATE_CONTINUE,
	COM_SESSION_RESET		= Mysqlx::ClientMessages_Type_SESS_RESET,
	COM_SESSION_CLOSE		= Mysqlx::ClientMessages_Type_SESS_CLOSE,
	COM_SQL_STMT_EXECUTE	= Mysqlx::ClientMessages_Type_SQL_STMT_EXECUTE,
	COM_CRUD_FIND			= Mysqlx::ClientMessages_Type_CRUD_FIND,
	COM_CRUD_INSERT			= Mysqlx::ClientMessages_Type_CRUD_INSERT,
	COM_CRUD_UPDATE			= Mysqlx::ClientMessages_Type_CRUD_UPDATE,
	COM_CRUD_DELETE			= Mysqlx::ClientMessages_Type_CRUD_DELETE,
	COM_EXPECTATIONS_OPEN	= Mysqlx::ClientMessages_Type_EXPECT_OPEN,
	COM_EXPECTATIONS_CLOSE	= Mysqlx::ClientMessages_Type_EXPECT_CLOSE,
	COM_CRUD_CREATE_VIEW    = Mysqlx::ClientMessages_Type_CRUD_CREATE_VIEW,
	COM_CRUD_MODIFY_VIEW    = Mysqlx::ClientMessages_Type_CRUD_MODIFY_VIEW,
	COM_CRUD_DROP_VIEW      = Mysqlx::ClientMessages_Type_CRUD_DROP_VIEW,
	COM_PREPARE_PREPARE     = Mysqlx::ClientMessages_Type_PREPARE_PREPARE,
	COM_PREPARE_EXECUTE     = Mysqlx::ClientMessages_Type_PREPARE_EXECUTE,
	COM_CURSOR_OPEN			= Mysqlx::ClientMessages_Type_CURSOR_OPEN,
	COM_CURSOR_CLOSE		= Mysqlx::ClientMessages_Type_CURSOR_CLOSE,
	COM_CURSOR_FETCH		= Mysqlx::ClientMessages_Type_CURSOR_FETCH,
	COM_COMPRESSION			= Mysqlx::ClientMessages_Type_COMPRESSION,
	COM_NONE                = 255
};

enum xmysqlnd_server_message_type
{
	XMSG_OK						    = Mysqlx::ServerMessages_Type_OK,
	XMSG_ERROR					    = Mysqlx::ServerMessages_Type_ERROR,
	XMSG_CAPABILITIES			    = Mysqlx::ServerMessages_Type_CONN_CAPABILITIES,
	XMSG_AUTH_CONTINUE			    = Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_CONTINUE,
	XMSG_AUTH_OK				    = Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_OK,
	XMSG_NOTICE					    = Mysqlx::ServerMessages_Type_NOTICE,
	XMSG_COLUMN_METADATA		    = Mysqlx::ServerMessages_Type_RESULTSET_COLUMN_META_DATA,
	XMSG_RSET_ROW				    = Mysqlx::ServerMessages_Type_RESULTSET_ROW,
	XMSG_RSET_FETCH_DONE		    = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE,
	XMGS_RSET_FETCH_SUSPENDED	    = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_SUSPENDED,
	XMSG_RSET_FETCH_DONE_MORE_RSETS = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_RESULTSETS,
	XMSG_STMT_EXECUTE_OK		    = Mysqlx::ServerMessages_Type_SQL_STMT_EXECUTE_OK,
	XMSG_RSET_FETCH_DONE_MORE_OUT   = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS,
	XMSG_COMPRESSION				= Mysqlx::ServerMessages_Type_COMPRESSION,
	XMSG_NONE = 255
};

enum xmysqlnd_stmt_warning_level
{
	XSTMT_WARN_NOTE		= Mysqlx::Notice::Warning_Level_NOTE,
	XSTMT_WARN_WARNING	= Mysqlx::Notice::Warning_Level_WARNING,
	XSTMT_WARN_ERROR	= Mysqlx::Notice::Warning_Level_ERROR,
	XSTMT_WARN_NONE = 255,
};

enum xmysqlnd_execution_state_type
{
	EXEC_STATE_NONE = 0,
	EXEC_STATE_GENERATED_INSERT_ID = 1,
	EXEC_STATE_ROWS_AFFECTED,
	EXEC_STATE_ROWS_FOUND,
	EXEC_STATE_ROWS_MATCHED,
};

enum xmysqlnd_transaction_state_type
{
	TRX_STATE_COMMITTED = 1,
	TRX_STATE_ROLLEDBACK,
};

enum xmysqlnd_changed_state_type
{
	CHG_STATE_CURRENT_SCHEMA = 1,
	CHG_STATE_ACCOUNT_EXPIRED,
	CHG_STATE_PRODUCED_MESSAGE,
	CHG_STATE_CLIENT_ID_ASSIGNED,
};

enum xmysqlnd_data_model
{
	XMYSQLND_MODEL_TABLE,
	XMYSQLND_MODEL_COLLECTION
};

struct Message_data : public util::custom_allocable
{
	Message_data(
		xmysqlnd_server_message_type packet_type,
		util::bytes payload);
	xmysqlnd_server_message_type packet_type;
	util::bytes payload;
};

using Messages = util::vector<Message_data>;

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_WIREPROTOCOL_TYPES_H */
