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

#ifndef XMYSQLND_WIREPROTOCOL_H
#define XMYSQLND_WIREPROTOCOL_H

#include <ext/mysqlnd/mysqlnd_vio.h>
#include "xmysqlnd/xmysqlnd_protocol_frame_codec.h"

#ifdef __cplusplus
#include "proto_gen/mysqlx.pb.h"

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
	COM_EXPECTATIONS_CLOSE = Mysqlx::ClientMessages_Type_EXPECT_CLOSE,
	COM_NONE = 255
};

enum xmysqlnd_server_message_type
{
	XMSG_OK						= Mysqlx::ServerMessages_Type_OK,
	XMSG_ERROR					= Mysqlx::ServerMessages_Type_ERROR,
	XMSG_CAPABILITIES			= Mysqlx::ServerMessages_Type_CONN_CAPABILITIES,
	XMSG_AUTH_CONTINUE			= Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_CONTINUE,
	XMSG_AUTH_OK				= Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_OK,
	XMSG_NOTICE					= Mysqlx::ServerMessages_Type_NOTICE,
	XMSG_COLUMN_METADATA		= Mysqlx::ServerMessages_Type_RESULTSET_COLUMN_META_DATA,
	XMSG_RSET_ROW				= Mysqlx::ServerMessages_Type_RESULTSET_ROW,
	XMSG_RSET_FETCH_DONE		= Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE,
	XMGS_RSET_FETCH_SUSPENDED	= Mysqlx::ServerMessages_Type_RESULTSET_FETCH_SUSPENDED,
	XMSG_RSET_FETCH_DONE_MORE_RSETS = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_RESULTSETS,
	XMSG_STMT_EXECUTE_OK		= Mysqlx::ServerMessages_Type_SQL_STMT_EXECUTE_OK,
	XMSG_RSET_FETCH_DONE_MORE_OUT = Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS,
	XMSG_NONE = 255
};
extern "C"
{

#else
enum xmysqlnd_client_message_type
{
	COM_NONE = 255
};

enum xmysqlnd_server_message_type
{
	XMSG_NONE = 255
};

#endif

#define SEND_READ_CTX_DEF			MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info
#define SEND_READ_CTX_PASSTHRU		vio, pfc, stats, error_info

struct st_xmysqlnd_capabilities_get_message_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * capabilities_zval;
	enum xmysqlnd_server_message_type server_message_type;

	enum_func_status (*send_request)(struct st_xmysqlnd_capabilities_get_message_ctx * msg, SEND_READ_CTX_DEF);
	enum_func_status (*read_response)(struct st_xmysqlnd_capabilities_get_message_ctx * msg, zval * capabilities, SEND_READ_CTX_DEF);
};
struct st_xmysqlnd_capabilities_get_message_ctx xmysqlnd_get_capabilities_get_message(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


struct st_xmysqlnd_capabilities_set_message_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * return_value_zval;
	enum xmysqlnd_server_message_type server_message_type;

	enum_func_status (*send_request)(struct st_xmysqlnd_capabilities_set_message_ctx * msg, const size_t cap_count, zval ** capabilities_names, zval ** capabilities_values, SEND_READ_CTX_DEF);
	enum_func_status (*read_response)(struct st_xmysqlnd_capabilities_set_message_ctx * msg, zval * return_value, SEND_READ_CTX_DEF);
};
struct st_xmysqlnd_capabilities_set_message_ctx xmysqlnd_get_capabilities_set_message(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


struct st_xmysqlnd_auth_start_message_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * auth_start_response_zval;
	enum xmysqlnd_server_message_type server_message_type;
	
	enum_func_status (*send_request)(struct st_xmysqlnd_auth_start_message_ctx * msg, const MYSQLND_CSTRING auth_mech_name, const MYSQLND_CSTRING auth_data, SEND_READ_CTX_DEF);
	enum_func_status (*read_response)(struct st_xmysqlnd_auth_start_message_ctx * msg, zval * auth_start_response, SEND_READ_CTX_DEF);
};
struct st_xmysqlnd_auth_start_message_ctx xmysqlnd_get_auth_start_message(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


struct st_xmysqlnd_auth_continue_message_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * auth_continue_response_zval;
	enum xmysqlnd_server_message_type server_message_type;

	enum_func_status (*send_request)(struct st_xmysqlnd_auth_continue_message_ctx * msg, const MYSQLND_CSTRING schema, const MYSQLND_CSTRING user, const MYSQLND_CSTRING password, const MYSQLND_CSTRING salt, SEND_READ_CTX_DEF);
	enum_func_status (*read_response)(struct st_xmysqlnd_auth_continue_message_ctx * msg, zval * auth_continue_response, SEND_READ_CTX_DEF);
};
struct st_xmysqlnd_auth_continue_message_ctx xmysqlnd_get_auth_continue_message(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


struct st_xmysqlnd_sql_stmt_execute_message_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * response_zval;
	enum xmysqlnd_server_message_type server_message_type;

	enum_func_status (*send_request)(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg, const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt, const zend_bool compact_meta, SEND_READ_CTX_DEF);
	enum_func_status (*read_response)(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg, zval * response, SEND_READ_CTX_DEF);
};
struct st_xmysqlnd_sql_stmt_execute_message_ctx xmysqlnd_get_sql_stmt_execute_message(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* XMYSQLND_WIREPROTOCOL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
