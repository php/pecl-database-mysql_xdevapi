/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#ifndef XMYSQLND_WIREPROTOCOL_H
#define XMYSQLND_WIREPROTOCOL_H

#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd_protocol_frame_codec.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_notice.pb.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_session_data;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_stmt_result_meta;
struct st_xmysqlnd_stmt_execution_state;
struct st_xmysqlnd_warning_list;
struct st_xmysqlnd_level3_io;
struct st_xmysqlnd_pb_message_shell;

MYSQLND_CSTRING xmysqlnd_field_type_name(const unsigned int type);

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
	COM_CRUD_CREATE_VIEW = Mysqlx::ClientMessages_Type_CRUD_CREATE_VIEW,
	COM_CRUD_MODIFY_VIEW = Mysqlx::ClientMessages_Type_CRUD_MODIFY_VIEW,
	COM_CRUD_DROP_VIEW = Mysqlx::ClientMessages_Type_CRUD_DROP_VIEW,
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



struct st_xmysqlnd_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_on_session_var_change_bind
{
	const enum_hnd_func_status (*handler)(void * context, const MYSQLND_CSTRING name, const zval * value);
	void * ctx;
};




struct st_xmysqlnd_msg__capabilities_get
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__capabilities_get* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__capabilities_get* msg,
									  zval * capabilities);

	enum_func_status (*init_read)(st_xmysqlnd_msg__capabilities_get* const msg,
								  const struct st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	st_xmysqlnd_on_error_bind on_error;
	zval* capabilities_zval;
};


struct st_xmysqlnd_msg__capabilities_set
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__capabilities_set* msg,
									 const size_t cap_count, zval ** capabilities_names, zval ** capabilities_values);

	enum_func_status (*read_response)(st_xmysqlnd_msg__capabilities_set* msg,
									  zval * return_value);

	enum_func_status (*init_read)(st_xmysqlnd_msg__capabilities_set* const msg,
								  const struct st_xmysqlnd_on_error_bind on_error);
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	st_xmysqlnd_on_error_bind on_error;
	zval* return_value_zval;
};


struct st_xmysqlnd_on_auth_continue_bind
{
	const enum_hnd_func_status (*handler)(void * context, const MYSQLND_CSTRING input, MYSQLND_STRING * output);
	void * ctx;
};

struct st_xmysqlnd_on_client_id_bind
{
	enum_func_status (*handler)(void * context, const size_t id);
	void * ctx;
};


struct st_xmysqlnd_msg__auth_start
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__auth_start* msg,
									 const MYSQLND_CSTRING auth_mech_name,
									 const MYSQLND_CSTRING auth_data);

	enum_func_status (*read_response)(st_xmysqlnd_msg__auth_start* msg,
									  zval * auth_start_response);

	enum_func_status (*init_read)(st_xmysqlnd_msg__auth_start* const msg,
								  const struct st_xmysqlnd_on_auth_continue_bind on_auth_continue,
								  const struct st_xmysqlnd_on_warning_bind on_warning,
								  const struct st_xmysqlnd_on_error_bind on_error,
								  const struct st_xmysqlnd_on_client_id_bind on_client_id,
								  const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	st_xmysqlnd_on_auth_continue_bind on_auth_continue;
	st_xmysqlnd_on_warning_bind on_warning;
	st_xmysqlnd_on_error_bind on_error;
	st_xmysqlnd_on_client_id_bind on_client_id;
	st_xmysqlnd_on_session_var_change_bind on_session_var_change;
	zval* auth_start_response_zval;
};

#if AUTH_CONTINUE
struct st_xmysqlnd_msg__auth_continue
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__auth_continue* msg,
									 const MYSQLND_CSTRING schema,
									 const MYSQLND_CSTRING user,
									 const MYSQLND_CSTRING password,
									 const MYSQLND_CSTRING salt);

	enum_func_status (*read_response)(st_xmysqlnd_msg__auth_continue* msg,
									  zval * auth_continue_response);

	enum_func_status (*init_read)(st_xmysqlnd_msg__auth_continue* const msg,
								  const struct st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	st_xmysqlnd_on_error_bind on_error;
	zval* auth_continue_response_zval;
};
#endif

struct st_xmysqlnd_meta_field_create_bind
{
	st_xmysqlnd_result_field_meta* (*create)(void * context);
	void * ctx;
};

typedef enum_func_status (*func_xmysqlnd_wireprotocol__row_field_decoder)(const MYSQLND_CSTRING buffer, const st_xmysqlnd_result_field_meta* const field_meta, const unsigned int idx, zval * out_zv);

struct st_xmysqlnd_on_row_field_bind
{
	const enum_hnd_func_status (*handler)(void * context, const MYSQLND_CSTRING buffer, const unsigned int idx, const func_xmysqlnd_wireprotocol__row_field_decoder decoder);
	void * ctx;
};

struct st_xmysqlnd_on_meta_field_bind
{
	const enum_hnd_func_status (*handler)(void * context, st_xmysqlnd_result_field_meta* field);
	void * ctx;
};


struct st_xmysqlnd_on_execution_state_change_bind
{
	const enum_hnd_func_status (*handler)(void * context, const enum xmysqlnd_execution_state_type type, const size_t value);
	void * ctx;
};

struct st_xmysqlnd_on_generated_doc_ids_bind
{
	const enum_hnd_func_status (*handler)(void * context, const MYSQLND_STRING id);
	void * ctx;
};


struct st_xmysqlnd_on_trx_state_change_bind
{
	const enum_hnd_func_status (*handler)(void * context, const enum xmysqlnd_transaction_state_type type);
	void * ctx;
};


struct st_xmysqlnd_on_stmt_execute_ok_bind
{
	const enum_hnd_func_status (*handler)(void * context);
	void * ctx;
};

struct st_xmysqlnd_on_resultset_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, const zend_bool has_more);
	void * ctx;
};

struct st_xmysqlnd_result_set_reader_ctx
{
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;

	st_xmysqlnd_meta_field_create_bind create_meta_field;

	st_xmysqlnd_on_row_field_bind on_row_field;
	st_xmysqlnd_on_meta_field_bind on_meta_field;
	st_xmysqlnd_on_warning_bind on_warning;
	st_xmysqlnd_on_error_bind on_error;
	st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids;
	st_xmysqlnd_on_execution_state_change_bind on_execution_state_change;
	st_xmysqlnd_on_session_var_change_bind on_session_var_change;
	st_xmysqlnd_on_trx_state_change_bind on_trx_state_change;
	st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok;
	st_xmysqlnd_on_resultset_end_bind on_resultset_end;

	unsigned int field_count:16;
	zend_bool has_more_results:1;
	zend_bool has_more_rows_in_set:1;
	zend_bool read_started:1;
	zval* response_zval;
};


struct st_xmysqlnd_msg__sql_stmt_execute
{
	enum_func_status (*send_execute_request)(st_xmysqlnd_msg__sql_stmt_execute* msg,
											 const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*init_read)(st_xmysqlnd_msg__sql_stmt_execute* const msg,
								  const struct st_xmysqlnd_meta_field_create_bind create_meta_field,
								  const struct st_xmysqlnd_on_row_field_bind on_row_field,
								  const struct st_xmysqlnd_on_meta_field_bind on_meta_field,
								  const struct st_xmysqlnd_on_warning_bind on_warning,
								  const struct st_xmysqlnd_on_error_bind on_error,
								  const struct st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids,
								  const struct st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
								  const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change,
								  const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
								  const struct st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok,
								  const struct st_xmysqlnd_on_resultset_end_bind on_resultset_end);


	enum_func_status (*read_response)(st_xmysqlnd_msg__sql_stmt_execute* const msg,
									  zval * const response);

	st_xmysqlnd_result_set_reader_ctx reader_ctx;
};

struct st_xmysqlnd_msg__session_reset
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__session_reset* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__session_reset* msg);

	enum_func_status (*init_read)(
		st_xmysqlnd_msg__session_reset* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO* vio;
	XMYSQLND_PFC* pfc;
	MYSQLND_STATS* stats;
	MYSQLND_ERROR_INFO* error_info;

	st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_msg__connection_close
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__connection_close* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__connection_close* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__connection_close* const msg,
								  const st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;

	st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_result_ctx
{
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;

	st_xmysqlnd_on_warning_bind on_warning;
	st_xmysqlnd_on_error_bind on_error;
	st_xmysqlnd_on_execution_state_change_bind on_execution_state_change;
	st_xmysqlnd_on_session_var_change_bind on_session_var_change;
	st_xmysqlnd_on_trx_state_change_bind on_trx_state_change;

	zval* response_zval;
};

/* User for Create */
struct st_xmysqlnd_msg__collection_add
{
	enum_func_status(*send_request)(st_xmysqlnd_msg__collection_add* msg,
						const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__collection_add* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__collection_add* const msg,
		const struct st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;

	struct st_xmysqlnd_on_error_bind on_error;
};

/* User for Create */
struct st_xmysqlnd_msg__table_insert
{
	enum_func_status(*send_insert_request)(st_xmysqlnd_msg__table_insert* msg,
											const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__table_insert* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__table_insert* const msg,
		const struct st_xmysqlnd_on_warning_bind on_warning,
		const struct st_xmysqlnd_on_error_bind on_error,
		const struct st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
		const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change,
		const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change);

	struct st_xmysqlnd_result_ctx result_ctx;
};

/* user for Remove, Update, Delete */
struct st_xmysqlnd_msg__collection_ud
{
	enum_func_status (*send_update_request)(st_xmysqlnd_msg__collection_ud* msg,
											const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*send_delete_request)(st_xmysqlnd_msg__collection_ud* msg,
											const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*read_response)(st_xmysqlnd_msg__collection_ud* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__collection_ud* const msg,
								  const struct st_xmysqlnd_on_error_bind on_error);

	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;

	struct st_xmysqlnd_on_error_bind on_error;
};


/* user for Remove, Update, Delete */
struct st_xmysqlnd_msg__collection_read
{
	enum_func_status (*send_read_request)(st_xmysqlnd_msg__collection_read* msg,
										  const struct st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*read_response_result)(st_xmysqlnd_msg__collection_read* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__collection_read* const msg,
								  const struct st_xmysqlnd_meta_field_create_bind create_meta_field,
								  const struct st_xmysqlnd_on_row_field_bind on_row_field,
								  const struct st_xmysqlnd_on_meta_field_bind on_meta_field,
								  const struct st_xmysqlnd_on_warning_bind on_warning,
								  const struct st_xmysqlnd_on_error_bind on_error,
								  const struct st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
								  const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change,
								  const struct st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
								  const struct st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok,
								  const struct st_xmysqlnd_on_resultset_end_bind on_resultset_end);

	struct st_xmysqlnd_result_set_reader_ctx reader_ctx;
};

/* used for View create, alter, drop */
struct st_xmysqlnd_msg__view_cmd
{
	enum_func_status(*send_cmd_request)(
		st_xmysqlnd_msg__view_cmd* msg,
		st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__view_cmd* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__view_cmd* const msg,
		const st_xmysqlnd_on_warning_bind on_warning,
		const st_xmysqlnd_on_error_bind on_error,
		const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
		const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
		const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change);

	st_xmysqlnd_result_ctx result_ctx;
};


struct st_xmysqlnd_message_factory
{
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	struct st_xmysqlnd_msg__capabilities_get	(*get__capabilities_get)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__capabilities_set	(*get__capabilities_set)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__auth_start			(*get__auth_start)(const st_xmysqlnd_message_factory* const factory);
#if AUTH_CONTINUE
	struct st_xmysqlnd_msg__auth_continue		(*get__auth_continue)(const st_xmysqlnd_message_factory* const factory);
#endif
	struct st_xmysqlnd_msg__sql_stmt_execute	(*get__sql_stmt_execute)(const st_xmysqlnd_message_factory* const factory);
	st_xmysqlnd_msg__session_reset (*get__session_reset)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__connection_close	(*get__connection_close)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__collection_add	    (*get__collection_add)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__collection_ud		(*get__collection_ud)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__sql_stmt_execute	(*get__collection_read)(const st_xmysqlnd_message_factory* const factory);
	struct st_xmysqlnd_msg__table_insert		(*get__table_insert)(const st_xmysqlnd_message_factory* const factory);
	st_xmysqlnd_msg__view_cmd (*get__view_create)(const st_xmysqlnd_message_factory * const factory);
	st_xmysqlnd_msg__view_cmd (*get__view_alter)(const st_xmysqlnd_message_factory * const factory);
	st_xmysqlnd_msg__view_cmd (*get__view_drop)(const st_xmysqlnd_message_factory * const factory);
};

struct st_xmysqlnd_message_factory xmysqlnd_get_message_factory(const st_xmysqlnd_level3_io* const io, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

void xmysqlnd_shutdown_protobuf_library();

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_WIREPROTOCOL_H */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
