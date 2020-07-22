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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_compression.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_protocol_dumper.h"

#include "xmysqlnd_session.h"
#include "xmysqlnd_stmt_result.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_rowset.h"

#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_crud.pb.h"
#include "proto_gen/mysqlx_datatypes.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "proto_gen/mysqlx_resultset.pb.h"
#include "proto_gen/mysqlx_session.pb.h"
#include "proto_gen/mysqlx_sql.pb.h"

#include "xmysqlnd_crud_collection_commands.h"

#include "util/pb_utils.h"
#include "util/string_utils.h"
#include "util/value.h"
#include "protobuf_api.h"

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_inspect_warning_bind
{
	enum_hnd_func_status (*handler)(void * context, const Mysqlx::Notice::Warning & warning);
	void * ctx;
};


struct st_xmysqlnd_inspect_changed_variable_bind
{
	enum_hnd_func_status (*handler)(void * context, const Mysqlx::Notice::SessionVariableChanged & message);
	void * ctx;
};

static int
get_datetime_type(const XMYSQLND_RESULT_FIELD_META* const field_meta)
{
	if (field_meta->content_type_set && (field_meta->content_type == Mysqlx::Resultset::DATE)) {
		return FIELD_TYPE_DATE;
	}
	return FIELD_TYPE_DATETIME;
}

static enum_hnd_func_status
xmysqlnd_inspect_changed_variable(const st_xmysqlnd_on_session_var_change_bind on_session_var_change, const Mysqlx::Notice::SessionVariableChanged & message)
{
	enum_hnd_func_status ret{HND_AGAIN};
	DBG_ENTER("xmysqlnd_inspect_changed_variable");

	const bool has_param = message.has_param();
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? message.param().c_str() : "n/a");

	const bool has_value = message.has_value();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_param && has_value) {
		const util::string_view name = message.param();
		zval zv;
		ZVAL_UNDEF(&zv);
		if (PASS == scalar2zval(message.value(), &zv)) {
			ret = on_session_var_change.handler(on_session_var_change.ctx, name, &zv);
		}
	}

	DBG_RETURN(ret);
}

static enum_hnd_func_status
xmysqlnd_inspect_warning(const st_xmysqlnd_on_warning_bind on_warning, const Mysqlx::Notice::Warning & warning)
{
	enum_hnd_func_status ret{HND_PASS};
	DBG_ENTER("xmysqlnd_inspect_warning");
	DBG_INF_FMT("on_warning=%p", on_warning.handler);
	if (on_warning.handler) {
		const bool has_level = warning.has_level();
		const bool has_code = warning.has_code();
		const bool has_msg = warning.has_msg();
		const unsigned int code = has_code? warning.code() : 1000;
		const xmysqlnd_stmt_warning_level level{
			has_level ? static_cast<xmysqlnd_stmt_warning_level>(warning.level()) : XSTMT_WARN_WARNING};
		constexpr util::string_view Empty_message = "";
		const util::string_view warn_message{ has_msg ? warning.msg() : Empty_message };

		DBG_INF_FMT("level[%s] is %s", has_level? "SET":"NOT SET",
									   has_level? Mysqlx::Notice::Warning::Level_Name(warning.level()).c_str() : "n/a");
		DBG_INF_FMT("code[%s] is %u", has_code? "SET":"NOT SET",
									  has_code? warning.code() : 0);
		DBG_INF_FMT("messsage[%s] is %s", has_msg? "SET":"NOT SET",
										  has_msg? warning.msg().c_str() : "n/a");

		ret = on_warning.handler(on_warning.ctx, level, code, warn_message);
	}
	DBG_RETURN(ret);
}

static enum_hnd_func_status
xmysqlnd_inspect_changed_exec_state(const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change, const Mysqlx::Notice::SessionStateChanged & message)
{
	enum_hnd_func_status ret{HND_AGAIN};
	xmysqlnd_execution_state_type state_type = EXEC_STATE_NONE;
	DBG_ENTER("xmysqlnd_inspect_changed_exec_state");
	DBG_INF_FMT("on_execution_state_handler=%p", on_execution_state_change.handler);
	DBG_INF_FMT("param is %s", Mysqlx::Notice::SessionStateChanged::Parameter_Name(message.param()).c_str());

	switch (message.param()) {
		case Mysqlx::Notice::SessionStateChanged::GENERATED_INSERT_ID:	  state_type = EXEC_STATE_GENERATED_INSERT_ID;	break;
		case Mysqlx::Notice::SessionStateChanged::ROWS_AFFECTED:		  state_type = EXEC_STATE_ROWS_AFFECTED;		break;
		case Mysqlx::Notice::SessionStateChanged::ROWS_FOUND:			  state_type = EXEC_STATE_ROWS_FOUND;			break;
		case Mysqlx::Notice::SessionStateChanged::ROWS_MATCHED:			  state_type = EXEC_STATE_ROWS_MATCHED;			break;
		default:
			DBG_ERR_FMT("Unknown param name %d. Please add it to the switch", message.param());
			php_error_docref(nullptr, E_WARNING, "Unknown param name %d in %s::%d. Please add it to the switch", message.param(), __FILE__, __LINE__);
			break;
	}
	if (state_type != EXEC_STATE_NONE) {
		ret = on_execution_state_change.handler(
			on_execution_state_change.ctx,
			state_type,
			static_cast<size_t>(scalar2uint(message.value(0))));
	}

#ifdef PHP_DEBUG
	repeated2log(message.value());
#endif

	DBG_RETURN(ret);
}

static enum_hnd_func_status
xmysqlnd_inspect_generated_doc_ids(const st_xmysqlnd_on_generated_doc_ids_bind on_execution_state_change,
								const Mysqlx::Notice::SessionStateChanged & message)
{
	DBG_ENTER("xmysqlnd_inspect_generated_doc_ids");
	enum_hnd_func_status ret{HND_AGAIN};
	for( int idx{ 0 } ; idx < message.value_size(); ++idx ) {
		const util::string& value = scalar2string( message.value(idx) );
		ret = on_execution_state_change.handler(
					on_execution_state_change.ctx,
					value );
	}
	DBG_RETURN(ret);
}

static enum_hnd_func_status
xmysqlnd_inspect_changed_state(const Mysqlx::Notice::SessionStateChanged & message,
							   const st_xmysqlnd_on_execution_state_change_bind on_exec_state_change,
							   const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
							   const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids,
							   const st_xmysqlnd_on_client_id_bind on_client_id)
{
	enum_hnd_func_status ret{HND_AGAIN};
	const bool has_param = message.has_param();
	const bool has_value = 0 < message.value_size();
	DBG_ENTER("xmysqlnd_inspect_changed_state");
	DBG_INF_FMT("on_execution_state_handler=%p", on_exec_state_change.handler);
	DBG_INF_FMT("on_trx_state_change=%p", on_trx_state_change.handler);
	DBG_INF_FMT("on_client_id=%p", on_client_id.handler);
	DBG_INF_FMT("param is %s", has_param? "SET":"NOT SET");
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_param && has_value) {
		switch (message.param()) {
			case Mysqlx::Notice::SessionStateChanged::CURRENT_SCHEMA:
			case Mysqlx::Notice::SessionStateChanged::ACCOUNT_EXPIRED:
				break;
			case Mysqlx::Notice::SessionStateChanged::GENERATED_DOCUMENT_IDS:
				if (on_generated_doc_ids.handler){
					ret = xmysqlnd_inspect_generated_doc_ids( on_generated_doc_ids, message);
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::GENERATED_INSERT_ID:
			case Mysqlx::Notice::SessionStateChanged::ROWS_AFFECTED:
			case Mysqlx::Notice::SessionStateChanged::ROWS_FOUND:
			case Mysqlx::Notice::SessionStateChanged::ROWS_MATCHED:
				if (on_exec_state_change.handler) {
					ret = xmysqlnd_inspect_changed_exec_state(on_exec_state_change, message);
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::TRX_COMMITTED:
				if (on_trx_state_change.handler) {
					ret = on_trx_state_change.handler(on_trx_state_change.ctx, TRX_STATE_COMMITTED);
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::TRX_ROLLEDBACK:
				if (on_trx_state_change.handler) {
					ret = on_trx_state_change.handler(on_trx_state_change.ctx, TRX_STATE_ROLLEDBACK);
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::PRODUCED_MESSAGE:
				break;
			case Mysqlx::Notice::SessionStateChanged::CLIENT_ID_ASSIGNED:
				if (on_client_id.handler) {
					const enum_func_status status = on_client_id.handler(
						on_client_id.ctx,
						static_cast<size_t>(scalar2uint(message.value(0))));
					ret = (status == PASS)? HND_AGAIN : HND_FAIL;
				}
				break;
		}
	}
	if (has_value) {
		repeated2log(message.value());
	}

	DBG_RETURN(ret);
}

static enum_hnd_func_status
xmysqlnd_inspect_notice_frame(const Mysqlx::Notice::Frame & frame,
							  const st_xmysqlnd_on_warning_bind on_warning,
							  const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
							  const st_xmysqlnd_on_execution_state_change_bind on_exec_state_change,
							  const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change,
							  const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids,
							  const st_xmysqlnd_on_client_id_bind on_client_id)
{
	enum_hnd_func_status ret{HND_AGAIN};
	DBG_ENTER("xmysqlnd_inspect_notice_frame");

	const bool has_scope = frame.has_scope();
	DBG_INF_FMT("scope[%s] is %s", has_scope? "SET":"NOT SET",
								   has_scope? Mysqlx::Notice::Frame::Scope_Name(frame.scope()).c_str() : "n/a");

	const bool has_payload = frame.has_payload();
	DBG_INF_FMT("payload is %s", has_payload? "SET":"NOT SET");

	const bool has_type = frame.has_type();

	DBG_INF_FMT("type is %s", has_type? "SET":"NOT SET");
	if (has_scope && frame.scope() == Mysqlx::Notice::Frame_Scope_LOCAL && has_type && has_payload) {
		const char* frame_payload_str = frame.payload().c_str();
		const int frame_payload_size = static_cast<int>(frame.payload().size());
		switch (frame.type()) {
			case 1:{ /* Warning */
					Mysqlx::Notice::Warning message;
					DBG_INF("Warning");
					message.ParseFromArray(frame_payload_str, frame_payload_size);
					if (on_warning.handler) {
						ret = xmysqlnd_inspect_warning(on_warning, message);
					}
					break;
				}
			case 2:{ /* SessionVariableChanged */
					Mysqlx::Notice::SessionVariableChanged message;
					DBG_INF("SessionVariableChanged");
					message.ParseFromArray(frame_payload_str, frame_payload_size);
					if (on_session_var_change.handler) {
						ret = xmysqlnd_inspect_changed_variable(on_session_var_change, message);
					}
					break;
				}
			case 3:{ /* SessionStateChanged */
					Mysqlx::Notice::SessionStateChanged message;
					DBG_INF("SessionStateChanged");
					message.ParseFromArray(frame_payload_str, frame_payload_size);
					ret = xmysqlnd_inspect_changed_state(message, on_exec_state_change, on_trx_state_change, on_generated_doc_ids, on_client_id);
				}
				break;
			default:
				DBG_ERR_FMT("Unknown type %d", frame.type());
				break;
		}
	}
	DBG_RETURN(ret);
}

static inline zend_bool
xmysqlnd_client_message_type_is_valid(const xmysqlnd_client_message_type type)
{
	return Mysqlx::ClientMessages::Type_IsValid((Mysqlx::ClientMessages_Type) type);
}

static inline zend_bool
xmysqlnd_server_message_type_is_valid(const xmysqlnd_server_message_type type)
{
	DBG_ENTER("xmysqlnd_server_message_type_is_valid");
	Mysqlx::ServerMessages_Type server_type = static_cast<Mysqlx::ServerMessages_Type>(type);
	zend_bool ret = Mysqlx::ServerMessages::Type_IsValid(server_type);
	if (ret) {
		DBG_INF_FMT("TYPE=%s", Mysqlx::ServerMessages::Type_Name(server_type).c_str());
	}
	DBG_RETURN(ret);
}

std::string prepare_compression_message_payload(
	xmysqlnd_client_message_type packet_type,
	const compression::Compress_result& compress_result,
	Message_context& msg_ctx)
{
	Mysqlx::Connection::Compression compression_msg;

	compression_msg.set_client_messages(
		static_cast<Mysqlx::ClientMessages_Type>(packet_type));
	compression_msg.set_uncompressed_size(compress_result.uncompressed_size);
	compression_msg.set_payload(compress_result.compressed_payload);

	std::string output;
	compression_msg.SerializeToString(&output);
	return output;
}

const std::size_t SIZE_OF_STACK_BUFFER = 1024;

static const enum_func_status
xmysqlnd_send_message(
	xmysqlnd_client_message_type packet_type,
	::google::protobuf::Message& message,
	Message_context& msg_ctx,
	size_t* bytes_sent)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_send_message");
#ifdef PHP_DEBUG
	if (!xmysqlnd_client_message_type_is_valid(packet_type)) {
		SET_CLIENT_ERROR(msg_ctx.error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The client wants to send invalid packet type");
		DBG_ERR_FMT("The client wants to send invalid packet type %d", (int) packet_type);
		DBG_RETURN(FAIL);
	}
#endif
	char stack_buffer[SIZE_OF_STACK_BUFFER];
	void* payload = stack_buffer;

	const size_t payload_size = message.ByteSize();
	if (payload_size > sizeof(stack_buffer)) {
		payload = payload_size? mnd_emalloc(payload_size) : nullptr;
		if (payload_size && !payload) {
			php_error_docref(nullptr, E_WARNING, "Memory allocation problem");
			SET_OOM_ERROR(msg_ctx.error_info);
			DBG_RETURN(FAIL);
		}
	}
	message.SerializeToArray(payload, static_cast<int>(payload_size));
	if ((payload_size < compression::Client_compression_threshold) || !msg_ctx.compression_executor->enabled()) {
		ret = msg_ctx.pfc->data->m.send(
			msg_ctx.pfc,
			msg_ctx.vio,
			static_cast<zend_uchar>(packet_type),
			static_cast<zend_uchar*>(payload),
			payload_size,
			bytes_sent,
			msg_ctx.stats,
			msg_ctx.error_info);
	} else {
		const compression::Compress_result& compress_result = msg_ctx.compression_executor->compress_message(
			packet_type,
			payload_size,
			static_cast<util::byte*>(payload));
		const std::string& msg_payload = prepare_compression_message_payload(
			packet_type,
			compress_result,
			msg_ctx);
		ret = msg_ctx.pfc->data->m.send(
			msg_ctx.pfc,
			msg_ctx.vio,
			COM_COMPRESSION,
			reinterpret_cast<const util::byte*>(msg_payload.data()),
			msg_payload.length(),
			bytes_sent,
			msg_ctx.stats,
			msg_ctx.error_info);
	}
	if (payload != stack_buffer) {
		mnd_efree(payload);
	}
	DBG_RETURN(ret);
}

struct st_xmysqlnd_server_messages_handlers
{
	const enum_hnd_func_status (*on_OK)(const Mysqlx::Ok & message, void * context);
	const enum_hnd_func_status (*on_ERROR)(const Mysqlx::Error & message, void * context);
	const enum_hnd_func_status (*on_CAPABILITIES)(const Mysqlx::Connection::Capabilities & message, void * context);
	const enum_hnd_func_status (*on_AUTHENTICATE_CONTINUE)(const Mysqlx::Session::AuthenticateContinue & message, void * context);
	const enum_hnd_func_status (*on_AUTHENTICATE_OK)(const Mysqlx::Session::AuthenticateOk & message, void * context);
	const enum_hnd_func_status (*on_NOTICE)(const Mysqlx::Notice::Frame & message, void * context);
	const enum_hnd_func_status (*on_COLUMN_META)(const Mysqlx::Resultset::ColumnMetaData & message, void * context);
	const enum_hnd_func_status (*on_RSET_ROW)(const Mysqlx::Resultset::Row & message, void * context);
	const enum_hnd_func_status (*on_RSET_FETCH_DONE)(const Mysqlx::Resultset::FetchDone & message, void * context);
	const enum_hnd_func_status (*on_RSET_FETCH_SUSPENDED)(void * context); /*  there is no Mysqlx::Resultset::FetchSuspended*/
	const enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_RSETS)(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context);
	const enum_hnd_func_status (*on_STMT_EXECUTE_OK)(const Mysqlx::Sql::StmtExecuteOk & message, void * context);
	const enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_OUT_PARAMS)(const Mysqlx::Resultset::FetchDoneMoreOutParams & message, void * context);
	const enum_hnd_func_status (*on_COMPRESSED)(const Mysqlx::Connection::Compression& message, Message_context& msg_ctx, Messages& messages);
	const enum_hnd_func_status (*on_UNEXPECTED)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
	const enum_hnd_func_status (*on_UNKNOWN)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
};

xmysqlnd_handler_func_status process_received_message(
	st_xmysqlnd_server_messages_handlers* handlers,
	void* handler_ctx,
	Message_context& msg_ctx,
	Messages& messages,
	xmysqlnd_server_message_type packet_type,
	int payload_size,
	unsigned char* payload)
{
	DBG_ENTER("process_received_message");

	if (!xmysqlnd_server_message_type_is_valid(packet_type)) {
		SET_CLIENT_ERROR(msg_ctx.error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The server sent invalid packet type");
		DBG_ERR_FMT("Invalid packet type %u from the server", static_cast<unsigned int>(packet_type));
		DBG_RETURN(HND_FAIL);
	}
	xmysqlnd_handler_func_status hnd_ret = HND_PASS;
	bool handled{ false };
	switch (packet_type) {
		case XMSG_OK:
			if (handlers->on_OK) {
				Mysqlx::Ok message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_OK(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_ERROR:
			if (handlers->on_ERROR) {
				Mysqlx::Error message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_ERROR(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_CAPABILITIES:
			if (handlers->on_CAPABILITIES) {
				Mysqlx::Connection::Capabilities message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_CAPABILITIES(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_AUTH_CONTINUE:
			if (handlers->on_AUTHENTICATE_CONTINUE) {
				Mysqlx::Session::AuthenticateContinue message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_AUTHENTICATE_CONTINUE(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_AUTH_OK:
			if (handlers->on_AUTHENTICATE_OK) {
				Mysqlx::Session::AuthenticateOk message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_AUTHENTICATE_OK(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_NOTICE:
			if (handlers->on_NOTICE) {
				Mysqlx::Notice::Frame message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_NOTICE(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_COLUMN_METADATA:
			if (handlers->on_COLUMN_META) {
				Mysqlx::Resultset::ColumnMetaData message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_COLUMN_META(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_RSET_ROW:
			if (handlers->on_RSET_ROW) {
				Mysqlx::Resultset::Row message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_RSET_ROW(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_RSET_FETCH_DONE:
			if (handlers->on_RSET_FETCH_DONE) {
				Mysqlx::Resultset::FetchDone message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_RSET_FETCH_DONE(message, handler_ctx);
				handled = true;
			}
			break;

		case XMGS_RSET_FETCH_SUSPENDED:
			if (handlers->on_RSET_FETCH_SUSPENDED) {
				hnd_ret = handlers->on_RSET_FETCH_SUSPENDED(handler_ctx);
				handled = true;
			}
			break;

		case XMSG_RSET_FETCH_DONE_MORE_RSETS:
			if (handlers->on_RSET_FETCH_DONE_MORE_RSETS) {
				Mysqlx::Resultset::FetchDoneMoreResultsets message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_RSET_FETCH_DONE_MORE_RSETS(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_STMT_EXECUTE_OK:
			if (handlers->on_STMT_EXECUTE_OK) {
				Mysqlx::Sql::StmtExecuteOk message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_STMT_EXECUTE_OK(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_RSET_FETCH_DONE_MORE_OUT:
			if (handlers->on_RSET_FETCH_DONE_MORE_OUT_PARAMS) {
				Mysqlx::Resultset::FetchDoneMoreOutParams message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_RSET_FETCH_DONE_MORE_OUT_PARAMS(message, handler_ctx);
				handled = true;
			}
			break;

		case XMSG_COMPRESSION:
			if (handlers->on_COMPRESSED) {
				Mysqlx::Connection::Compression message;
				message.ParseFromArray(payload, payload_size);
				hnd_ret = handlers->on_COMPRESSED(message, msg_ctx, messages);
				handled = true;
			}
			break;

		default:
			handled = true;
			if (handlers->on_UNKNOWN) {
				hnd_ret = handlers->on_UNKNOWN(packet_type, payload, payload_size, handler_ctx);
			}
			SET_CLIENT_ERROR(msg_ctx.error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Unknown type");
			DBG_ERR_FMT("Unknown type %d", (int) packet_type);
			break;
	}
	if (!handled) {
		DBG_INF_FMT("Unhandled message %d", packet_type);
		if (handlers->on_UNEXPECTED) {
			hnd_ret = handlers->on_UNEXPECTED(packet_type, payload, payload_size, handler_ctx);
		}
	}
	DBG_RETURN(hnd_ret);
}

enum_func_status
xmysqlnd_receive_message(
	st_xmysqlnd_server_messages_handlers* handlers,
	void* handler_ctx,
	Message_context& msg_ctx)
{
	zend_uchar stack_buffer[SIZE_OF_STACK_BUFFER];
	enum_func_status ret{FAIL};
	enum_hnd_func_status hnd_ret;
	size_t rcv_payload_size;
	zend_uchar* payload;
	zend_uchar type;
	Messages decompressed_messages;

	DBG_ENTER("xmysqlnd_receive_message");

	do {
		if (decompressed_messages.empty()) {
			ret = msg_ctx.pfc->data->m.receive(
				msg_ctx.pfc,
				msg_ctx.vio,
				stack_buffer,
				sizeof(stack_buffer),
				&type,
				&payload,
				&rcv_payload_size,
				msg_ctx.stats,
				msg_ctx.error_info);
			if (FAIL == ret) {
				DBG_RETURN(FAIL);
			}
			hnd_ret = process_received_message(
				handlers,
				handler_ctx,
				msg_ctx,
				decompressed_messages,
				static_cast<xmysqlnd_server_message_type>(type),
				static_cast<int>(rcv_payload_size),
				payload);
			if (payload != stack_buffer) {
				mnd_efree(payload);
			}
			if (hnd_ret == HND_AGAIN) {
				DBG_INF("HND_AGAIN. Reading new packet from the network");
			}
		} else {
			Messages empty_decompressed_messages;
			for (auto& message : decompressed_messages) {
				hnd_ret = process_received_message(
					handlers,
					handler_ctx,
					msg_ctx,
					empty_decompressed_messages,
					message.packet_type,
					static_cast<int>(message.payload.size()),
					message.payload.data());
				// we don't expect any compressed message among just decompressed ones :-P
				assert(empty_decompressed_messages.empty());
			}
			decompressed_messages.clear();
		}
	} while (hnd_ret == HND_AGAIN);
	DBG_INF_FMT("hnd_ret=%d", hnd_ret);
	ret = (hnd_ret == HND_PASS || hnd_ret == HND_AGAIN_ASYNC)? PASS:FAIL;
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}

static enum_hnd_func_status
on_ERROR(const Mysqlx::Error & error, const st_xmysqlnd_on_error_bind on_error)
{
	enum_hnd_func_status ret{HND_PASS_RETURN_FAIL};
	DBG_ENTER("on_ERROR");
	DBG_INF_FMT("on_error.handler=%p", on_error.handler);

	if (on_error.handler) {
		const bool has_sql_state = error.has_sql_state();
		constexpr std::string_view Unknown_sqlstate = UNKNOWN_SQLSTATE;
		const util::string_view sql_state{ has_sql_state? error.sql_state() : Unknown_sqlstate };

		const bool has_code = error.has_code();
		const unsigned int code = has_code? error.code() : CR_UNKNOWN_ERROR;

		const bool has_msg = error.has_msg();
		constexpr std::string_view Unknown_server_error = "Unknown server error";
		const util::string_view error_message{ has_msg? error.msg() : Unknown_server_error };

		ret = on_error.handler(on_error.ctx, code, sql_state, error_message);
	}
	DBG_RETURN(ret);
}

/************************************** CAPABILITIES GET **************************************************/
static void
capabilities_to_zval(const Mysqlx::Connection::Capabilities & message, zval* return_value)
{
	DBG_ENTER("capabilities_to_zv");
	util::zvalue capabilities(util::zvalue::create_array(message.capabilities_size()));
	for (int i{0}; i < message.capabilities_size(); ++i) {
		const auto& capability{ message.capabilities(i) };
		capabilities.insert(capability.name(), any2zval(capability.value()));
	}
	capabilities.move_to(return_value);
	DBG_VOID_RETURN;
}

static const enum_hnd_func_status
capabilities_get_on_ERROR(const Mysqlx::Error & error, void * context)
{
	const enum_hnd_func_status ret = HND_PASS_RETURN_FAIL;
	st_xmysqlnd_msg__capabilities_get* const ctx = static_cast<st_xmysqlnd_msg__capabilities_get* >(context);
	DBG_ENTER("capabilities_get_on_ERROR");
	on_ERROR(error, ctx->on_error);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
capabilities_get_on_CAPABILITIES(const Mysqlx::Connection::Capabilities& message, void* context)
{
	st_xmysqlnd_msg__capabilities_get* const ctx = static_cast<st_xmysqlnd_msg__capabilities_get* >(context);
	capabilities_to_zval(message, ctx->capabilities_zval);
	return HND_PASS;
}

static const enum_hnd_func_status
capabilities_get_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers capabilities_get_handlers =
{
	nullptr,							// on_OK
	capabilities_get_on_ERROR,		// on_ERROR
	capabilities_get_on_CAPABILITIES,// on_CAPABILITIES
	nullptr,							// on_AUTHENTICATE_CONTINUE
	nullptr,							// on_AUTHENTICATE_OK
	capabilities_get_on_NOTICE,		// on_NOTICE
	nullptr,							// on_RSET_COLUMN_META
	nullptr,							// on_RSET_ROW
	nullptr,							// on_RSET_FETCH_DONE
	nullptr,							// on_RESULTSET_FETCH_SUSPENDED
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,							// on_SQL_STMT_EXECUTE_OK
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,							// on_UNEXPECTED
	nullptr,							// on_UNKNOWN
};


enum_func_status
xmysqlnd_capabilities_get__read_response(st_xmysqlnd_msg__capabilities_get* msg, zval * capabilities)
{
	DBG_ENTER("xmysqlnd_capabilities_get__read_response");
	msg->capabilities_zval = capabilities;
	const enum_func_status ret = xmysqlnd_receive_message(&capabilities_get_handlers,
										msg,
										msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_capabilities_get__send_request(st_xmysqlnd_msg__capabilities_get* msg)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesGet message;
	return xmysqlnd_send_message(COM_CAPABILITIES_GET,
					message, msg->msg_ctx, &bytes_sent);
}

enum_func_status
xmysqlnd_capabilities_get__init_read(st_xmysqlnd_msg__capabilities_get* const msg,
									 const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_capabilities_get__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}


static st_xmysqlnd_msg__capabilities_get
xmysqlnd_get_capabilities_get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__capabilities_get ctx =
	{
		xmysqlnd_capabilities_get__send_request,
		xmysqlnd_capabilities_get__read_response,
		xmysqlnd_capabilities_get__init_read,
		msg_ctx,
		{ nullptr, nullptr }, /* on_error */
		nullptr, /* zval */
	};
	return ctx;
}

/************************************** CAPABILITIES SET **************************************************/

static const enum_hnd_func_status
capabilities_set_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
capabilities_set_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__capabilities_set* const ctx = static_cast<st_xmysqlnd_msg__capabilities_set* >(context);
	const enum_hnd_func_status ret = HND_PASS_RETURN_FAIL;
	DBG_ENTER("capabilities_set_on_ERROR");
	on_ERROR(error, ctx->on_error);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
capabilities_set_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers capabilities_set_handlers =
{
	capabilities_set_on_OK,			// on_OK
	capabilities_set_on_ERROR,		// on_ERROR
	nullptr,							// on_CAPABILITIES
	nullptr,							// on_AUTHENTICATE_CONTINUE
	nullptr,							// on_AUTHENTICATE_OK
	capabilities_set_on_NOTICE,		// on_NOTICE
	nullptr,							// on_RSET_COLUMN_META
	nullptr,							// on_RSET_ROW
	nullptr,							// on_RSET_FETCH_DONE
	nullptr,							// on_RESULTSET_FETCH_SUSPENDED
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,							// on_SQL_STMT_EXECUTE_OK
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,							// on_UNEXPECTED
	nullptr,							// on_UNKNOWN
};

enum_func_status
xmysqlnd_capabilities_set__read_response(st_xmysqlnd_msg__capabilities_set* msg, zval* return_value)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_read__capabilities_set");
	msg->return_value_zval = return_value;
	ret = xmysqlnd_receive_message(&capabilities_set_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_capabilities_set__send_request(st_xmysqlnd_msg__capabilities_set* msg,
										const size_t cap_count,
										zval ** capabilities_names,
										zval ** capabilities_values)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesSet message;
	for (unsigned i{0}; i < cap_count; ++i) {
		Mysqlx::Connection::Capability * capability = message.mutable_capabilities()->add_capabilities();
		capability->set_name(Z_STRVAL_P(capabilities_names[i]),
							 Z_STRLEN_P(capabilities_names[i]));
		Mysqlx::Datatypes::Any any_entry;
		zval2any(capabilities_values[i], any_entry);
		capability->mutable_value()->CopyFrom(any_entry);
	}
	return xmysqlnd_send_message(COM_CAPABILITIES_SET, message, msg->msg_ctx, &bytes_sent);
}

enum_func_status
xmysqlnd_capabilities_set__init_read(st_xmysqlnd_msg__capabilities_set* const msg,
									 const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_capabilities_set__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}


static st_xmysqlnd_msg__capabilities_set
xmysqlnd_get_capabilities_set_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__capabilities_set ctx =
	{
		xmysqlnd_capabilities_set__send_request,
		xmysqlnd_capabilities_set__read_response,
		xmysqlnd_capabilities_set__init_read,
		msg_ctx,
		{ nullptr, nullptr },	/* on_error */
		nullptr,			/* zval */
	};
	return ctx;
}

/************************************** AUTH_START **************************************************/
static const enum_hnd_func_status
auth_start_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__auth_start* const ctx = static_cast<st_xmysqlnd_msg__auth_start* >(context);
	const enum_hnd_func_status ret = HND_PASS_RETURN_FAIL;
	DBG_ENTER("auth_start_on_ERROR");
	on_ERROR(error, ctx->on_error);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
auth_start_on_NOTICE(const Mysqlx::Notice::Frame& message, void* context)
{
	const st_xmysqlnd_msg__auth_start* const ctx = static_cast<const st_xmysqlnd_msg__auth_start* >(context);
	const st_xmysqlnd_on_warning_bind on_warning = { nullptr, nullptr };
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change = { nullptr, nullptr };
	const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change = { nullptr, nullptr };
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change = { nullptr, nullptr };
	const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids = { nullptr, nullptr };

	DBG_ENTER("auth_start_on_NOTICE");

	const enum_hnd_func_status ret = xmysqlnd_inspect_notice_frame(message,
																   on_warning,
																   on_session_var_change,
																   on_execution_state_change,
																   on_trx_state_change,
																   on_generated_doc_ids,
																   ctx->on_client_id);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
auth_start_on_AUTHENTICATE_CONTINUE(const Mysqlx::Session::AuthenticateContinue& message, void* context)
{
	enum_hnd_func_status ret{HND_PASS};
	st_xmysqlnd_msg__auth_start* const ctx = static_cast<st_xmysqlnd_msg__auth_start* >(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_CONTINUE");
	if (ctx->on_auth_continue.handler) {
		const util::string_view handler_input = message.auth_data();
		util::string handler_output;

		ret = ctx->on_auth_continue.handler(ctx->on_auth_continue.ctx, handler_input, &handler_output);
		DBG_INF_FMT("handler_output[%d]=[%s]", handler_output.length(), handler_output.data());
		if (!handler_output.empty()) {
			size_t bytes_sent;
			Mysqlx::Session::AuthenticateContinue msg;
			msg.set_auth_data(handler_output.data(), handler_output.length());

			if (FAIL == xmysqlnd_send_message(COM_AUTH_CONTINUE, msg, ctx->msg_ctx, &bytes_sent)) {
				ret = HND_FAIL;
			}
		}
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
auth_start_on_AUTHENTICATE_OK(const Mysqlx::Session::AuthenticateOk& /*message*/, void* context)
{
	st_xmysqlnd_msg__auth_start* const ctx = static_cast<st_xmysqlnd_msg__auth_start* >(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_OK");
	DBG_INF_FMT("ctx->auth_start_response_zval=%p", ctx->auth_start_response_zval);
	DBG_RETURN(HND_PASS);
}

static st_xmysqlnd_server_messages_handlers auth_start_handlers =
{
	nullptr,							// on_OK
	auth_start_on_ERROR,			// on_ERROR
	nullptr,							// on_CAPABILITIES
	auth_start_on_AUTHENTICATE_CONTINUE,// on_AUTHENTICATE_CONTINUE
	auth_start_on_AUTHENTICATE_OK,	// on_AUTHENTICATE_OK
	auth_start_on_NOTICE,			// on_NOTICE
	nullptr,							// on_RSET_COLUMN_META
	nullptr,							// on_RSET_ROW
	nullptr,							// on_RSET_FETCH_DONE
	nullptr,							// on_RESULTSET_FETCH_SUSPENDED
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,							// on_SQL_STMT_EXECUTE_OK
	nullptr,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,							// on_UNEXPECTED
	nullptr,							// on_UNKNOWN
};

enum_func_status
xmysqlnd_authentication_start__init_read(st_xmysqlnd_msg__auth_start* const msg,
										 const st_xmysqlnd_on_auth_continue_bind on_auth_continue,
										 const st_xmysqlnd_on_warning_bind on_warning,
										 const st_xmysqlnd_on_error_bind on_error,
										 const st_xmysqlnd_on_client_id_bind on_client_id,
										 const st_xmysqlnd_on_session_var_change_bind on_session_var_change)
{
	DBG_ENTER("xmysqlnd_authentication_start__init_read");
	msg->on_auth_continue = on_auth_continue;
	msg->on_warning = on_warning;
	msg->on_error = on_error;
	msg->on_client_id = on_client_id;
	msg->on_session_var_change = on_session_var_change;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_authentication_start__read_response(st_xmysqlnd_msg__auth_start* msg, zval * auth_start_response)
{
	DBG_ENTER("xmysqlnd_read__authentication_start");
	msg->auth_start_response_zval = auth_start_response;
	const enum_func_status ret = xmysqlnd_receive_message(&auth_start_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_authentication_start__send_request(st_xmysqlnd_msg__auth_start* msg, const util::string_view& auth_mech_name, const util::string_view& auth_data)
{
	size_t bytes_sent;
	Mysqlx::Session::AuthenticateStart message;
	message.set_mech_name(auth_mech_name.data(), auth_mech_name.length());
	message.set_auth_data(auth_data.data(), auth_data.length());
	return xmysqlnd_send_message(COM_AUTH_START, message, msg->msg_ctx, &bytes_sent);
}


static st_xmysqlnd_msg__auth_start
xmysqlnd_get_auth_start_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__auth_start ctx =
	{
		xmysqlnd_authentication_start__send_request,
		xmysqlnd_authentication_start__read_response,
		xmysqlnd_authentication_start__init_read,
		msg_ctx,
		{ nullptr, nullptr }, 	/* on_auth_continue */
		{ nullptr, nullptr }, 	/* on_warning */
		{ nullptr, nullptr },		/* on_error */
		{ nullptr, nullptr }, 	/* on_client_id */
		{ nullptr, nullptr }, 	/* on_session_var_change */
		nullptr,				/* zval */
	};
	return ctx;
}

/**************************************  STMT_EXECUTE **************************************************/

static const enum_hnd_func_status
stmt_execute_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_ERROR");
	enum_hnd_func_status ret = on_ERROR(error, ctx->on_error);
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
stmt_execute_on_NOTICE(const Mysqlx::Notice::Frame& message, void* context)
{
	const st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<const st_xmysqlnd_result_set_reader_ctx* >(context);
	const st_xmysqlnd_on_client_id_bind on_client_id = { nullptr, nullptr };
	DBG_ENTER("stmt_execute_on_NOTICE");

	const enum_hnd_func_status ret = xmysqlnd_inspect_notice_frame(message,
																   ctx->on_warning,
																   ctx->on_session_var_change,
																   ctx->on_execution_state_change,
																   ctx->on_trx_state_change,
																   ctx->on_generated_doc_ids,
																   on_client_id);

	DBG_RETURN(ret);
}

static const enum_hnd_func_status
stmt_execute_on_COLUMN_META(const Mysqlx::Resultset::ColumnMetaData& message, void* context)
{
	enum_hnd_func_status ret{HND_AGAIN};
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);

	DBG_ENTER("stmt_execute_on_COLUMN_META");
	DBG_INF_FMT("on_meta_field=%p", ctx->on_meta_field.handler);

	ctx->has_more_results = TRUE;
	ctx->has_more_rows_in_set = TRUE;

	++ctx->field_count;
	DBG_INF_FMT("field_count=%u", ctx->field_count);

	if (ctx->create_meta_field.create && ctx->on_meta_field.handler) {
		XMYSQLND_RESULT_FIELD_META * field = ctx->create_meta_field.create(ctx->create_meta_field.ctx);
		if (!field) {
			if (ctx->msg_ctx.error_info) {
				SET_OOM_ERROR(ctx->msg_ctx.error_info);
			}
			DBG_INF("HND_FAIL");
			DBG_RETURN(HND_FAIL);
		}
		if (message.has_type()) {
			field->m->set_type(field, static_cast<xmysqlnd_field_type>(message.type()));
		}
		if (message.has_name()) {
			field->m->set_name(field, message.name().c_str(), message.name().size());
		}
		if (message.has_original_name()) {
			field->m->set_original_name(field, message.original_name().c_str(), message.original_name().size());
		}
		if (message.has_table()) {
			field->m->set_table(field, message.table().c_str(), message.table().size());
		}
		if (message.has_original_table()) {
			field->m->set_original_table(field, message.original_table().c_str(), message.original_table().size());
		}
		if (message.has_schema()) {
			field->m->set_schema(field, message.schema().c_str(), message.schema().size());
		}
		if (message.has_catalog()) {
			field->m->set_catalog(field, message.catalog().c_str(), message.catalog().size());
		}
		if (message.has_collation()) {
			field->m->set_collation(field, message.collation());
		}
		if (message.has_fractional_digits()) {
			field->m->set_fractional_digits(field, message.fractional_digits());
		}
		if (message.has_length()) {
			field->m->set_length(field, message.length());
		}
		if (message.has_flags()) {
			field->m->set_flags(field, message.flags());
		}
		if (message.has_content_type()) {
			field->m->set_content_type(field, message.content_type());
		}

		ret = ctx->on_meta_field.handler(ctx->on_meta_field.ctx, field);

		DBG_INF_FMT("ret=%s", ret == HND_AGAIN? "HND_AGAIN":"n/a");
		DBG_RETURN(ret);
	} else {
		/* skipping */
		DBG_INF("HND_AGAIN");
		DBG_RETURN(ret);
	}
}

static const char * zt2str[] =
{
	"UNDEF",
	"NULL",
	"FALSE",
	"TRUE",
	"LONG",
	"DOUBLE",
	"STRING",
	"ARRAY",
	"OBJECT",
	"RESOURCE",
	"REFERENCE",
};

static inline const char *
ztype2str(const zval * const zv)
{
	const unsigned int type = Z_TYPE_P(zv);
	if (type > IS_REFERENCE) return "n/a";
	return zt2str[type];
}

static
enum_func_status xmysqlnd_row_sint_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_sint_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 gval;
	if (util::pb::read_variant_64(input_stream, &gval)) {
		int64_t ival = ::google::protobuf::internal::WireFormatLite::ZigZagDecode64(gval);
#if SIZEOF_ZEND_LONG==4
		if (UNEXPECTED(ival >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(zv, strpprintf(0, "%s", util::to_string(ival).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(zv));
		} else
#endif
		{
			ZVAL_LONG(zv, static_cast<zend_long>(ival));
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(zv));
		}
	} else {
		DBG_ERR("Error decoding SINT");
		php_error_docref(nullptr, E_WARNING, "Error decoding SINT");
		ret = FAIL;
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_uint_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_uint_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 gval;
	if (util::pb::read_variant_64(input_stream, &gval)) {
#if SIZEOF_ZEND_LONG==8
		if (gval > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
		if (gval > L64(2147483647)) {
#endif
			ZVAL_NEW_STR(zv, strpprintf(0, "%s", util::to_string(gval).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(zv));
		} else {
			ZVAL_LONG(zv, static_cast<zend_long>(gval));
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(zv));
		}
	} else {
		DBG_ERR("Error decoding UINT");
		php_error_docref(nullptr, E_WARNING, "Error decoding UINT");
		ret = FAIL;
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_double_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_double_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 gval;
	if (input_stream.ReadLittleEndian64(&gval)) {
		ZVAL_DOUBLE(zv, ::google::protobuf::internal::WireFormatLite::DecodeDouble(gval));
		DBG_INF_FMT("value   =%10.15f", Z_DVAL_P(zv));
	} else {
		DBG_ERR("Error decoding DOUBLE");
		php_error_docref(nullptr, E_WARNING, "Error decoding DOUBLE");
		ZVAL_NULL(zv);
		ret = FAIL;
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_float_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size,
										const XMYSQLND_RESULT_FIELD_META * const field_meta )
{
	DBG_ENTER("xmysqlnd_row_float_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint32 gval;
	if (input_stream.ReadLittleEndian32(&gval)) {
		const float fval = ::google::protobuf::internal::WireFormatLite::DecodeFloat(gval);
		const unsigned int fractional_digits = field_meta->fractional_digits;
#ifndef NOT_FIXED_DEC
# define NOT_FIXED_DEC 31
#endif
		const double dval = mysql_float_to_double(fval, (fractional_digits >= NOT_FIXED_DEC) ? -1 : fractional_digits);

		ZVAL_DOUBLE(zv, dval);
		DBG_INF_FMT("value   =%f", Z_DVAL_P(zv));
	} else {
		DBG_ERR("Error decoding FLOAT");
		php_error_docref(nullptr, E_WARNING, "Error decoding FLOAT");
		ret = FAIL;
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_time_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_time_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 neg = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
	if ( buf_size != 0 )
	{
		if (buf_size == 1) {
			if (!buf[0]) {
#define	TIME_NULL_VALUE "00:00:00.00"
				ZVAL_NEW_STR(zv, zend_string_init(TIME_NULL_VALUE, sizeof(TIME_NULL_VALUE)-1, 0));
#undef TIME_NULL_VALUE
			} else {
				ZVAL_NULL(zv);
				php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of TIME", static_cast<unsigned int>(buf[0]));
				ret = FAIL;
			}
		} else {
			do {
				if (!util::pb::read_variant_64(input_stream, &neg)) break;
				DBG_INF_FMT("neg     =" MYSQLX_LLU_SPEC, neg);
				if (!util::pb::read_variant_64(input_stream, &hours)) break;
				DBG_INF_FMT("hours   =" MYSQLX_LLU_SPEC, hours);
				if (!util::pb::read_variant_64(input_stream, &minutes)) break;
				DBG_INF_FMT("mins    =" MYSQLX_LLU_SPEC, minutes);
				if (!util::pb::read_variant_64(input_stream, &seconds)) break;
				DBG_INF_FMT("secs    =" MYSQLX_LLU_SPEC, seconds);
				if (!util::pb::read_variant_64(input_stream, &useconds)) break;
				DBG_INF_FMT("usecs   =" MYSQLX_LLU_SPEC, useconds);
			} while (0);

			auto str = util::formatter("%s%02u:%02u:%02u.%08u")
				% (neg ? "-" : "")
				% hours
				% minutes
				% seconds
				% useconds;
			ZVAL_NEW_STR(zv, util::to_zend_string(str));
		}
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_datetime_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_datetime_field_to_zval");
	enum_func_status ret{PASS};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 year = 0, month = 0, day = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
	if ( buf_size != 0 ) {
		if (buf_size == 1) {
			if (!buf[0]) {
#define	DATETIME_NULL_VALUE "0000-00-00 00:00:00.00"
				ZVAL_NEW_STR(zv, zend_string_init(DATETIME_NULL_VALUE, sizeof(DATETIME_NULL_VALUE)-1, 0));
#undef DATETIME_NULL_VALUE
			} else {
				php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of DATETIME", static_cast<unsigned int>(buf[0]));
				ret = FAIL;
			}
		} else {
			do {
				if (!util::pb::read_variant_64(input_stream, &year)) break;
				DBG_INF_FMT("year    =" MYSQLX_LLU_SPEC, year);
				if (!util::pb::read_variant_64(input_stream, &month)) break;
				DBG_INF_FMT("month   =" MYSQLX_LLU_SPEC, month);
				if (!util::pb::read_variant_64(input_stream, &day)) break;
				DBG_INF_FMT("day     =" MYSQLX_LLU_SPEC, day);
				if (!util::pb::read_variant_64(input_stream, &hours)) break;
				DBG_INF_FMT("hours   =" MYSQLX_LLU_SPEC, hours);
				if (!util::pb::read_variant_64(input_stream, &minutes)) break;
				DBG_INF_FMT("mins    =" MYSQLX_LLU_SPEC, minutes);
				if (!util::pb::read_variant_64(input_stream, &seconds)) break;
				DBG_INF_FMT("secs    =" MYSQLX_LLU_SPEC, seconds);
				if (!util::pb::read_variant_64(input_stream, &useconds)) break;
				DBG_INF_FMT("usecs   =" MYSQLX_LLU_SPEC, useconds);
			} while (0);

			auto str = util::formatter("%04u-%02u-%02u %02u:%02u:%02u")
				% year
				% month
				% day
				% hours
				% minutes
				% seconds;
			ZVAL_NEW_STR(zv, util::to_zend_string(str));
		}
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_date_field_to_zval(
	zval* zv,
	const uint8_t * buf,
	const size_t buf_size)
{
	DBG_ENTER("xmysqlnd_row_date_field_to_zval");
	enum_func_status ret{FAIL};
	if (buf_size) {
		::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
		::google::protobuf::uint64 year = 0;
		::google::protobuf::uint64 month = 0;
		::google::protobuf::uint64 day = 0;
		if (buf_size == 1) {
			if (!buf[0]) {
				const std::string date_null_value("0000-00-00");
				ZVAL_NEW_STR(zv, zend_string_init(date_null_value.c_str(), date_null_value.length(), 0));
				ret = PASS;
			} else {
				php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of DATE", static_cast<unsigned int>(buf[0]));
			}
		} else {
			do {
				if (!util::pb::read_variant_64(input_stream, &year)) break;
				DBG_INF_FMT("year  =" MYSQLX_LLU_SPEC, year);
				if (!util::pb::read_variant_64(input_stream, &month)) break;
				DBG_INF_FMT("month =" MYSQLX_LLU_SPEC, month);
				if (!util::pb::read_variant_64(input_stream, &day)) break;
				DBG_INF_FMT("day   =" MYSQLX_LLU_SPEC, day);
			} while (0);

			auto str = util::formatter("%04u-%02u-%02u")
				% year
				% month
				% day;
			ZVAL_NEW_STR(zv, util::to_zend_string(str));
			ret = PASS;
		}
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_set_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_set_field_to_zval");
	enum_func_status ret{PASS};
	unsigned int j{0};
	::google::protobuf::io::CodedInputStream input_stream(buf, static_cast<int>(buf_size));
	::google::protobuf::uint64 gval;
	bool length_read_ok{true};
	array_init(zv);
	if (buf_size == 1 && buf[0] == 0x1) { /* Empty set */
		DBG_RETURN( ret );
	}
	while (length_read_ok) {
		if ((length_read_ok = util::pb::read_variant_64(input_stream, &gval)) == true) {
			char* set_value{nullptr};
			int rest_buffer_size{0};
			if (input_stream.GetDirectBufferPointer((const void**) &set_value, &rest_buffer_size)) {
				zval set_entry;
				DBG_INF_FMT("[%u]value length=%3u  rest_buffer_size=%3d", j, static_cast<unsigned int>(gval), rest_buffer_size);
				if ((rest_buffer_size < 0) || (gval > static_cast<decltype(gval)>(rest_buffer_size))) {
					DBG_ERR("Length pointing outside of the buffer");
					php_error_docref(nullptr, E_WARNING, "Length pointing outside of the buffer");
					ret = FAIL;
					break;
				}
				ZVAL_STRINGL(&set_entry, set_value, static_cast<zend_long>(gval));
				DBG_INF_FMT("[%u]subvalue=%s", j, Z_STRVAL(set_entry));
				zend_hash_next_index_insert(Z_ARRVAL_P(zv), &set_entry);
				if (!input_stream.Skip(static_cast<int>(gval))) {
					break;
				}
			}
		}
		j++;
	}
	DBG_INF_FMT("set elements=%u", zend_hash_num_elements(Z_ARRVAL_P(zv)));
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_decimal_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_decimal_field_to_zval");
	enum_func_status ret{PASS};
	if (!buf_size) {
		DBG_RETURN( ret );
	}
	if (buf_size == 1) {
		DBG_ERR_FMT("Unexpected value for first byte of TIME");
		php_error_docref(nullptr, E_WARNING, "Unexpected value for first byte of TIME");
	}
	const uint8_t scale = buf[0];
	const uint8_t last_byte = buf[buf_size - 1]; /* last byte is the sign and the last 4 bits, if any */
	const uint8_t sign = ((last_byte & 0xF)? last_byte  : last_byte >> 4) & 0xF;
	const size_t digits = (buf_size - 2 /* scale & last */) * 2  + ((last_byte & 0xF) > 0x9? 1:0);
	DBG_INF_FMT("scale   =%u", static_cast<unsigned int>(scale));
	DBG_INF_FMT("sign    =%u", static_cast<unsigned int>(sign));
	DBG_INF_FMT("digits  =%u", static_cast<unsigned int>(digits));
	if (!digits) {
		DBG_ERR_FMT("Wrong value for DECIMAL. scale=%u  last_byte=%u", static_cast<unsigned int>(scale), last_byte);
		php_error_docref(nullptr, E_WARNING, "Wrong value for DECIMAL. scale=%u  last_byte=%u", static_cast<unsigned int>(scale), last_byte);
		ret = FAIL;
	} else {
		const size_t d_val_len = digits + (sign == 0xD? 1:0) + (digits > scale? 1:0); /* one for the dot, one for the sign*/
		char * d_val = new char [d_val_len + 1];
		d_val[d_val_len] = '\0';
		char * p = d_val;
		if (sign == 0xD) {
			*(p++) = '-';
		}
		const size_t dot_position = digits - scale - 1;
		for (unsigned int pos{0}; pos < digits; ++pos) {
			const size_t offset = 1 + (pos >> 1);
			/* if uneven (&0x01) then use the second 4-bits, otherwise shift (>>) the first 4 to the right and then use them */
			const uint8_t digit = (pos & 0x01 ? buf[offset] : buf[offset] >> 4) & 0x0F;
			*(p++) = '0' + digit;
			if (pos == dot_position) {
				*(p++) = '.';
			}
		}
		DBG_INF_FMT("value   =%*s", d_val_len, d_val);
		ZVAL_STRINGL(zv, d_val, d_val_len);
		delete [] d_val;
	}
	DBG_RETURN( ret );
}

static
enum_func_status xmysqlnd_row_string_field_to_zval( zval* zv,
									  const uint8_t * buf,
									  const size_t buf_size )
{
	DBG_ENTER("xmysqlnd_row_string_field_to_zval");
	enum_func_status ret{PASS};
	if (buf_size) {
		DBG_INF("type    =STRING");
		ZVAL_STRINGL(zv, reinterpret_cast<const char *>(buf), buf_size - 1); /* skip the ending \0 */
		DBG_INF_FMT("value   =%s", Z_STRVAL_P(zv));
	} else {
		DBG_ERR("Zero length buffer. NOT ALLOWED in the protocol");
		DBG_INF("value   =NULL");
		ZVAL_NULL(zv);
		ret = FAIL;
	}
	DBG_RETURN( ret );
}

static enum_func_status
xmysqlnd_row_field_to_zval(const util::string_view& buffer,
						   const XMYSQLND_RESULT_FIELD_META * const field_meta,
						   const unsigned int /*i*/,
						   zval * zv)
{
	enum_func_status ret{PASS};
	const uint8_t * buf = reinterpret_cast<const uint8_t*>(buffer.data());
	const size_t buf_size = buffer.length();
	DBG_ENTER("xmysqlnd_row_field_to_zval");
	DBG_INF_FMT("buf_size=%u", static_cast<unsigned int>(buf_size));
	DBG_INF_FMT("name    =%s", field_meta->name.c_str());
	/*
		  Precaution, as if something misbehaves and doesn't initialize `zv` then `zv` will be at
		  the same place in the stack and have the previous value. String reuse will lead to
		  double-free and a crash.
	*/
	ZVAL_NULL(zv);
	if ( buf_size != 0 ) {
		switch (field_meta->type) {
		case XMYSQLND_TYPE_SIGNED_INT:
		{
			DBG_INF("type    =SINT");
			ret = xmysqlnd_row_sint_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_BIT:
			DBG_INF("type    =BIT handled as UINT");
		case XMYSQLND_TYPE_UNSIGNED_INT:
		{
			DBG_INF("type    =UINT");
			ret = xmysqlnd_row_uint_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_DOUBLE:
		{
			DBG_INF("type    =DOUBLE");
			ret = xmysqlnd_row_double_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_FLOAT:
		{
			DBG_INF_FMT("type    =FLOAT");
			ret = xmysqlnd_row_float_field_to_zval( zv, buf, buf_size, field_meta );
			break;
		}
		case XMYSQLND_TYPE_ENUM:
			DBG_INF("type    =ENUM handled as STRING");
		case XMYSQLND_TYPE_BYTES:
		{
			ret = xmysqlnd_row_string_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_TIME:
		{
			DBG_INF("[%2u]type    =TIME");
			ret = xmysqlnd_row_time_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_DATETIME:
		{
			DBG_INF("type    =DATETIME");
			const int datetime_type = get_datetime_type(field_meta);
			if (datetime_type == FIELD_TYPE_DATETIME) {
				ret = xmysqlnd_row_datetime_field_to_zval( zv, buf, buf_size );
			} else {
				assert(datetime_type == FIELD_TYPE_DATE);
				ret = xmysqlnd_row_date_field_to_zval( zv, buf, buf_size );
			}
			break;
		}
		case XMYSQLND_TYPE_SET:
		{
			DBG_INF_FMT("type    =SET");
			ret = xmysqlnd_row_set_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_DECIMAL:{
			DBG_INF("type    =DECIMAL");
			ret = xmysqlnd_row_decimal_field_to_zval( zv, buf, buf_size );
			break;
		}
		case XMYSQLND_TYPE_NONE:{
			DBG_INF("type    =NONE");
			break;
		}
		}
		DBG_INF_FMT("TYPE(zv)=%s", ztype2str(zv));
		DBG_INF("");
	}
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
stmt_execute_on_RSET_ROW(const Mysqlx::Resultset::Row& message, void* context)
{
	enum_hnd_func_status ret{HND_AGAIN};
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_RSET_ROW");
	DBG_INF_FMT("on_row_field.handler=%p  field_count=%u", ctx->on_row_field.handler, ctx->field_count);

	ctx->has_more_results = TRUE;
	if (ctx->on_row_field.handler) {
		for (unsigned int i{0}; i < ctx->field_count; ++i) {
			const util::string_view buffer = message.field(i);
			ret = ctx->on_row_field.handler(ctx->on_row_field.ctx, buffer, i, xmysqlnd_row_field_to_zval);

			if (ret != HND_PASS && ret != HND_AGAIN) {
				DBG_ERR("Something was wrong");
				DBG_RETURN(ret);
			}
		}
	}
	DBG_INF(ret == HND_AGAIN? "HND_AGAIN":"HND_PASS");
	DBG_RETURN(ret);
}

static const enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE(const Mysqlx::Resultset::FetchDone& /*message*/, void* context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_RSET_FETCH_DONE");
	DBG_INF_FMT("on_resultset_end.handler=%p", ctx->on_resultset_end.handler);
	ctx->has_more_results = FALSE;
	ctx->has_more_rows_in_set = FALSE;
	if (ctx->on_resultset_end.handler) {
		ctx->on_resultset_end.handler(ctx->on_resultset_end.ctx, FALSE);
	}
	DBG_RETURN(HND_AGAIN); /* After FETCH_DONE a STMT_EXECUTE_OK is expected */
}

static const enum_hnd_func_status
stmt_execute_on_RSET_FETCH_SUSPENDED(void * context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_RSET_FETCH_SUSPENDED");
	ctx->has_more_results = TRUE;
	DBG_RETURN(HND_PASS);
}

static const enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS(const Mysqlx::Resultset::FetchDoneMoreResultsets& /*message*/, void* context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS");
	DBG_INF_FMT("on_resultset_end.handler=%p", ctx->on_resultset_end.handler);
	ctx->has_more_results = TRUE;
	ctx->has_more_rows_in_set = FALSE;
	if (ctx->on_resultset_end.handler) {
		ctx->on_resultset_end.handler(ctx->on_resultset_end.ctx, TRUE);
	}
	DBG_RETURN(HND_PASS);
}

static const enum_hnd_func_status
stmt_execute_on_STMT_EXECUTE_OK(const Mysqlx::Sql::StmtExecuteOk& /*message*/, void* context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_STMT_EXECUTE_OK");
	DBG_INF_FMT("on_stmt_execute_ok.handler=%p", ctx->on_stmt_execute_ok.handler);
	ctx->has_more_results = FALSE;
	if (ctx->on_stmt_execute_ok.handler) {
		ctx->on_stmt_execute_ok.handler(ctx->on_stmt_execute_ok.ctx);
	}
	DBG_RETURN(HND_PASS);
}

static const enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS(const Mysqlx::Resultset::FetchDoneMoreOutParams& /*message*/, void* context)
{
	st_xmysqlnd_result_set_reader_ctx* const ctx = static_cast<st_xmysqlnd_result_set_reader_ctx* >(context);
	DBG_ENTER("stmt_execute_on_STMT_EXECUTE_OK");
	ctx->has_more_results = TRUE;
	DBG_RETURN(HND_PASS);
}

static const enum_hnd_func_status
stmt_execute_on_COMPRESSED(
	const Mysqlx::Connection::Compression& message,
	Message_context& msg_ctx,
	Messages& messages)
{
	DBG_ENTER("stmt_on_COMPRESSED");
	msg_ctx.compression_executor->decompress_messages(message, messages);
	DBG_RETURN(HND_AGAIN);
}

static st_xmysqlnd_server_messages_handlers stmt_execute_handlers =
{
	nullptr,								// on_OK
	stmt_execute_on_ERROR,				// on_ERROR
	nullptr,								// on_CAPABILITIES
	nullptr,								// on_AUTHENTICATE_CONTINUE
	nullptr,								// on_AUTHENTICATE_OK
	stmt_execute_on_NOTICE,				// on_NOTICE
	stmt_execute_on_COLUMN_META,		// on_RSET_COLUMN_META
	stmt_execute_on_RSET_ROW,			// on_RSET_ROW
	stmt_execute_on_RSET_FETCH_DONE,	// on_RSET_FETCH_DONE
	stmt_execute_on_RSET_FETCH_SUSPENDED,			// on_RESULTSET_FETCH_SUSPENDED
	stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS,		// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	stmt_execute_on_STMT_EXECUTE_OK,				// on_SQL_STMT_EXECUTE_OK
	stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS,// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS
	stmt_execute_on_COMPRESSED,	// on_COMPRESSED
	nullptr,								// on_UNEXPECTED
	nullptr,								// on_UNKNOWN
};


enum_func_status
xmysqlnd_sql_stmt_execute__init_read(st_xmysqlnd_msg__sql_stmt_execute* const msg,
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
									 const st_xmysqlnd_on_resultset_end_bind on_resultset_end)
{
	DBG_ENTER("xmysqlnd_sql_stmt_execute__init_read");
	DBG_INF_FMT("on_row_field.handler  =%p", on_row_field.handler);
	DBG_INF_FMT("on_meta_field.handler =%p", on_meta_field.handler);
	DBG_INF_FMT("on_warning.handler    =%p", on_warning.handler);
	DBG_INF_FMT("on_error.handler      =%p", on_error.handler);
	DBG_INF_FMT("on_error.on_generated_doc_ids    =%p", on_generated_doc_ids.handler);
	DBG_INF_FMT("on_execution_state_change.handler=%p", on_execution_state_change.handler);
	DBG_INF_FMT("on_session_var_change.handler    =%p", on_session_var_change.handler);
	DBG_INF_FMT("on_trx_state_change.handler      =%p", on_trx_state_change.handler);
	DBG_INF_FMT("on_stmt_execute_ok.handler       =%p", on_stmt_execute_ok.handler);
	DBG_INF_FMT("on_resultset_end.handler         =%p", on_resultset_end.handler);

	msg->reader_ctx.create_meta_field = create_meta_field;

	msg->reader_ctx.on_row_field = on_row_field;
	msg->reader_ctx.on_meta_field = on_meta_field;
	msg->reader_ctx.on_warning = on_warning;
	msg->reader_ctx.on_error = on_error;
	msg->reader_ctx.on_generated_doc_ids = on_generated_doc_ids;
	msg->reader_ctx.on_execution_state_change = on_execution_state_change;
	msg->reader_ctx.on_session_var_change = on_session_var_change;
	msg->reader_ctx.on_trx_state_change = on_trx_state_change;
	msg->reader_ctx.on_stmt_execute_ok = on_stmt_execute_ok;
	msg->reader_ctx.on_resultset_end = on_resultset_end;

	msg->reader_ctx.field_count = 0;
	msg->reader_ctx.has_more_results = FALSE;
	msg->reader_ctx.has_more_rows_in_set = FALSE;
	msg->reader_ctx.read_started = FALSE;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_sql_stmt_execute__read_response(st_xmysqlnd_msg__sql_stmt_execute* const msg,
										 zval * const response)
{
	DBG_ENTER("xmysqlnd_sql_stmt_execute__read_response");

	msg->reader_ctx.response_zval = response;
	const enum_func_status ret = xmysqlnd_receive_message(&stmt_execute_handlers,
														  &msg->reader_ctx,
														  msg->reader_ctx.msg_ctx);

	DBG_INF_FMT("xmysqlnd_receive_message returned %s", PASS == ret? "PASS":"FAIL");
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_sql_stmt_execute__send_execute_request(st_xmysqlnd_msg__sql_stmt_execute* msg,
												const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_sql_stmt_execute__send_execute_request");
	size_t bytes_sent;

	const enum_func_status ret = xmysqlnd_send_message(
		static_cast<xmysqlnd_client_message_type>(pb_message_shell.command),
		*(google::protobuf::Message *)(pb_message_shell.message),
		msg->reader_ctx.msg_ctx,
		&bytes_sent);

	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__sql_stmt_execute
xmysqlnd_get_sql_stmt_execute_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__sql_stmt_execute ctx =
	{
		xmysqlnd_sql_stmt_execute__send_execute_request,
		xmysqlnd_sql_stmt_execute__init_read,
		xmysqlnd_sql_stmt_execute__read_response,

		{
			msg_ctx,

			{ nullptr, nullptr}, /* create meta field */

			{ nullptr, nullptr}, /* on_row_field */
			{ nullptr, nullptr}, /* on_meta_field */
			{ nullptr, nullptr}, /* on_warning */
			{ nullptr, nullptr}, /* on_error */
			{ nullptr, nullptr}, /* on_generated_doc_ids */
			{ nullptr, nullptr}, /* on_execution_state_change */
			{ nullptr, nullptr}, /* on_session_var_change */
			{ nullptr, nullptr}, /* on_trx_state_change */
			{ nullptr, nullptr}, /* on_stmt_execute_ok */
			{ nullptr, nullptr}, /* on_resultset_end */

			0,     /* field_count*/
			FALSE, /* has_more_results */
			FALSE, /* has_more_rows_in_set */
			FALSE, /* read_started */
			nullptr,  /* response_zval */
		}
	};
	return ctx;
}

/**************************************  SESS_RESET **************************************************/
static const enum_hnd_func_status
sess_reset_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
sess_reset_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__session_reset* const ctx{ static_cast<st_xmysqlnd_msg__session_reset*>(context) };
	DBG_ENTER("sess_reset_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
sess_reset_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers sess_reset_handlers =
{
	sess_reset_on_OK,		// on_OK
	sess_reset_on_ERROR,	// on_ERROR
	nullptr,				// on_CAPABILITIES
	nullptr,				// on_AUTHENTICATE_CONTINUE
	nullptr,				// on_AUTHENTICATE_OK
	sess_reset_on_NOTICE,	// on_NOTICE
	nullptr,				// on_RSET_COLUMN_META
	nullptr,				// on_RSET_ROW
	nullptr,				// on_RSET_FETCH_DONE
	nullptr,				// on_RESULTSET_FETCH_SUSPENDED
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,				// on_SQL_STMT_EXECUTE_OK
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,				// on_UNEXPECTED
	nullptr,				// on_UNKNOWN
};

enum_func_status
xmysqlnd_sess_reset__init_read(
	st_xmysqlnd_msg__session_reset* const msg,
	const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_sess_reset__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_sess_reset__read_response(st_xmysqlnd_msg__session_reset* msg)
{
	DBG_ENTER("xmysqlnd_sess_reset__read_response");
	const enum_func_status ret{
		xmysqlnd_receive_message(
			&sess_reset_handlers, msg, msg->msg_ctx) };
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_sess_reset__send_request(st_xmysqlnd_msg__session_reset* msg)
{
	size_t bytes_sent;
	Mysqlx::Session::Reset message;
	if (msg->keep_open) {
		message.set_keep_open(*msg->keep_open);
	}
	return xmysqlnd_send_message(
		COM_SESSION_RESET, message, msg->msg_ctx, &bytes_sent);
}

static st_xmysqlnd_msg__session_reset
xmysqlnd_sess_reset__get_message(
	Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__session_reset ctx =
	{
		xmysqlnd_sess_reset__send_request,
		xmysqlnd_sess_reset__read_response,
		xmysqlnd_sess_reset__init_read,
		msg_ctx,
		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  SESS_CLOSE **************************************************/
static const enum_hnd_func_status
sess_close_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
sess_close_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__session_close* const ctx{ static_cast<st_xmysqlnd_msg__session_close*>(context) };
	DBG_ENTER("sess_close_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
sess_close_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers sess_close_handlers =
{
	sess_close_on_OK,		// on_OK
	sess_close_on_ERROR,	// on_ERROR
	nullptr,				// on_CAPABILITIES
	nullptr,				// on_AUTHENTICATE_CONTINUE
	nullptr,				// on_AUTHENTICATE_OK
	sess_close_on_NOTICE,	// on_NOTICE
	nullptr,				// on_RSET_COLUMN_META
	nullptr,				// on_RSET_ROW
	nullptr,				// on_RSET_FETCH_DONE
	nullptr,				// on_RESULTSET_FETCH_SUSPENDED
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,				// on_SQL_STMT_EXECUTE_OK
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,				// on_UNEXPECTED
	nullptr,				// on_UNKNOWN
};

enum_func_status
xmysqlnd_sess_close__init_read(
	st_xmysqlnd_msg__session_close* const msg,
	const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_sess_close__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_sess_close__read_response(st_xmysqlnd_msg__session_close* msg)
{
	DBG_ENTER("xmysqlnd_sess_close__read_response");
	const enum_func_status ret{
		xmysqlnd_receive_message(
			&sess_close_handlers, msg, msg->msg_ctx) };
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_sess_close__send_request(st_xmysqlnd_msg__session_close* msg)
{
	size_t bytes_sent;
	Mysqlx::Session::Close message;
	return xmysqlnd_send_message(
		COM_SESSION_CLOSE, message, msg->msg_ctx, &bytes_sent);
}

static st_xmysqlnd_msg__session_close
xmysqlnd_sess_close__get_message(
	Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__session_close ctx =
	{
		xmysqlnd_sess_close__send_request,
		xmysqlnd_sess_close__read_response,
		xmysqlnd_sess_close__init_read,
		msg_ctx,
		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  CON_CLOSE **************************************************/
static const enum_hnd_func_status
con_close_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
con_close_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__connection_close* const ctx = static_cast<st_xmysqlnd_msg__connection_close* >(context);
	DBG_ENTER("con_close_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
con_close_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers con_close_handlers =
{
	con_close_on_OK,		// on_OK
	con_close_on_ERROR,		// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	con_close_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};

enum_func_status
xmysqlnd_con_close__init_read(st_xmysqlnd_msg__connection_close* const msg,
							  const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_con_close__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_con_close__read_response(st_xmysqlnd_msg__connection_close* msg)
{
	DBG_ENTER("xmysqlnd_con_close__read_response");
	const enum_func_status ret = xmysqlnd_receive_message(&con_close_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_con_close__send_request(st_xmysqlnd_msg__connection_close* msg)
{
	size_t bytes_sent;
	Mysqlx::Connection::Close message;

	return xmysqlnd_send_message(COM_CONN_CLOSE, message, msg->msg_ctx, &bytes_sent);
}

static st_xmysqlnd_msg__connection_close
xmysqlnd_con_close__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__connection_close ctx =
	{
		xmysqlnd_con_close__send_request,
		xmysqlnd_con_close__read_response,
		xmysqlnd_con_close__init_read,
		msg_ctx,

		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  EXPECT_OPEN **************************************************/
static const enum_hnd_func_status
expectations_open_on_OK(const Mysqlx::Ok& /*message*/, void* context)
{
	st_xmysqlnd_msg__expectations_open* const ctx{ static_cast<st_xmysqlnd_msg__expectations_open*>(context) };
	ctx->result = st_xmysqlnd_msg__expectations_open::Result::ok;
	return HND_PASS;
}

static const enum_hnd_func_status
expectations_open_on_ERROR(const Mysqlx::Error& error, void* context)
{
	st_xmysqlnd_msg__expectations_open* const ctx{ static_cast<st_xmysqlnd_msg__expectations_open*>(context) };
	DBG_ENTER("expectations_open_on_ERROR");
	ctx->result = st_xmysqlnd_msg__expectations_open::Result::error;
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
expectations_open_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers expectations_open_handlers
{
	expectations_open_on_OK,		// on_OK
	expectations_open_on_ERROR,	// on_ERROR
	nullptr,				// on_CAPABILITIES
	nullptr,				// on_AUTHENTICATE_CONTINUE
	nullptr,				// on_AUTHENTICATE_OK
	expectations_open_on_NOTICE,	// on_NOTICE
	nullptr,				// on_RSET_COLUMN_META
	nullptr,				// on_RSET_ROW
	nullptr,				// on_RSET_FETCH_DONE
	nullptr,				// on_RESULTSET_FETCH_SUSPENDED
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,				// on_SQL_STMT_EXECUTE_OK
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,				// on_UNEXPECTED
	nullptr,				// on_UNKNOWN
};

enum_func_status
xmysqlnd_expectations_open__init_read(
	st_xmysqlnd_msg__expectations_open* const msg,
	const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_expectations_open__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_expectations_open__read_response(st_xmysqlnd_msg__expectations_open* msg)
{
	DBG_ENTER("xmysqlnd_expectations_open__read_response");
	const enum_func_status ret{
		xmysqlnd_receive_message(
			&expectations_open_handlers, msg, msg->msg_ctx) };
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_expectations_open__send_request(st_xmysqlnd_msg__expectations_open* msg)
{
	size_t bytes_sent;
	Mysqlx::Expect::Open message;
	Mysqlx::Expect::Open_Condition* msg_cond{ message.add_cond() };
	msg_cond->set_condition_key(msg->condition_key);
	msg_cond->set_condition_value(msg->condition_value.c_str());
	msg_cond->set_op(msg->condition_op);
	return xmysqlnd_send_message(
		COM_EXPECTATIONS_OPEN, message, msg->msg_ctx, &bytes_sent);
}

static st_xmysqlnd_msg__expectations_open
xmysqlnd_expectations_open__get_message(
	Message_context& msg_ctx)
{
	st_xmysqlnd_msg__expectations_open ctx
	{
		xmysqlnd_expectations_open__send_request,
		xmysqlnd_expectations_open__read_response,
		xmysqlnd_expectations_open__init_read,
		msg_ctx,
		{ nullptr, nullptr }, /* on_error */
		Mysqlx::Expect::Open_Condition::EXPECT_NO_ERROR,
		{},
		Mysqlx::Expect::Open_Condition::EXPECT_OP_SET,
		st_xmysqlnd_msg__expectations_open::Result::unknown
	};
	return ctx;
}

/**************************************  EXPECT_CLOSE **************************************************/
static const enum_hnd_func_status
expectations_close_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
expectations_close_on_ERROR(const Mysqlx::Error& error, void* context)
{
	st_xmysqlnd_msg__expectations_close* const ctx{ static_cast<st_xmysqlnd_msg__expectations_close*>(context) };
	DBG_ENTER("expectations_close_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
expectations_close_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers expectations_close_handlers
{
	expectations_close_on_OK,		// on_OK
	expectations_close_on_ERROR,	// on_ERROR
	nullptr,				// on_CAPABILITIES
	nullptr,				// on_AUTHENTICATE_CONTINUE
	nullptr,				// on_AUTHENTICATE_OK
	expectations_close_on_NOTICE,	// on_NOTICE
	nullptr,				// on_RSET_COLUMN_META
	nullptr,				// on_RSET_ROW
	nullptr,				// on_RSET_FETCH_DONE
	nullptr,				// on_RESULTSET_FETCH_SUSPENDED
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,				// on_SQL_STMT_EXECUTE_OK
	nullptr,				// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,				// on_UNEXPECTED
	nullptr,				// on_UNKNOWN
};

enum_func_status
xmysqlnd_expectations_close__init_read(
	st_xmysqlnd_msg__expectations_close* const msg,
	const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_expectations_close__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_expectations_close__read_response(st_xmysqlnd_msg__expectations_close* msg)
{
	DBG_ENTER("xmysqlnd_expectations_close__read_response");
	const enum_func_status ret{
		xmysqlnd_receive_message(
			&expectations_close_handlers, msg, msg->msg_ctx) };
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_expectations_close__send_request(st_xmysqlnd_msg__expectations_close* msg)
{
	size_t bytes_sent;
	Mysqlx::Expect::Close message;
	return xmysqlnd_send_message(
		COM_EXPECTATIONS_CLOSE, message, msg->msg_ctx, &bytes_sent);
}

static st_xmysqlnd_msg__expectations_close
xmysqlnd_expectations_close__get_message(
	Message_context& msg_ctx)
{
	st_xmysqlnd_msg__expectations_close ctx
	{
		xmysqlnd_expectations_close__send_request,
		xmysqlnd_expectations_close__read_response,
		xmysqlnd_expectations_close__init_read,
		msg_ctx,
		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  COLLECTION_INSERT **************************************************/
static const enum_hnd_func_status
collection_add_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
collection_add_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__collection_add* const ctx = static_cast<st_xmysqlnd_msg__collection_add* >(context);
	DBG_ENTER("collection_add_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
collection_add_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers collection_add_handlers =
{
	collection_add_on_OK,	// on_OK
	collection_add_on_ERROR,	// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	collection_add_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};

enum_func_status
xmysqlnd_collection_add__init_read(st_xmysqlnd_msg__collection_add* const msg,
									  const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_collection_add__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_collection_add__read_response(st_xmysqlnd_msg__collection_add* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_collection_add__read_response");
	ret = xmysqlnd_receive_message(&collection_add_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_collection_add__send_request(st_xmysqlnd_msg__collection_add* msg,
				const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_collection_add__send_request");
	size_t bytes_sent;
	const enum_func_status ret = xmysqlnd_send_message(COM_CRUD_INSERT,
								 *(google::protobuf::Message *)(pb_message_shell.message),
								 msg->msg_ctx,
								 &bytes_sent);
	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__collection_add
xmysqlnd_collection_add__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__collection_add ctx =
	{
		xmysqlnd_collection_add__send_request,
		xmysqlnd_collection_add__read_response,
		xmysqlnd_collection_add__init_read,
		msg_ctx,

		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  TABLE_INSERT  **************************************************/

static const enum_hnd_func_status
table_insert_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
table_insert_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_result_ctx* const ctx = static_cast<st_xmysqlnd_result_ctx* >(context);
	DBG_ENTER("table_insert_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
table_insert_on_NOTICE(const Mysqlx::Notice::Frame& message, void* context)
{
	DBG_ENTER("table_insert_on_NOTICE");
	st_xmysqlnd_result_ctx* const ctx = static_cast<st_xmysqlnd_result_ctx* >(context);

	const st_xmysqlnd_on_client_id_bind on_client_id = { nullptr, nullptr };
	const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids = { nullptr, nullptr };

	const enum_hnd_func_status ret = xmysqlnd_inspect_notice_frame(message,
																   ctx->on_warning,
																   ctx->on_session_var_change,
																   ctx->on_execution_state_change,
																   ctx->on_trx_state_change,
																   on_generated_doc_ids,
																   on_client_id);

	DBG_RETURN(ret);
}

static st_xmysqlnd_server_messages_handlers table_insert_handlers =
{
	table_insert_on_OK,	// on_OK
	table_insert_on_ERROR,	// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	table_insert_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};

enum_func_status
xmysqlnd_table_insert__send_request(
	st_xmysqlnd_msg__table_insert* msg,
	const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_table_insert__send_request");
	size_t bytes_sent;

	const enum_func_status ret = xmysqlnd_send_message(COM_CRUD_INSERT,
													   *(google::protobuf::Message *)(pb_message_shell.message),
													   msg->result_ctx.msg_ctx,
													   &bytes_sent);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_table_insert__init_read(st_xmysqlnd_msg__table_insert* const msg,
	const st_xmysqlnd_on_warning_bind on_warning,
	const st_xmysqlnd_on_error_bind on_error,
	const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change)
{
	DBG_ENTER("xmysqlnd_table_insert__init_read");
	msg->result_ctx.on_warning = on_warning;
	msg->result_ctx.on_error = on_error;
	msg->result_ctx.on_execution_state_change = on_execution_state_change;
	msg->result_ctx.on_session_var_change = on_session_var_change;
	msg->result_ctx.on_trx_state_change = on_trx_state_change;

	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_table_insert__read_response(st_xmysqlnd_msg__table_insert* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_table_insert__read_response");
	ret = xmysqlnd_receive_message(&table_insert_handlers, &msg->result_ctx, msg->result_ctx.msg_ctx);
	DBG_RETURN(ret);
}


static st_xmysqlnd_msg__table_insert
xmysqlnd_table_insert__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__table_insert ctx =
	{
		xmysqlnd_table_insert__send_request,
		xmysqlnd_table_insert__read_response,
		xmysqlnd_table_insert__init_read,
		{
			msg_ctx,

			{ nullptr, nullptr}, /* on_warning */
			{ nullptr, nullptr}, /* on_error */
			{ nullptr, nullptr}, /* on_execution_state_change */
			{ nullptr, nullptr}, /* on_session_var_change */
			{ nullptr, nullptr}, /* on_trx_state_change */

			nullptr,  /* response_zval */
		}
	};
	return ctx;
}

/**************************************  COLLECTION_MODIFY / COLLECTION_REMOVE  **************************************************/
static const enum_hnd_func_status
collection_ud_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
collection_ud_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__collection_ud* const ctx = static_cast<st_xmysqlnd_msg__collection_ud* >(context);
	DBG_ENTER("collection_ud_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
collection_ud_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers collection_ud_handlers =
{
	collection_ud_on_OK,		// on_OK
	collection_ud_on_ERROR,	// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	collection_ud_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};


enum_func_status
xmysqlnd_collection_ud__init_read(st_xmysqlnd_msg__collection_ud* const msg,
								   const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_collection_ud__init_read");
	DBG_INF_FMT("on_error.handler=%p", on_error.handler);

	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_collection_ud__read_response(st_xmysqlnd_msg__collection_ud* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_collection_ud__read_response");
	ret = xmysqlnd_receive_message(&collection_ud_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

static enum_func_status
xmysqlnd_collection_ud__send_request(st_xmysqlnd_msg__collection_ud* msg,
									 const xmysqlnd_client_message_type pb_message_type,
									 const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_collection_ud__send_request");
	size_t bytes_sent;

	const enum_func_status ret = xmysqlnd_send_message(pb_message_type,
													   *(google::protobuf::Message *)(pb_message_shell.message),
													   msg->msg_ctx,
													   &bytes_sent);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_collection_ud__send_update_request(st_xmysqlnd_msg__collection_ud* msg,
											 const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_collection_ud__send_update_request");
	const enum_func_status ret = xmysqlnd_collection_ud__send_request(msg, COM_CRUD_UPDATE, pb_message_shell);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_collection_ud__send_delete_request(st_xmysqlnd_msg__collection_ud* msg,
											 const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_collection_ud__send_delete_request");
	const enum_func_status ret = xmysqlnd_collection_ud__send_request(msg, COM_CRUD_DELETE, pb_message_shell);
	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__collection_ud
xmysqlnd_collection_ud__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__collection_ud ctx =
	{
		xmysqlnd_collection_ud__send_update_request,
		xmysqlnd_collection_ud__send_delete_request,
		xmysqlnd_collection_ud__read_response,
		xmysqlnd_collection_ud__init_read,

		msg_ctx,

		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  VIEW_CMD  **************************************************/

static const enum_hnd_func_status
view_cmd_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
view_cmd_on_ERROR(const Mysqlx::Error & error, void* context)
{
	st_xmysqlnd_result_ctx* const ctx = static_cast<st_xmysqlnd_result_ctx*>(context);
	DBG_ENTER("view_cmd_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
view_cmd_on_NOTICE(const Mysqlx::Notice::Frame & message, void* context)
{
	DBG_ENTER("view_cmd_on_NOTICE");
	st_xmysqlnd_result_ctx* const ctx = static_cast<st_xmysqlnd_result_ctx*>(context);

	const st_xmysqlnd_on_client_id_bind on_client_id = { nullptr, nullptr };
	const st_xmysqlnd_on_generated_doc_ids_bind on_generated_doc_ids = { nullptr, nullptr };

	const enum_hnd_func_status ret = xmysqlnd_inspect_notice_frame(
		message,
		ctx->on_warning,
		ctx->on_session_var_change,
		ctx->on_execution_state_change,
		ctx->on_trx_state_change,
		on_generated_doc_ids,
		on_client_id);

	DBG_RETURN(ret);
}

static st_xmysqlnd_server_messages_handlers view_cmd_handlers =
{
	view_cmd_on_OK,	// on_OK
	view_cmd_on_ERROR, // on_ERROR
	nullptr, // on_CAPABILITIES
	nullptr, // on_AUTHENTICATE_CONTINUE
	nullptr, // on_AUTHENTICATE_OK
	view_cmd_on_NOTICE,	// on_NOTICE
	nullptr, // on_RSET_COLUMN_META
	nullptr, // on_RSET_ROW
	nullptr, // on_RSET_FETCH_DONE
	nullptr, // on_RESULTSET_FETCH_SUSPENDED
	nullptr, // on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr, // on_SQL_STMT_EXECUTE_OK
	nullptr, // on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr, // on_UNEXPECTED
	nullptr, // on_UNKNOWN
};

template<xmysqlnd_client_message_type View_cmd_id>
enum_func_status
xmysqlnd_view_cmd__send_request(
	st_xmysqlnd_msg__view_cmd* msg,
	const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_view_cmd__send_request");
	size_t bytes_sent;

	const enum_func_status ret = xmysqlnd_send_message(
		View_cmd_id,
		*(google::protobuf::Message *)(pb_message_shell.message),
		msg->result_ctx.msg_ctx,
		&bytes_sent);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_view_cmd__init_read(
	st_xmysqlnd_msg__view_cmd* const msg,
	const st_xmysqlnd_on_warning_bind on_warning,
	const st_xmysqlnd_on_error_bind on_error,
	const st_xmysqlnd_on_execution_state_change_bind on_execution_state_change,
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change,
	const st_xmysqlnd_on_trx_state_change_bind on_trx_state_change)
{
	DBG_ENTER("xmysqlnd_view_cmd__init_read");
	msg->result_ctx.on_warning = on_warning;
	msg->result_ctx.on_error = on_error;
	msg->result_ctx.on_execution_state_change = on_execution_state_change;
	msg->result_ctx.on_session_var_change = on_session_var_change;
	msg->result_ctx.on_trx_state_change = on_trx_state_change;

	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_view_cmd__read_response(st_xmysqlnd_msg__view_cmd* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_view_cmd__read_response");
	ret = xmysqlnd_receive_message(
		&view_cmd_handlers,
		&msg->result_ctx,
		msg->result_ctx.msg_ctx);
	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_view_create__get_message(
	Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__view_cmd ctx =
	{
		xmysqlnd_view_cmd__send_request<COM_CRUD_CREATE_VIEW>,
		xmysqlnd_view_cmd__read_response,
		xmysqlnd_view_cmd__init_read,
		{
			msg_ctx,

			{ nullptr, nullptr}, /* on_warning */
			{ nullptr, nullptr}, /* on_error */
			{ nullptr, nullptr}, /* on_execution_state_change */
			{ nullptr, nullptr}, /* on_session_var_change */
			{ nullptr, nullptr}, /* on_trx_state_change */

			nullptr,  /* response_zval */
		}
	};
	return ctx;
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_view_alter__get_message(
	Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__view_cmd ctx =
	{
		xmysqlnd_view_cmd__send_request<COM_CRUD_MODIFY_VIEW>,
		xmysqlnd_view_cmd__read_response,
		xmysqlnd_view_cmd__init_read,
		{
			msg_ctx,

			{ nullptr, nullptr}, /* on_warning */
			{ nullptr, nullptr}, /* on_error */
			{ nullptr, nullptr}, /* on_execution_state_change */
			{ nullptr, nullptr}, /* on_session_var_change */
			{ nullptr, nullptr}, /* on_trx_state_change */

			nullptr,  /* response_zval */
		}
	};
	return ctx;
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_view_drop__get_message(
	Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__view_cmd ctx =
	{
		xmysqlnd_view_cmd__send_request<COM_CRUD_DROP_VIEW>,
		xmysqlnd_view_cmd__read_response,
		xmysqlnd_view_cmd__init_read,
		{
			msg_ctx,

			{ nullptr, nullptr}, /* on_warning */
			{ nullptr, nullptr}, /* on_error */
			{ nullptr, nullptr}, /* on_execution_state_change */
			{ nullptr, nullptr}, /* on_session_var_change */
			{ nullptr, nullptr}, /* on_trx_state_change */

			nullptr,  /* response_zval */
		}
	};
	return ctx;
}

/**************************************  PREPARE_PREPARE **************************************************/
static const enum_hnd_func_status
prepare_prepare_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
prepare_prepare_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__prepare_prepare* const ctx = static_cast<st_xmysqlnd_msg__prepare_prepare* >(context);
	DBG_ENTER("prepare_prepare_on_ERROR");
    on_ERROR(error, ctx->on_error);
    return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
prepare_prepare_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers prepare_prepare_handlers =
{
	prepare_prepare_on_OK,	// on_OK
	prepare_prepare_on_ERROR,	// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	prepare_prepare_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};

enum_func_status
xmysqlnd_prepare_prepare__init_read(st_xmysqlnd_msg__prepare_prepare* const msg,
									  const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_prepare_prepare__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_prepare_prepare__read_response(st_xmysqlnd_msg__prepare_prepare* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_prepare_prepare__read_response");
	ret = xmysqlnd_receive_message(&prepare_prepare_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_prepare_prepare__send_request(st_xmysqlnd_msg__prepare_prepare* msg,
				const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_prepare_prepare__send_request");
	size_t bytes_sent;
	const enum_func_status ret = xmysqlnd_send_message(COM_PREPARE_PREPARE,
								 *(google::protobuf::Message *)(pb_message_shell.message),
								 msg->msg_ctx,
								 &bytes_sent);
	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__prepare_prepare
xmysqlnd_prepare_prepare__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__prepare_prepare ctx =
	{
		xmysqlnd_prepare_prepare__send_request,
		xmysqlnd_prepare_prepare__read_response,
		xmysqlnd_prepare_prepare__init_read,
		msg_ctx,

		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  PREPARE_EXECUTE **************************************************/
static const enum_hnd_func_status
prepare_execute_on_OK(const Mysqlx::Ok& /*message*/, void* /*context*/)
{
	return HND_PASS;
}

static const enum_hnd_func_status
prepare_execute_on_ERROR(const Mysqlx::Error & error, void * context)
{
	st_xmysqlnd_msg__prepare_execute* const ctx = static_cast<st_xmysqlnd_msg__prepare_execute* >(context);
	DBG_ENTER("prepare_execute_on_ERROR");
	on_ERROR(error, ctx->on_error);
	return HND_PASS_RETURN_FAIL;
}

static const enum_hnd_func_status
prepare_execute_on_NOTICE(const Mysqlx::Notice::Frame& /*message*/, void* /*context*/)
{
	return HND_AGAIN;
}

static st_xmysqlnd_server_messages_handlers prepare_execute_handlers =
{
	prepare_execute_on_OK,	// on_OK
	prepare_execute_on_ERROR,	// on_ERROR
	nullptr,					// on_CAPABILITIES
	nullptr,					// on_AUTHENTICATE_CONTINUE
	nullptr,					// on_AUTHENTICATE_OK
	prepare_execute_on_NOTICE,	// on_NOTICE
	nullptr,					// on_RSET_COLUMN_META
	nullptr,					// on_RSET_ROW
	nullptr,					// on_RSET_FETCH_DONE
	nullptr,					// on_RESULTSET_FETCH_SUSPENDED
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	nullptr,					// on_SQL_STMT_EXECUTE_OK
	nullptr,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	nullptr,					// on_UNEXPECTED
	nullptr,					// on_UNKNOWN
};

enum_func_status
xmysqlnd_prepare_execute__init_read(st_xmysqlnd_msg__prepare_execute* const msg,
									  const st_xmysqlnd_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_prepare_execute__init_read");
	msg->on_error = on_error;
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_prepare_execute__read_response(st_xmysqlnd_msg__prepare_execute* msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_prepare_execute__read_response");
	ret = xmysqlnd_receive_message(&prepare_execute_handlers, msg, msg->msg_ctx);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_prepare_execute__send_request(st_xmysqlnd_msg__prepare_execute* msg,
				const st_xmysqlnd_pb_message_shell pb_message_shell)
{
	DBG_ENTER("xmysqlnd_prepare_execute__send_request");
	size_t bytes_sent;
	const enum_func_status ret = xmysqlnd_send_message(COM_PREPARE_EXECUTE,
								 *(google::protobuf::Message *)(pb_message_shell.message),
								 msg->msg_ctx,
								 &bytes_sent);
	DBG_RETURN(ret);
}

static st_xmysqlnd_msg__prepare_execute
xmysqlnd_prepare_execute__get_message(Message_context& msg_ctx)
{
	const st_xmysqlnd_msg__prepare_execute ctx =
	{
		xmysqlnd_prepare_execute__send_request,
		xmysqlnd_prepare_execute__read_response,
		xmysqlnd_prepare_execute__init_read,
		msg_ctx,

		{ nullptr, nullptr } /* on_error */
	};
	return ctx;
}

/**************************************  FACTORY **************************************************/

static st_xmysqlnd_msg__capabilities_get
xmysqlnd_msg_factory_get__capabilities_get(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_get_capabilities_get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__capabilities_set
xmysqlnd_msg_factory_get__capabilities_set(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_get_capabilities_set_message(factory->msg_ctx);
}


static st_xmysqlnd_msg__auth_start
xmysqlnd_msg_factory_get__auth_start(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_get_auth_start_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__sql_stmt_execute
xmysqlnd_msg_factory_get__sql_stmt_execute(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_get_sql_stmt_execute_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__session_reset
xmysqlnd_msg_factory_get__sess_reset(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_sess_reset__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__session_close
xmysqlnd_msg_factory_get__sess_close(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_sess_close__get_message(factory->msg_ctx);
}


static st_xmysqlnd_msg__connection_close
xmysqlnd_msg_factory_get__con_close(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_con_close__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__expectations_open
xmysqlnd_msg_factory_get__expectations_open(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_expectations_open__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__expectations_close
xmysqlnd_msg_factory_get__expectations_close(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_expectations_close__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__collection_add
xmysqlnd_msg_factory_get__collection_add(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_collection_add__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__collection_ud
xmysqlnd_msg_factory_get__collection_ud(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_collection_ud__get_message(factory->msg_ctx);
}


static st_xmysqlnd_msg__sql_stmt_execute
xmysqlnd_msg_factory_get__collection_read(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_msg_factory_get__sql_stmt_execute(factory);
}

static st_xmysqlnd_msg__table_insert
xmysqlnd_msg_factory_get__table_insert(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_table_insert__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_msg_factory_get__view_create(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_view_create__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_msg_factory_get__view_alter(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_view_alter__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__view_cmd
xmysqlnd_msg_factory_get__view_drop(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_view_drop__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__prepare_prepare
xmysqlnd_msg_factory_get__prepare_prepare(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_prepare_prepare__get_message(factory->msg_ctx);
}

static st_xmysqlnd_msg__prepare_execute
xmysqlnd_msg_factory_get__prepare_execute(st_xmysqlnd_message_factory* factory)
{
	return xmysqlnd_prepare_execute__get_message(factory->msg_ctx);
}


st_xmysqlnd_message_factory
get_message_factory(Message_context msg_ctx)
{
	st_xmysqlnd_message_factory factory
	{
		msg_ctx,
		xmysqlnd_msg_factory_get__capabilities_get,
		xmysqlnd_msg_factory_get__capabilities_set,
		xmysqlnd_msg_factory_get__auth_start,
		xmysqlnd_msg_factory_get__sql_stmt_execute,
		xmysqlnd_msg_factory_get__sess_reset,
		xmysqlnd_msg_factory_get__sess_close,
		xmysqlnd_msg_factory_get__con_close,
		xmysqlnd_msg_factory_get__expectations_open,
		xmysqlnd_msg_factory_get__expectations_close,
		xmysqlnd_msg_factory_get__collection_add,
		xmysqlnd_msg_factory_get__collection_ud,
		xmysqlnd_msg_factory_get__collection_read,
		xmysqlnd_msg_factory_get__table_insert,
		xmysqlnd_msg_factory_get__view_create,
		xmysqlnd_msg_factory_get__view_alter,
		xmysqlnd_msg_factory_get__view_drop,
		xmysqlnd_msg_factory_get__prepare_prepare,
		xmysqlnd_msg_factory_get__prepare_execute
	};
	return factory;
}

void
xmysqlnd_shutdown_protobuf_library()
{
	google::protobuf::ShutdownProtobufLibrary();
}

} // namespace drv

} // namespace mysqlx
