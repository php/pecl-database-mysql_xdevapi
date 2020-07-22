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
#ifndef XMYSQLND_WIREPROTOCOL_H
#define XMYSQLND_WIREPROTOCOL_H

#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd_wireprotocol_types.h"
#include "xmysqlnd/xmysqlnd_protocol_frame_codec.h"

#include "proto_gen/mysqlx_expect.pb.h"

#include "util/strings.h"

#include <optional>

namespace mysqlx {

namespace drv {

namespace compression { class Executor; }

class xmysqlnd_session_data;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_stmt_result_meta;
struct st_xmysqlnd_stmt_execution_state;
class xmysqlnd_warning_list;
struct st_xmysqlnd_level3_io;
struct st_xmysqlnd_pb_message_shell;

struct Message_context
{
	MYSQLND_VIO* vio;
	XMYSQLND_PFC* pfc;
	MYSQLND_STATS* stats;
	MYSQLND_ERROR_INFO* error_info;
	compression::Executor* compression_executor;
};


struct st_xmysqlnd_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, const unsigned int code, const util::string_view& sql_state, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_on_session_var_change_bind
{
	const enum_hnd_func_status (*handler)(void * context, const util::string_view& name, const zval * value);
	void * ctx;
};


struct st_xmysqlnd_msg__capabilities_get
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__capabilities_get* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__capabilities_get* msg,
									  zval * capabilities);

	enum_func_status (*init_read)(st_xmysqlnd_msg__capabilities_get* const msg,
								  const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;
	st_xmysqlnd_on_error_bind on_error;
	zval* capabilities_zval;
};


struct st_xmysqlnd_msg__capabilities_set
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__capabilities_set* msg,
									 const size_t cap_count, zval ** capabilities_names, zval ** capabilities_values);

	enum_func_status (*read_response)(st_xmysqlnd_msg__capabilities_set* msg,
									  zval* return_value);

	enum_func_status (*init_read)(st_xmysqlnd_msg__capabilities_set* const msg,
								  const st_xmysqlnd_on_error_bind on_error);
	Message_context msg_ctx;
	st_xmysqlnd_on_error_bind on_error;
	zval* return_value_zval;
};


struct st_xmysqlnd_on_auth_continue_bind
{
	const enum_hnd_func_status (*handler)(void * context, const util::string_view& input, util::string* output);
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
									 const util::string_view& auth_mech_name,
									 const util::string_view& auth_data);

	enum_func_status (*read_response)(st_xmysqlnd_msg__auth_start* msg,
									  zval * auth_start_response);

	enum_func_status (*init_read)(st_xmysqlnd_msg__auth_start* const msg,
								  const st_xmysqlnd_on_auth_continue_bind on_auth_continue,
								  const st_xmysqlnd_on_warning_bind on_warning,
								  const st_xmysqlnd_on_error_bind on_error,
								  const st_xmysqlnd_on_client_id_bind on_client_id,
								  const st_xmysqlnd_on_session_var_change_bind on_session_var_change);

	Message_context msg_ctx;
	st_xmysqlnd_on_auth_continue_bind on_auth_continue;
	st_xmysqlnd_on_warning_bind on_warning;
	st_xmysqlnd_on_error_bind on_error;
	st_xmysqlnd_on_client_id_bind on_client_id;
	st_xmysqlnd_on_session_var_change_bind on_session_var_change;
	zval* auth_start_response_zval;
};

struct st_xmysqlnd_meta_field_create_bind
{
	st_xmysqlnd_result_field_meta* (*create)(void * context);
	void * ctx;
};

typedef enum_func_status (*func_xmysqlnd_wireprotocol__row_field_decoder)(const util::string_view& buffer, const st_xmysqlnd_result_field_meta* const field_meta, const unsigned int idx, zval * out_zv);

struct st_xmysqlnd_on_row_field_bind
{
	const enum_hnd_func_status (*handler)(void * context, const util::string_view& buffer, const unsigned int idx, const func_xmysqlnd_wireprotocol__row_field_decoder decoder);
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
	const enum_hnd_func_status (*handler)(void * context, const util::string& id);
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
	Message_context msg_ctx;

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
											 const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*init_read)(st_xmysqlnd_msg__sql_stmt_execute* const msg,
								  const st_xmysqlnd_meta_field_create_bind create_meta_field,
								  const st_xmysqlnd_on_row_field_bind on_row_field,
								  const st_xmysqlnd_on_meta_field_bind on_meta_field,
								  const st_xmysqlnd_on_warning_bind on_warning,
								  const st_xmysqlnd_on_error_bind on_error,
								  const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids,
								  const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
								  const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
								  const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
								  const st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok,
								  const st_xmysqlnd_on_resultset_end_bind on_resultset_end);


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

	Message_context msg_ctx;

	st_xmysqlnd_on_error_bind on_error;

	std::optional<bool> keep_open;
};

struct st_xmysqlnd_msg__session_close
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__session_close* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__session_close* msg);

	enum_func_status (*init_read)(
		st_xmysqlnd_msg__session_close* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_msg__connection_close
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__connection_close* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__connection_close* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__connection_close* const msg,
								  const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_msg__expectations_open
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__expectations_open* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__expectations_open* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__expectations_open* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	st_xmysqlnd_on_error_bind on_error;

	Mysqlx::Expect::Open_Condition::Key condition_key;
	util::string condition_value;
	Mysqlx::Expect::Open_Condition::ConditionOperation condition_op;

	enum class Result {
		unknown,
		error,
		ok,
	};

	Result result;

};

struct st_xmysqlnd_msg__expectations_close
{
	enum_func_status (*send_request)(st_xmysqlnd_msg__expectations_close* msg);

	enum_func_status (*read_response)(st_xmysqlnd_msg__expectations_close* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__expectations_close* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_result_ctx
{
	Message_context msg_ctx;

	st_xmysqlnd_on_warning_bind on_warning;
	st_xmysqlnd_on_error_bind on_error;
	st_xmysqlnd_on_execution_state_change_bind on_execution_state_change;
	st_xmysqlnd_on_session_var_change_bind on_session_var_change;
	st_xmysqlnd_on_trx_state_change_bind on_trx_state_change;

	zval* response_zval;
};

struct st_xmysqlnd_msg__collection_add
{
	enum_func_status(*send_request)(st_xmysqlnd_msg__collection_add* msg,
						const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__collection_add* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__collection_add* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	struct st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_msg__table_insert
{
	enum_func_status(*send_insert_request)(st_xmysqlnd_msg__table_insert* msg,
											const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__table_insert* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__table_insert* const msg,
		const st_xmysqlnd_on_warning_bind on_warning,
		const st_xmysqlnd_on_error_bind on_error,
		const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
		const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
		const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change);

	struct st_xmysqlnd_result_ctx result_ctx;
};

struct st_xmysqlnd_msg__prepare_prepare
{
	enum_func_status(*send_prepare_request)(st_xmysqlnd_msg__prepare_prepare* msg,
											const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__prepare_prepare* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__prepare_prepare* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	struct st_xmysqlnd_on_error_bind on_error;
};

struct st_xmysqlnd_msg__prepare_execute
{
	enum_func_status(*send_execute_request)(st_xmysqlnd_msg__prepare_execute* msg,
											const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status(*read_response)(st_xmysqlnd_msg__prepare_execute* msg);

	enum_func_status(*init_read)(st_xmysqlnd_msg__prepare_execute* const msg,
		const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	struct st_xmysqlnd_on_error_bind on_error;
};

/* user for Remove, Update, Delete */
struct st_xmysqlnd_msg__collection_ud
{
	enum_func_status (*send_update_request)(st_xmysqlnd_msg__collection_ud* msg,
											const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*send_delete_request)(st_xmysqlnd_msg__collection_ud* msg,
											const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*read_response)(st_xmysqlnd_msg__collection_ud* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__collection_ud* const msg,
								  const st_xmysqlnd_on_error_bind on_error);

	Message_context msg_ctx;

	struct st_xmysqlnd_on_error_bind on_error;
};


/* user for Remove, Update, Delete */
struct st_xmysqlnd_msg__collection_read
{
	enum_func_status (*send_read_request)(st_xmysqlnd_msg__collection_read* msg,
										  const st_xmysqlnd_pb_message_shell pb_message_shell);

	enum_func_status (*read_response_result)(st_xmysqlnd_msg__collection_read* msg);

	enum_func_status (*init_read)(st_xmysqlnd_msg__collection_read* const msg,
								  const st_xmysqlnd_meta_field_create_bind create_meta_field,
								  const st_xmysqlnd_on_row_field_bind on_row_field,
								  const st_xmysqlnd_on_meta_field_bind on_meta_field,
								  const st_xmysqlnd_on_warning_bind on_warning,
								  const st_xmysqlnd_on_error_bind on_error,
								  const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
								  const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
								  const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
								  const st_xmysqlnd_on_stmt_execute_ok_bind on_stmt_execute_ok,
								  const st_xmysqlnd_on_resultset_end_bind on_resultset_end);

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
	Message_context msg_ctx;
	st_xmysqlnd_msg__capabilities_get	(*get__capabilities_get)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__capabilities_set	(*get__capabilities_set)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__auth_start			(*get__auth_start)(st_xmysqlnd_message_factory* factory);

	st_xmysqlnd_msg__sql_stmt_execute	(*get__sql_stmt_execute)(st_xmysqlnd_message_factory* factory);

	st_xmysqlnd_msg__session_reset (*get__session_reset)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__session_close (*get__session_close)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__connection_close	(*get__connection_close)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__expectations_open (*get__expectations_open)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__expectations_close (*get__expectations_close)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__collection_add	    (*get__collection_add)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__collection_ud		(*get__collection_ud)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__sql_stmt_execute	(*get__collection_read)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__table_insert		(*get__table_insert)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__view_cmd                   (*get__view_create)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__view_cmd                   (*get__view_alter)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__view_cmd                   (*get__view_drop)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__prepare_prepare            (*get__prepare_prepare)(st_xmysqlnd_message_factory* factory);
	st_xmysqlnd_msg__prepare_execute            (*get__prepare_execute)(st_xmysqlnd_message_factory* factory);
};

st_xmysqlnd_message_factory get_message_factory(Message_context msg_ctx);

void xmysqlnd_shutdown_protobuf_library();

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_WIREPROTOCOL_H */
