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
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_connection.h>
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "messages/mysqlx_message__capabilities.h"
#include "xmysqlnd_zval2any.h"

#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"

#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "proto_gen/mysqlx_resultset.pb.h"
#include "proto_gen/mysqlx_session.pb.h"
#include "proto_gen/mysqlx_sql.pb.h"


/* {{{ xmysqlnd_client_message_type_is_valid */
zend_bool
xmysqlnd_client_message_type_is_valid(enum xmysqlnd_client_message_type type)
{
	return Mysqlx::ClientMessages::Type_IsValid((Mysqlx::ClientMessages_Type) type);
}
/* }}} */


/* {{{ xmysqlnd_server_message_type_is_valid */
zend_bool
xmysqlnd_server_message_type_is_valid(zend_uchar type)
{
	DBG_ENTER("xmysqlnd_server_message_type_is_valid");
	zend_bool ret = Mysqlx::ServerMessages::Type_IsValid((Mysqlx::ServerMessages_Type) type);
	if (ret) {
		DBG_INF_FMT("TYPE=%s", Mysqlx::ServerMessages::Type_Name((Mysqlx::ServerMessages_Type) type).c_str());
	}
	DBG_RETURN(ret);
}
/* }}} */


#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"

/* {{{ xmysqlnd_send_protobuf_message */
static size_t
xmysqlnd_send_protobuf_message(struct st_mysqlx_node_connection * connection, struct st_mysqlx_node_pfc * codec,
							   enum xmysqlnd_client_message_type packet_type, ::google::protobuf::Message & message)
{
	size_t ret;
	DBG_ENTER("xmysqlnd_send_protobuf_message");

	const size_t payload_size = message.ByteSize();
	size_t bytes_sent;
	void * payload = payload_size? mnd_emalloc(payload_size) : NULL;
	if (payload_size && !payload) {
		php_error_docref(NULL, E_WARNING, "Memory allocation problem");
		
		DBG_RETURN(0);
	}
	message.SerializeToArray(payload, payload_size);
	ret = codec->pfc->data->m.send(codec->pfc, connection->vio,
								   packet_type,
								   (zend_uchar *) payload, payload_size,
								   &bytes_sent,
								   connection->stats,
								   connection->error_info);
	mnd_efree(payload);
	return bytes_sent;
}
/* }}} */

#define SIZE_OF_STACK_BUFFER 200

/* {{{ xmysqlnd_send_message */
static enum_func_status
xmysqlnd_send_message(enum xmysqlnd_client_message_type packet_type, ::google::protobuf::Message & message,
					  MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info,
					  size_t * bytes_sent)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_send_protobuf_message");
#ifdef PHP_DEBUG
	if (!xmysqlnd_client_message_type_is_valid(packet_type)) {
		SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The client wants to send invalid packet type");
		DBG_ERR_FMT("The client wants to send invalid packet type %d", (int) packet_type);
		DBG_RETURN(FAIL);	
	}
#endif
	char stack_buffer[SIZE_OF_STACK_BUFFER];
	void * payload = stack_buffer;

	const size_t payload_size = message.ByteSize();
	if (payload_size > sizeof(stack_buffer)) {
		payload = payload_size? mnd_emalloc(payload_size) : NULL;
		if (payload_size && !payload) {
			php_error_docref(NULL, E_WARNING, "Memory allocation problem");
			SET_OOM_ERROR(error_info);
			DBG_RETURN(FAIL);
		}
	}

	message.SerializeToArray(payload, payload_size);
	ret = pfc->data->m.send(pfc, vio, packet_type, (zend_uchar *) payload, payload_size, bytes_sent, stats, error_info);
	if (payload != stack_buffer) {
		mnd_efree(payload);
	}
	DBG_RETURN(ret);
}
/* }}} */



typedef enum xmysqlnd_handler_func_status
{
	HND_PASS = PASS,
	HND_FAIL = FAIL,
	HND_PASS_RETURN_FAIL,
	HND_AGAIN,
	HND_AGAIN_ASYNC,
} enum_hnd_func_status;



struct st_xmysqlnd_server_messages_handlers
{
	enum_hnd_func_status (*on_OK)(const Mysqlx::Ok & message, void * context);
	enum_hnd_func_status (*on_ERROR)(const Mysqlx::Error & message, void * context);
	enum_hnd_func_status (*on_CAPABILITIES)(const Mysqlx::Connection::Capabilities & message, void * context);
	enum_hnd_func_status (*on_AUTHENTICATE_CONTINUE)(const Mysqlx::Session::AuthenticateContinue & message, void * context);
	enum_hnd_func_status (*on_AUTHENTICATE_OK)(const Mysqlx::Session::AuthenticateOk & message, void * context);
	enum_hnd_func_status (*on_NOTICE)(const Mysqlx::Notice::Frame & message, void * context);
	enum_hnd_func_status (*on_COLUMN_META)(const Mysqlx::Resultset::ColumnMetaData & message, void * context);
	enum_hnd_func_status (*on_RSET_ROW)(const Mysqlx::Resultset::Row & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_DONE)(const Mysqlx::Resultset::FetchDone & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_SUSPENDED)(void * context); /*  there is no Mysqlx::Resultset::FetchSuspended*/
	enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_RSETS)(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context);
	enum_hnd_func_status (*on_STMT_EXECUTE_OK)(const Mysqlx::Sql::StmtExecuteOk & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_OUT_PARAMS)(const Mysqlx::Resultset::FetchDoneMoreOutParams & message, void * context);
	enum_hnd_func_status (*on_UNEXPECTED)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
	enum_hnd_func_status (*on_UNKNOWN)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
};


/* {{{ xmysqlnd_receive_message */
enum_func_status
xmysqlnd_receive_message(struct st_xmysqlnd_server_messages_handlers * handlers, void * handler_ctx,
						 MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	zend_uchar stack_buffer[SIZE_OF_STACK_BUFFER];
	enum_func_status ret = FAIL;
	enum_hnd_func_status hnd_ret;
	size_t payload_size;
	zend_uchar * payload;
	zend_uchar type;

	DBG_ENTER("xmysqlnd_receive_message");

	do {
		ret = pfc->data->m.receive(pfc, vio, stack_buffer, sizeof(stack_buffer), &type, &payload, &payload_size, stats, error_info);
		if (FAIL == ret) {
			DBG_RETURN(FAIL);
		}
		if (!xmysqlnd_server_message_type_is_valid(type)) {
			SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The server sent invalid packet type");
			DBG_ERR_FMT("Invalid packet type %u from the server", (uint) type);
			DBG_RETURN(FAIL);	
		}
		enum xmysqlnd_server_message_type packet_type = (enum xmysqlnd_server_message_type) type;
		hnd_ret = HND_PASS;
		bool handled = false;
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
			default:
				handled = true;
				if (handlers->on_UNKNOWN) {
					hnd_ret = handlers->on_UNKNOWN(type, payload, payload_size, handler_ctx);
				}
				SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Unknown type");
				DBG_ERR_FMT("Unknown type %d", (int) packet_type);
				break;
		}
		if (!handled) {
			DBG_INF_FMT("Unhandled message %d", packet_type);
			if (handlers->on_UNEXPECTED) {
				hnd_ret = handlers->on_UNEXPECTED(type, payload, payload_size, handler_ctx);
			}
		}
		if (payload != stack_buffer) {
			mnd_efree(payload);
		}
		if (hnd_ret == HND_AGAIN) {
			DBG_INF("HND_AGAIN. Reading new packet from the network");
		}
	} while (hnd_ret == HND_AGAIN);
	ret = (HND_PASS || HND_AGAIN_ASYNC)? PASS:FAIL;
	DBG_RETURN(ret);
}
/* }}} */


/************************************** CAPABILITIES GET **************************************************/
/* {{{ proto capabilities_to_zv */
static void
capabilities_to_zval(const Mysqlx::Connection::Capabilities & message, zval * return_value)
{
	DBG_ENTER("capabilities_to_zv");
	array_init_size(return_value, message.capabilities_size());
	for (unsigned int i = 0; i < message.capabilities_size(); ++i) {
		zval zv = {0};
		any2zval(message.capabilities(i).value(), &zv);
		if (Z_REFCOUNTED(zv)) {
			Z_ADDREF(zv);
		}
		add_assoc_zval_ex(return_value, message.capabilities(i).name().c_str(), message.capabilities(i).name().size(), &zv);
		zval_ptr_dtor(&zv);
	}
	DBG_VOID_RETURN;
}
/* }}} */




/* {{{ capabilities_get_on_ERROR */
static enum_hnd_func_status
capabilities_get_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_capabilities_get_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_get_message_ctx *>(context);
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ capabilities_get_on_CAPABILITIES */
static enum_hnd_func_status
capabilities_get_on_CAPABILITIES(const Mysqlx::Connection::Capabilities & message, void * context)
{
	struct st_xmysqlnd_capabilities_get_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_get_message_ctx *>(context);
	ctx->server_message_type = XMSG_CAPABILITIES;
	capabilities_to_zval(message, ctx->capabilities_zval);
	return HND_PASS;
}
/* }}} */


/* {{{ capabilities_get_on_NOTICE */
static enum_hnd_func_status
capabilities_get_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_capabilities_get_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_get_message_ctx *>(context);
	ctx->server_message_type = XMSG_NOTICE;
	return HND_AGAIN;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers capabilities_get_handlers =
{
	NULL, 							// on_OK
	capabilities_get_on_ERROR,		// on_ERROR
	capabilities_get_on_CAPABILITIES,// on_CAPABILITIES
	NULL,							// on_AUTHENTICATE_CONTINUE
	NULL,							// on_AUTHENTICATE_OK
	capabilities_get_on_NOTICE,		// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};


/* {{{ xmysqlnd_capabilities_get__read_response */
extern "C" enum_func_status
xmysqlnd_capabilities_get__read_response(struct st_xmysqlnd_capabilities_get_message_ctx * msg, zval * capabilities)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_capabilities_get__read_response");
	msg->capabilities_zval = capabilities;
	ret = xmysqlnd_receive_message(&capabilities_get_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_capabilities_get__send_request */
extern "C" enum_func_status
xmysqlnd_capabilities_get__send_request(struct st_xmysqlnd_capabilities_get_message_ctx * msg)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesGet message;
	return xmysqlnd_send_message(COM_CAPABILITIES_GET, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
}
/* }}} */




/* {{{ xmysqlnd_get_capabilities_get_message */
static struct st_xmysqlnd_capabilities_get_message_ctx
xmysqlnd_get_capabilities_get_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_capabilities_get_message_ctx ctx =
	{
		xmysqlnd_capabilities_get__send_request,
		xmysqlnd_capabilities_get__read_response,
		vio,
		pfc,
		stats,
		error_info,
		NULL,
		XMSG_NONE,
	};
	return ctx;
}
/* }}} */

/************************************** CAPABILITIES SET **************************************************/

#include "messages/mysqlx_message__ok.h"

/* {{{ capabilities_set_on_OK */
static enum_hnd_func_status
capabilities_set_on_OK(const Mysqlx::Ok & message, void * context)
{
	struct st_xmysqlnd_capabilities_set_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_set_message_ctx *>(context);
	ctx->server_message_type = XMSG_OK;
	if (ctx->return_value_zval) {
		mysqlx_new_message__ok(ctx->return_value_zval, message);
	}
}
/* }}} */


/* {{{ capabilities_set_on_ERROR */
static enum_hnd_func_status
capabilities_set_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_capabilities_set_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_set_message_ctx *>(context);
	DBG_ENTER("capabilities_set_on_ERROR");
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ capabilities_on_NOTICE */
static enum_hnd_func_status
capabilities_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_capabilities_set_message_ctx * ctx = static_cast<struct st_xmysqlnd_capabilities_set_message_ctx *>(context);
	ctx->server_message_type = XMSG_NOTICE;
	return HND_AGAIN;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers capabilities_set_handlers =
{
	capabilities_set_on_OK, 		// on_OK
	capabilities_set_on_ERROR,		// on_ERROR
	NULL,							// on_CAPABILITIES
	NULL,							// on_AUTHENTICATE_CONTINUE
	NULL,							// on_AUTHENTICATE_OK
	capabilities_on_NOTICE,
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_capabilities_set__read_response */
extern "C" enum_func_status
xmysqlnd_capabilities_set__read_response(struct st_xmysqlnd_capabilities_set_message_ctx * msg, zval * return_value)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_read__capabilities_set");
	msg->return_value_zval = return_value;
	ret = xmysqlnd_receive_message(&capabilities_set_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_send__capabilities_set */
extern "C" enum_func_status
xmysqlnd_capabilities_set__send_request(struct st_xmysqlnd_capabilities_set_message_ctx * msg,
										const size_t cap_count, zval ** capabilities_names, zval ** capabilities_values)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesSet message;
	for (unsigned i = 0; i < cap_count; ++i) {
		Mysqlx::Connection::Capability * capability = message.mutable_capabilities()->add_capabilities();
		capability->set_name(Z_STRVAL_P(capabilities_names[i]), Z_STRLEN_P(capabilities_names[i]));
		Mysqlx::Datatypes::Any any_entry;
		zval2any(capabilities_values[i], any_entry);
		capability->mutable_value()->CopyFrom(any_entry);
	}

	return xmysqlnd_send_message(COM_CAPABILITIES_SET, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
}
/* }}} */


/* {{{ xmysqlnd_get_capabilities_set_message */
static struct st_xmysqlnd_capabilities_set_message_ctx
xmysqlnd_get_capabilities_set_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_capabilities_set_message_ctx ctx = 
	{
		xmysqlnd_capabilities_set__send_request,
		xmysqlnd_capabilities_set__read_response,
		vio,
		pfc,
		stats,
		error_info,
		NULL,
		XMSG_NONE,
	};
	return ctx;
}
/* }}} */


/************************************** AUTH_START **************************************************/
/* {{{ auth_start_on_ERROR */
static enum_hnd_func_status
auth_start_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_auth_start_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_start_message_ctx *>(context);
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ auth_start_on_NOTICE */
static enum_hnd_func_status
auth_start_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_auth_start_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_start_message_ctx *>(context);
	ctx->server_message_type = XMSG_NOTICE;
	return HND_AGAIN;
}
/* }}} */

#include "messages/mysqlx_message__auth_continue.h"
#include "messages/mysqlx_message__auth_ok.h"

/* {{{ auth_start_on_AUTHENTICATE_CONTINUE */
static enum_hnd_func_status
auth_start_on_AUTHENTICATE_CONTINUE(const Mysqlx::Session::AuthenticateContinue & message, void * context)
{
	struct st_xmysqlnd_auth_start_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_start_message_ctx *>(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_CONTINUE");
	ctx->server_message_type = XMSG_AUTH_CONTINUE;
	if (ctx->auth_start_response_zval) {
		mysqlx_new_message__auth_continue(ctx->auth_start_response_zval, message);
	}
	if (message.has_auth_data()) {
		const zend_bool persistent = FALSE;
		ctx->out_auth_data.l = message.auth_data().size();
		ctx->out_auth_data.s = mnd_pestrndup(message.auth_data().c_str(), ctx->out_auth_data.l, persistent);
	}
	DBG_RETURN(HND_PASS);
}
/* }}} */


/* {{{ auth_start_on_AUTHENTICATE_OK */
static enum_hnd_func_status
auth_start_on_AUTHENTICATE_OK(const Mysqlx::Session::AuthenticateOk & message, void * context)
{
	struct st_xmysqlnd_auth_start_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_start_message_ctx *>(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_OK");
	ctx->server_message_type = XMSG_AUTH_OK;
	DBG_INF_FMT("ctx->auth_start_response_zval=%p", ctx->auth_start_response_zval);
	if (ctx->auth_start_response_zval) {
		mysqlx_new_message__auth_ok(ctx->auth_start_response_zval, message);
	}
	DBG_RETURN(HND_PASS);
}
/* }}} */



static struct st_xmysqlnd_server_messages_handlers auth_start_handlers =
{
	NULL, 							// on_OK
	auth_start_on_ERROR,			// on_ERROR
	NULL,							// on_CAPABILITIES
	auth_start_on_AUTHENTICATE_CONTINUE,// on_AUTHENTICATE_CONTINUE
	auth_start_on_AUTHENTICATE_OK,	// on_AUTHENTICATE_OK
	auth_start_on_NOTICE,			// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_authentication_start__read_response */
extern "C" enum_func_status
xmysqlnd_authentication_start__read_response(struct st_xmysqlnd_auth_start_message_ctx * msg, zval * auth_start_response)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_read__authentication_start");
	DBG_INF_FMT("auth_start_response=%p", auth_start_response);
	msg->auth_start_response_zval = auth_start_response;
	ret = xmysqlnd_receive_message(&auth_start_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_authentication_start__send_request */
extern "C" enum_func_status
xmysqlnd_authentication_start__send_request(struct st_xmysqlnd_auth_start_message_ctx * msg, const MYSQLND_CSTRING auth_mech_name, const MYSQLND_CSTRING auth_data)
{
	size_t bytes_sent;
	Mysqlx::Session::AuthenticateStart message;
	message.set_mech_name(auth_mech_name.s, auth_mech_name.l);
	message.set_auth_data(auth_data.s, auth_data.l);
	return xmysqlnd_send_message(COM_AUTH_START, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
}
/* }}} */


/* {{{ xmysqlnd_authentication_start__continue */
extern "C" zend_bool
xmysqlnd_authentication_start__continue(const struct st_xmysqlnd_auth_start_message_ctx * msg)
{
	return (msg->server_message_type == XMSG_AUTH_CONTINUE) ? TRUE:FALSE;
}
/* }}} */


/* {{{ xmysqlnd_authentication_start__ok */
extern "C" zend_bool
xmysqlnd_authentication_start__ok(const struct st_xmysqlnd_auth_start_message_ctx * msg)
{
	return (msg->server_message_type == XMSG_AUTH_OK) ? TRUE:FALSE;
}
/* }}} */


/* {{{ xmysqlnd_authentication_start__free_resources */
extern "C" void
xmysqlnd_authentication_start__free_resources(struct st_xmysqlnd_auth_start_message_ctx * msg)
{
	if (msg->out_auth_data.s) {
		mnd_efree(msg->out_auth_data.s);
		msg->out_auth_data.s = NULL;
		msg->out_auth_data.l = 0;
	}
}
/* }}} */


/* {{{ xmysqlnd_get_auth_start_message */
static struct st_xmysqlnd_auth_start_message_ctx
xmysqlnd_get_auth_start_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_auth_start_message_ctx ctx = 
	{
		xmysqlnd_authentication_start__send_request,
		xmysqlnd_authentication_start__read_response,
		xmysqlnd_authentication_start__continue,
		xmysqlnd_authentication_start__ok,
		xmysqlnd_authentication_start__free_resources,
		vio,
		pfc,
		stats,
		error_info,
		NULL,
		XMSG_NONE,
		{NULL, 0},
	};
	return ctx;
}
/* }}} */

/************************************** AUTH_CONTINUE **************************************************/
/* {{{ auth_continue_on_ERROR */
static enum_hnd_func_status
auth_continue_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_auth_continue_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_continue_message_ctx *>(context);
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ auth_continue_on_NOTICE */
static enum_hnd_func_status
auth_continue_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */

#include "messages/mysqlx_message__auth_continue.h"
#include "messages/mysqlx_message__auth_ok.h"

/* {{{ auth_continue_on_AUTHENTICATE_CONTINUE */
static enum_hnd_func_status
auth_continue_on_AUTHENTICATE_CONTINUE(const Mysqlx::Session::AuthenticateContinue & message, void * context)
{
	struct st_xmysqlnd_auth_continue_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_continue_message_ctx *>(context);
	DBG_ENTER("auth_continue_on_AUTHENTICATE_CONTINUE");
	ctx->server_message_type = XMSG_AUTH_CONTINUE;
	if (ctx->auth_continue_response_zval) {
		mysqlx_new_message__auth_continue(ctx->auth_continue_response_zval, message);
	}
	if (message.has_auth_data()) {
		const zend_bool persistent = FALSE;
		ctx->out_auth_data.l = message.auth_data().size();
		ctx->out_auth_data.s = mnd_pestrndup(message.auth_data().c_str(), ctx->out_auth_data.l, persistent);
	}
	DBG_RETURN(HND_PASS);
}
/* }}} */


/* {{{ auth_continue_on_AUTHENTICATE_OK */
static enum_hnd_func_status
auth_continue_on_AUTHENTICATE_OK(const Mysqlx::Session::AuthenticateOk & message, void * context)
{
	struct st_xmysqlnd_auth_continue_message_ctx * ctx = static_cast<struct st_xmysqlnd_auth_continue_message_ctx *>(context);
	DBG_ENTER("auth_continue_on_AUTHENTICATE_OK");
	ctx->server_message_type = XMSG_AUTH_OK;
	if (ctx->auth_continue_response_zval) {
		mysqlx_new_message__auth_ok(ctx->auth_continue_response_zval, message);
	}
	DBG_RETURN(HND_PASS);
}
/* }}} */



static struct st_xmysqlnd_server_messages_handlers auth_continue_handlers =
{
	NULL, 							// on_OK
	auth_continue_on_ERROR,			// on_ERROR
	NULL,							// on_CAPABILITIES
	auth_continue_on_AUTHENTICATE_CONTINUE,	// on_AUTHENTICATE_CONTINUE
	auth_continue_on_AUTHENTICATE_OK,		// on_AUTHENTICATE_OK
	auth_continue_on_NOTICE,		// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_authentication_continue__read_response */
extern "C" enum_func_status
xmysqlnd_authentication_continue__read_response(struct st_xmysqlnd_auth_continue_message_ctx * msg, zval * auth_continue_response)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_authentication_continue__read_response");
	msg->auth_continue_response_zval = auth_continue_response;
	ret = xmysqlnd_receive_message(&auth_continue_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */

extern "C"
{
#include "ext/mysqlnd/mysqlnd_auth.h" /* php_mysqlnd_scramble */
}

static const char hexconvtab[] = "0123456789abcdef";

/* {{{ xmysqlnd_send__authentication_continue */
extern "C" enum_func_status
xmysqlnd_authentication_continue__send_request(struct st_xmysqlnd_auth_continue_message_ctx * msg,
											   const MYSQLND_CSTRING schema,
											   const MYSQLND_CSTRING user,
											   const MYSQLND_CSTRING password,
											   const MYSQLND_CSTRING salt)
{
	size_t bytes_sent;
	Mysqlx::Session::AuthenticateContinue message;
	DBG_ENTER("xmysqlnd_authentication_continue__send_request");

	std::string response(schema.s, schema.l);
	response.append(1, '\0');
	response.append(user.s, user.l);
	response.append(1, '\0'); 
	if (password.s && password.l) {
		zend_uchar hash[SCRAMBLE_LENGTH];

		php_mysqlnd_scramble(hash, (zend_uchar*) salt.s, (const zend_uchar*) password.s, password.l);
		char hexed_hash[SCRAMBLE_LENGTH*2];
		for (unsigned int i = 0; i < SCRAMBLE_LENGTH; i++) {
			hexed_hash[i*2] = hexconvtab[hash[i] >> 4];
			hexed_hash[i*2 + 1] = hexconvtab[hash[i] & 15];
		}
		DBG_INF_FMT("hexed_hash=%s", hexed_hash);
		response.append(1, '*');
		response.append(hexed_hash, SCRAMBLE_LENGTH*2);
	}
	response.append(1, '\0');
	DBG_INF_FMT("response_size=%u", (uint) response.size());
	message.set_auth_data(response.c_str(), response.size());

	enum_func_status ret = xmysqlnd_send_message(COM_AUTH_CONTINUE, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_authentication_continue__continue */
extern "C" zend_bool
xmysqlnd_authentication_continue__continue(const struct st_xmysqlnd_auth_continue_message_ctx * msg)
{
	return (msg->server_message_type == XMSG_AUTH_CONTINUE) ? TRUE:FALSE;
}
/* }}} */


/* {{{ xmysqlnd_authentication_start__ok */
extern "C" zend_bool
xmysqlnd_authentication_continue__ok(const struct st_xmysqlnd_auth_continue_message_ctx * msg)
{
	return (msg->server_message_type == XMSG_AUTH_OK) ? TRUE:FALSE;
}
/* }}} */


/* {{{ xmysqlnd_authentication_continue__free_resources */
extern "C" void
xmysqlnd_authentication_continue__free_resources(struct st_xmysqlnd_auth_continue_message_ctx * msg)
{
	if (msg->out_auth_data.s) {
		mnd_efree(msg->out_auth_data.s);
		msg->out_auth_data.s = NULL;
		msg->out_auth_data.l = 0;
	}
}
/* }}} */


/* {{{ xmysqlnd_get_auth_continue_message */
static struct st_xmysqlnd_auth_continue_message_ctx
xmysqlnd_get_auth_continue_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_auth_continue_message_ctx ctx = 
	{
		xmysqlnd_authentication_continue__send_request,
		xmysqlnd_authentication_continue__read_response,
		xmysqlnd_authentication_continue__continue,
		xmysqlnd_authentication_continue__ok,
		xmysqlnd_authentication_continue__free_resources,
		vio,
		pfc,
		stats,
		error_info,
		NULL,
		XMSG_NONE,
		{NULL, 0}
	};
	return ctx;
}
/* }}} */

/**************************************  STMT_EXECUTE **************************************************/
#include "messages/mysqlx_message__ok.h"


/* {{{ stmt_execute_on_ERROR */
static enum_hnd_func_status
stmt_execute_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ xmysqlnd_inspect_warning */
static void
xmysqlnd_inspect_warning(const Mysqlx::Notice::Warning & warning, struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx)
{
	DBG_ENTER("xmysqlnd_inspect_warning");
	if (ctx->current_result) {
		const bool has_level = warning.has_level();
		const bool has_code = warning.has_code();
		const bool has_msg = warning.has_msg();
		const unsigned int code = has_code? warning.code() : 1000;
		const enum xmysqlnd_stmt_warning_level level = has_level? (enum xmysqlnd_stmt_warning_level) warning.level() : XSTMT_WARN_WARNING;
		const MYSQLND_CSTRING warn_message = { has_msg? warning.msg().c_str():"", has_msg? warning.msg().size():0 };

		DBG_INF_FMT("level[%s] is %s", has_level? "SET":"NOT SET",
									   has_level? Mysqlx::Notice::Warning::Level_Name(warning.level()).c_str() : "n/a");

		DBG_INF_FMT("code[%s] is %u", has_code? "SET":"NOT SET",
									  has_code? warning.code() : 0);

		DBG_INF_FMT("messsage[%s] is %s", has_msg? "SET":"NOT SET",
										  has_msg? warning.msg().c_str() : "n/a");

		ctx->current_result->data->warnings->m->add_warning(ctx->current_result->data->warnings, level, code, warn_message);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_inspect_changed_variable */
static void
xmysqlnd_inspect_changed_variable(const Mysqlx::Notice::SessionVariableChanged & message, struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx)
{
	DBG_ENTER("xmysqlnd_inspect_changed_variable");

	const bool has_param = message.has_param();
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? message.param().c_str() : "n/a");

	const bool has_value = message.has_value();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_inspect_changed_state */
static void
xmysqlnd_inspect_changed_state(const Mysqlx::Notice::SessionStateChanged & message, struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx)
{
	const bool has_param = message.has_param();
	const bool has_value = message.has_value();
	DBG_ENTER("xmysqlnd_inspect_changed_state");
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? Mysqlx::Notice::SessionStateChanged::Parameter_Name(message.param()).c_str() : "n/a");


	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_param && has_value) {
		switch (message.param()) {
			case Mysqlx::Notice::SessionStateChanged::CURRENT_SCHEMA:
				break;
			case Mysqlx::Notice::SessionStateChanged::ACCOUNT_EXPIRED:
				break;
			case Mysqlx::Notice::SessionStateChanged::GENERATED_INSERT_ID:
				break;
			case Mysqlx::Notice::SessionStateChanged::ROWS_AFFECTED:
				if (ctx->current_result) {
					ctx->current_result->data->exec_state->m->set_affected_items_count(ctx->current_result->data->exec_state, scalar2uint(message.value()));
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::ROWS_FOUND:
				if (ctx->current_result) {
					ctx->current_result->data->exec_state->m->set_found_items_count(ctx->current_result->data->exec_state, scalar2uint(message.value()));
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::ROWS_MATCHED:
				if (ctx->current_result) {
					ctx->current_result->data->exec_state->m->set_matched_items_count(ctx->current_result->data->exec_state, scalar2uint(message.value()));
				}
				break;
			case Mysqlx::Notice::SessionStateChanged::TRX_COMMITTED:
				break;
			case Mysqlx::Notice::SessionStateChanged::TRX_ROLLEDBACK:
				break;
			case Mysqlx::Notice::SessionStateChanged::PRODUCED_MESSAGE:
				break;
			case Mysqlx::Notice::SessionStateChanged::CLIENT_ID_ASSIGNED:
				break;
			default:
				DBG_ERR_FMT("Unknown param name %d", message.param());
				break;
		}
	}
	if (has_value) {
		scalar2log(message.value());
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_inspect_notice_frame */
static void
xmysqlnd_inspect_notice_frame(const Mysqlx::Notice::Frame & frame, struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx)
{
	DBG_ENTER("xmysqlnd_inspect_notice_frame");

	const bool has_scope = frame.has_scope();
	DBG_INF_FMT("scope[%s] is %s", has_scope? "SET":"NOT SET",
								   has_scope? Mysqlx::Notice::Frame::Scope_Name(frame.scope()).c_str() : "n/a");

	const bool has_payload = frame.has_payload();
	DBG_INF_FMT("payload is %s", has_payload? "SET":"NOT SET");

	const bool has_type = frame.has_type();

	DBG_INF_FMT("type is %s", has_type? "SET":"NOT SET");
	if (has_scope && frame.scope() == Mysqlx::Notice::Frame_Scope_LOCAL && has_type && has_payload) {
		switch (frame.type()) {
			case 1:{ /* Warning */
					Mysqlx::Notice::Warning message;
					DBG_INF("Warning");
					message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
					xmysqlnd_inspect_warning(message, ctx);
					break;
				}
			case 2:{ /* SessionVariableChanged */
					Mysqlx::Notice::SessionVariableChanged message;
					DBG_INF("SessionVariableChanged");
					message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
					xmysqlnd_inspect_changed_variable(message, ctx);
					break;
				}
			case 3:{ /* SessionStateChanged */
					Mysqlx::Notice::SessionStateChanged message;
					DBG_INF("SessionStateChanged");
					message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
					xmysqlnd_inspect_changed_state(message, ctx);
				}
				break;
			default:
				DBG_ERR_FMT("Unknown type %d", frame.type());
				break;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ stmt_execute_on_NOTICE */
static enum_hnd_func_status
stmt_execute_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	DBG_ENTER("stmt_execute_on_NOTICE");
	ctx->server_message_type = XMSG_NOTICE;
	xmysqlnd_inspect_notice_frame(message, ctx);
	/* In case of UPSERT we get
 | | | >xmysqlnd_dump_server_message
| | | | | info : packet is NOTICE   payload_size=14
| | | | | info : payload[14]=[08 03 10 02 1a 08 08 04 12 04 08 02 18 01 ]
| | | | | >xmysqlnd_dump_notice_frame
| | | | | | info : scope[SET] is LOCAL
| | | | | | info : payload is SET
| | | | | | info : type is SET
| | | | | | >xmysqlnd_dump_changed_state
| | | | | | | info : param[SET] is ROWS_AFFECTED
| | | | | | | info : value is SET
| | | | | | | >scalar2log
| | | | | | | | info : subtype=V_UINT
| | | | | | | | info : value=1
| | | | | | | <scalar2log
| | | | | | <xmysqlnd_dump_changed_state
| | | | | <xmysqlnd_dump_notice_frame
| | | | <xmysqlnd_dump_server_message

and

| | | | >xmysqlnd_dump_server_message
| | | | | info : packet is NOTICE   payload_size=14
| | | | | info : payload[14]=[08 03 10 02 1a 08 08 03 12 04 08 02 18 01 ]
| | | | | >xmysqlnd_dump_notice_frame
| | | | | | info : scope[SET] is LOCAL
| | | | | | info : payload is SET
| | | | | | info : type is SET
| | | | | | >xmysqlnd_dump_changed_state
| | | | | | | info : param[SET] is GENERATED_INSERT_ID
| | | | | | | info : value is SET
| | | | | | | >scalar2log
| | | | | | | | info : subtype=V_UINT
| | | | | | | | info : value=1
| | | | | | | <scalar2log
| | | | | | <xmysqlnd_dump_changed_state
| | | | | <xmysqlnd_dump_notice_frame
| | | | <xmysqlnd_dump_server_message

		These have to update the result set's last_insert_id and affected_items_count.
	*/
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


#include "mysqlx_resultset__column_metadata.h"

/* {{{ stmt_execute_on_COLUMN_META */
static enum_hnd_func_status
stmt_execute_on_COLUMN_META(const Mysqlx::Resultset::ColumnMetaData & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	DBG_ENTER("stmt_execute_on_COLUMN_META");
	ctx->has_more = TRUE;
	ctx->server_message_type = XMSG_COLUMN_METADATA;
	if (!ctx->current_meta && ctx->create_meta.create) {
		ctx->current_meta = ctx->create_meta.create(ctx->create_meta.ctx);
		ctx->current_result = NULL; /* so we don't use the previous result which could have been freed */
	}

	XMYSQLND_RESULT_FIELD_META * field = NULL;
	if (ctx->current_meta && ctx->create_meta_field.create) {
		XMYSQLND_RESULT_FIELD_META * field = ctx->create_meta_field.create(ctx->create_meta_field.ctx);
		if (!field) {
			ctx->current_meta->m->dtor(ctx->current_meta, ctx->stats, ctx->error_info);
			ctx->current_meta = NULL;
			SET_OOM_ERROR(ctx->error_info);
			DBG_RETURN(HND_FAIL);
		}
		if (message.has_type()) {
			field->m->set_type(field, (enum xmysqlnd_field_type) message.type());
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
		ctx->current_meta->m->add_field(ctx->current_meta, field, ctx->stats, ctx->error_info);
		DBG_RETURN(HND_AGAIN);
	} else if (ctx->response_zval) {
		mysqlx_new_column_metadata(ctx->response_zval, message);
		DBG_RETURN(HND_PASS); /* typically this should be HND_AGAIN */
	}
}
/* }}} */

#include "mysqlx_resultset__data_row.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>
#include <ext/mysqlnd/mysql_float_to_double.h>

/* {{{ xmysqlnd_row_to_zval_array */
static enum_func_status
xmysqlnd_row_to_zval_array(const Mysqlx::Resultset::Row & message,
						   const XMYSQLND_NODE_STMT_RESULT_META * meta,
						   zval * return_value)
{
	enum_func_status ret = PASS;
	const unsigned int column_count = meta->m->get_field_count(meta);
	unsigned int i;
	DBG_ENTER("xmysqlnd_row_to_zval_array");

	for (i = 0; i < column_count; ++i) {
		const uint8_t * buf = reinterpret_cast<const uint8_t*>(message.field(i).c_str());
		const size_t buf_size = message.field(i).size();
		zval * zv = &(return_value[i]);
		DBG_INF_FMT("buf_size=%u", (uint) buf_size);
		for (unsigned j = 0; j < buf_size; j++) {
			DBG_INF_FMT("[%02u]=x%02X", j, buf[j]);
		}
		/*
		  Precaution, as if something misbehaves and doesn't initialize `zv` then `zv` will be at
		  the same place in the stack and have the previous value. String reuse will lead to
		  double-free and a crash.
		*/
		ZVAL_NULL(zv);
		if (buf_size == 0) {
			goto skip_switch;
		}
		switch (meta->m->get_field(meta, i)->type) {
			case XMYSQLND_TYPE_SIGNED_INT:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 gval;
				DBG_INF("SINT");
				if (input_stream.ReadVarint64(&gval)) {
					int64_t ival = ::google::protobuf::internal::WireFormatLite::ZigZagDecode64(gval);
#if SIZEOF_ZEND_LONG==4
					if (UNEXPECTED(ival >= ZEND_LONG_MAX)) {
						ZVAL_NEW_STR(zv, strpprintf(0, MYSQLND_LLU_SPEC, ival));
					} else
#endif
					{
						ZVAL_LONG(zv, ival);
					}
				} else {
					php_error_docref(NULL, E_WARNING, "Error decoding SINT");
					ret = FAIL;
				}
				break;
			}
			case XMYSQLND_TYPE_BIT:
				DBG_INF("BIT handled as UINT");
			case XMYSQLND_TYPE_UNSIGNED_INT:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 gval;
				DBG_INF("UINT");
				if (input_stream.ReadVarint64(&gval)) {
#if SIZEOF_ZEND_LONG==8
					if (gval > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
					if (gval > L64(2147483647) {
#endif
						ZVAL_NEW_STR(zv, strpprintf(0, MYSQLND_LLU_SPEC, gval));
					} else {
						ZVAL_LONG(zv, gval);
					}
				} else {
					php_error_docref(NULL, E_WARNING, "Error decoding UINT");
					ret = FAIL;
				}
				break;
			}
			case XMYSQLND_TYPE_DOUBLE:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 gval;
				DBG_INF("DOUBLE");
				if (input_stream.ReadLittleEndian64(&gval)) {
					ZVAL_DOUBLE(zv, ::google::protobuf::internal::WireFormatLite::DecodeDouble(gval));
				} else {
					php_error_docref(NULL, E_WARNING, "Error decoding DOUBLE");
					ZVAL_NULL(zv);
					ret = FAIL;
				}
				break;
			}
			case XMYSQLND_TYPE_FLOAT:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint32 gval;
				DBG_INF("FLOAT");
				if (input_stream.ReadLittleEndian32(&gval)) {
					const float fval = ::google::protobuf::internal::WireFormatLite::DecodeFloat(gval);
					const unsigned int fractional_digits = meta->m->get_field(meta, i)->fractional_digits;
#ifndef NOT_FIXED_DEC
# define NOT_FIXED_DEC 31
#endif
					const double dval = mysql_float_to_double(fval, (fractional_digits >= NOT_FIXED_DEC) ? -1 : fractional_digits);

					ZVAL_DOUBLE(zv, dval);
				} else {
					php_error_docref(NULL, E_WARNING, "Error decoding FLOAT");
					ret = FAIL;
				}
				break;
			}
			case XMYSQLND_TYPE_ENUM:
				DBG_INF("ENUM handled as STRING");
			case XMYSQLND_TYPE_BYTES:{
				if (buf_size) {
					DBG_INF("STRING");
					ZVAL_STRINGL(zv, reinterpret_cast<const char *>(buf), buf_size - 1); /* skip the ending \0 */
				} else {
					DBG_INF("NULL");
					ZVAL_NULL(zv);
					ret = FAIL;
				}
				break;
			}
			case XMYSQLND_TYPE_TIME:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 neg = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
				DBG_INF("TIME");
				if (!buf_size) {
					break;
				}
				if (buf_size == 1) {
					if (!buf[0]) {
						#define	TIME_NULL_VALUE "00:00:00.00"
						ZVAL_NEW_STR(&(return_value[i]), zend_string_init(TIME_NULL_VALUE, sizeof(TIME_NULL_VALUE)-1, 0));
						#undef TIME_NULL_VALUE
					} else {
						ZVAL_NULL(zv);
						php_error_docref(NULL, E_WARNING, "Unexpected value %d for first byte of TIME", (uint)(buf[0]));
						ret = FAIL;
					}
					break;
				}
				do {
					if (!input_stream.ReadVarint64(&neg)) break;		DBG_INF_FMT("neg  ="MYSQLND_LLU_SPEC, neg);
					if (!input_stream.ReadVarint64(&hours)) break;		DBG_INF_FMT("hours="MYSQLND_LLU_SPEC, hours);
					if (!input_stream.ReadVarint64(&minutes)) break;	DBG_INF_FMT("mins ="MYSQLND_LLU_SPEC, minutes);
					if (!input_stream.ReadVarint64(&seconds)) break;	DBG_INF_FMT("secs ="MYSQLND_LLU_SPEC, seconds);
					if (!input_stream.ReadVarint64(&useconds)) break;	DBG_INF_FMT("usecs="MYSQLND_LLU_SPEC, useconds);
				} while (0);
				#define TIME_FMT_STR "%s%02u:%02u:%02u.%08u"
				ZVAL_NEW_STR(zv, strpprintf(0, TIME_FMT_STR , neg? "-":"",
										    (unsigned int) hours,
											(unsigned int) minutes,
											(unsigned int) seconds,
											(unsigned int) useconds));
				#undef TIME_FMT_STR
				break;
			}
			case XMYSQLND_TYPE_DATETIME:{
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 year = 0, month = 0, day = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
				DBG_INF("DATETIME");
				if (!buf_size) {
					break;
				}
				if (buf_size == 1) {
					if (!buf[0]) {
						#define	DATETIME_NULL_VALUE "0000-00-00 00:00:00.00"
						ZVAL_NEW_STR(&(return_value[i]), zend_string_init(DATETIME_NULL_VALUE, sizeof(DATETIME_NULL_VALUE)-1, 0));
						#undef DATETIME_NULL_VALUE
					} else {
						php_error_docref(NULL, E_WARNING, "Unexpected value %d for first byte of TIME", (uint)(buf[0]));
						ret = FAIL;
					}
					break;
				}
				do {
					if (!input_stream.ReadVarint64(&year)) break; 		DBG_INF_FMT("year ="MYSQLND_LLU_SPEC, year);
					if (!input_stream.ReadVarint64(&month)) break;		DBG_INF_FMT("month="MYSQLND_LLU_SPEC, month);
					if (!input_stream.ReadVarint64(&day)) break;		DBG_INF_FMT("day  ="MYSQLND_LLU_SPEC, day);
					if (!input_stream.ReadVarint64(&hours)) break;		DBG_INF_FMT("hours="MYSQLND_LLU_SPEC, hours);
					if (!input_stream.ReadVarint64(&minutes)) break;	DBG_INF_FMT("mins ="MYSQLND_LLU_SPEC, minutes);
					if (!input_stream.ReadVarint64(&seconds)) break;	DBG_INF_FMT("secs ="MYSQLND_LLU_SPEC, seconds);
					if (!input_stream.ReadVarint64(&useconds)) break;	DBG_INF_FMT("usecs="MYSQLND_LLU_SPEC, useconds);
				} while (0);
				#define DATETIME_FMT_STR "%04u-%02u-%02u %02u:%02u:%02u"
				ZVAL_NEW_STR(zv, strpprintf(0, DATETIME_FMT_STR ,
											 (unsigned int) year,
											 (unsigned int) month,
											 (unsigned int) day,
											 (unsigned int) hours,
											 (unsigned int) minutes,
											 (unsigned int) seconds,
											 (unsigned int) useconds));
				#undef DATETIME_FMT_STR
				break;
			}
			case XMYSQLND_TYPE_SET:{
				DBG_INF("SET");
				::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
				::google::protobuf::uint64 gval;
				bool length_read_ok = true;
				array_init(zv);
				if (buf_size == 1 && buf[0] == 0x1) { /* Empty set */
					break;
				}
				while (length_read_ok) {
					if ((length_read_ok = input_stream.ReadVarint64(&gval))) {
						char * set_value = NULL;
						int rest_buffer_size = 0;
						if (input_stream.GetDirectBufferPointer((const void**) &set_value, &rest_buffer_size)) {
							zval set_entry;
							DBG_INF_FMT("value length=%3u  rest_buffer_size=%3d", (uint) gval, rest_buffer_size);
							if (gval > rest_buffer_size) {
								php_error_docref(NULL, E_WARNING, "Length pointing outside of the buffer");
								ret = FAIL;
								break;
							}
							ZVAL_STRINGL(&set_entry, set_value, gval);
							zend_hash_next_index_insert(Z_ARRVAL_P(zv), &set_entry);
							if (!input_stream.Skip(gval)) {
								break;
							}
						}
					}
				}
				DBG_INF_FMT("set elements=%u", zend_hash_num_elements(Z_ARRVAL_P(zv)));
				break;
			}
			case XMYSQLND_TYPE_DECIMAL:{
				DBG_INF("DECIMAL");
				if (!buf_size) {
					break;
				}
				if (buf_size == 1) {
					php_error_docref(NULL, E_WARNING, "Unexpected value %d for first byte of TIME");
				}
				const uint8_t scale = buf[0];
				const uint8_t last_byte = buf[buf_size - 1]; /* last byte is the sign and the last 4 bits, if any */
				const uint8_t sign = ((last_byte & 0xF)? last_byte  : last_byte >> 4) & 0xF;
				const size_t digits = (buf_size - 2 /* scale & last */) * 2  + ((last_byte & 0xF) > 0x9? 1:0);
				DBG_INF_FMT("scale     =%u", (uint) scale);
				DBG_INF_FMT("sign      =%u", (uint) sign);
				DBG_INF_FMT("digits    =%u", (uint) digits);
				if (!digits) {
					php_error_docref(NULL, E_WARNING, "Wrong value for DECIMAL. scale=%u  last_byte=%u", (uint) scale, last_byte);
					ret = FAIL;
					break;
				}
				const size_t d_val_len = digits + (sign == 0xD? 1:0) + (digits > scale? 1:0); /* one for the dot, one for the sign*/
				char * d_val = new char [d_val_len + 1];
				d_val[d_val_len] = '\0';
				char * p = d_val;
				if (sign == 0xD) {
					*(p++) = '-';
				}
				const size_t dot_position = digits - scale - 1;
				for (unsigned int pos = 0; pos < digits; ++pos) {
					const size_t offset = 1 + (pos >> 1);
					/* if uneven (&0x01) then use the second 4-bits, otherwise shift (>>) the first 4 to the right and then use them */
					const uint8_t digit = (pos & 0x01 ? buf[offset] : buf[offset] >> 4) & 0x0F; 
					*(p++) = '0' + digit;
					if (pos == dot_position) {
						*(p++) = '.';
					}
				}
				DBG_INF_FMT("value=%*s", d_val_len, d_val);
				ZVAL_STRINGL(zv, d_val, d_val_len);
				delete [] d_val;
				break;
			}
		}
skip_switch:
		;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ stmt_execute_on_RSET_ROW */
static enum_hnd_func_status
stmt_execute_on_RSET_ROW(const Mysqlx::Resultset::Row & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->has_more = TRUE;
	ctx->server_message_type = XMSG_RSET_ROW;
	if (ctx->response_zval) {
		mysqlx_new_data_row(ctx->response_zval, message);
		return HND_PASS;
	}
	if (!ctx->current_result && ctx->create_result.create) {
		ctx->current_result = ctx->create_result.create(ctx->create_result.ctx);
		if (ctx->current_result && ctx->current_meta) {
			ctx->current_result->data->m.attach_meta(ctx->current_result, ctx->current_meta, ctx->stats, ctx->error_info);
		}
	}
	if (ctx->current_result) {
		zval * row = ctx->current_result->data->m.create_row(ctx->current_result, ctx->current_meta, ctx->stats, ctx->error_info);
		if (row && PASS == xmysqlnd_row_to_zval_array(message, ctx->current_meta, row)) {
			ctx->current_result->data->m.add_row(ctx->current_result, row, ctx->current_meta, ctx->stats, ctx->error_info);
		}

		return HND_AGAIN;
	}
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE(const Mysqlx::Resultset::FetchDone & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_RSET_FETCH_DONE;
	ctx->current_meta = NULL;
	ctx->has_more = FALSE;
	return HND_AGAIN; /* After FETCH_DONE a STMT_EXECUTE_OK is expected */
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_SUSPENDED */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_SUSPENDED(void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMGS_RSET_FETCH_SUSPENDED;
	ctx->has_more = TRUE;
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_RSET_FETCH_DONE_MORE_RSETS;
	ctx->current_meta = NULL;
	ctx->has_more = TRUE;
//	return HND_AGAIN;
	return HND_PASS;
}
/* }}} */

#include "messages/mysqlx_message__stmt_execute_ok.h"

/* {{{ stmt_execute_on_STMT_EXECUTE_OK */
static enum_hnd_func_status
stmt_execute_on_STMT_EXECUTE_OK(const Mysqlx::Sql::StmtExecuteOk & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_STMT_EXECUTE_OK;
	ctx->has_more = FALSE;
	if (ctx->response_zval) {
		mysqlx_new_stmt_execute_ok(ctx->response_zval, message);
	}
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS(const Mysqlx::Resultset::FetchDoneMoreOutParams & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->has_more = TRUE;
	return HND_PASS;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers stmt_execute_handlers =
{
	NULL,				 				// on_OK
	stmt_execute_on_ERROR,				// on_ERROR
	NULL,								// on_CAPABILITIES
	NULL,								// on_AUTHENTICATE_CONTINUE
	NULL,								// on_AUTHENTICATE_OK
	stmt_execute_on_NOTICE,				// on_NOTICE
	stmt_execute_on_COLUMN_META,		// on_RSET_COLUMN_META
	stmt_execute_on_RSET_ROW,			// on_RSET_ROW
	stmt_execute_on_RSET_FETCH_DONE,	// on_RSET_FETCH_DONE
	stmt_execute_on_RSET_FETCH_SUSPENDED,			// on_RESULTSET_FETCH_SUSPENDED
	stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS,		// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	stmt_execute_on_STMT_EXECUTE_OK,				// on_SQL_STMT_EXECUTE_OK
	stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS,// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,								// on_UNEXPECTED
	NULL,								// on_UNKNOWN
};

/* {{{ xmysqlnd_sql_stmt_execute__read_response */
extern "C" enum_func_status
xmysqlnd_sql_stmt_execute__read_response(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg,
										 struct st_xmysqlnd_result_create_bind create_result,
										 struct st_xmysqlnd_meta_create_bind create_meta,
										 struct st_xmysqlnd_meta_field_create_bind create_meta_field,
										 zval * response)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_read__stmt_execute");
	msg->response_zval = response;
	msg->create_result = create_result;
	msg->create_meta = create_meta;
	msg->create_meta_field = create_meta_field;
	msg->current_result = NULL;
	msg->current_meta = NULL;

	ret = xmysqlnd_receive_message(&stmt_execute_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_sql_stmt_execute__send_request */
extern "C" enum_func_status
xmysqlnd_sql_stmt_execute__send_request(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg,
										const MYSQLND_CSTRING namespace_,
										const MYSQLND_CSTRING stmt,
										const zend_bool compact_meta)
{
	size_t bytes_sent;
	Mysqlx::Sql::StmtExecute message;

	message.set_namespace_(namespace_.s, namespace_.l);
	message.set_stmt(stmt.s, stmt.l);
	message.set_compact_metadata(compact_meta? true:false);
	return xmysqlnd_send_message(COM_SQL_STMT_EXECUTE, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
}
/* }}} */



/* {{{ xmysqlnd_get_sql_stmt_execute_message */
static struct st_xmysqlnd_sql_stmt_execute_message_ctx
xmysqlnd_get_sql_stmt_execute_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx ctx = 
	{
		xmysqlnd_sql_stmt_execute__send_request,
		xmysqlnd_sql_stmt_execute__read_response,
		vio,
		pfc,
		stats,
		error_info,
		NULL, /* current_result */
		NULL, /* current_meta */
		{ NULL, NULL}, /* create result */
		{ NULL, NULL}, /* create meta */
		{ NULL, NULL}, /* create meta field */
		FALSE, /* has_more */
		NULL, /* response_zval */
		XMSG_NONE,
	};
	return ctx;
}
/* }}} */


/**************************************  CON_CLOSE **************************************************/
/* {{{ con_close_on_OK */
static enum_hnd_func_status
con_close_on_OK(const Mysqlx::Ok & message, void * context)
{
	struct st_xmysqlnd_connection_close_ctx * ctx = static_cast<struct st_xmysqlnd_connection_close_ctx *>(context);
	ctx->server_message_type = XMSG_OK;
	return HND_PASS;
}
/* }}} */


/* {{{ con_close_on_ERROR */
static enum_hnd_func_status
con_close_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_xmysqlnd_connection_close_ctx * ctx = static_cast<struct st_xmysqlnd_connection_close_ctx *>(context);
	ctx->server_message_type = XMSG_ERROR;
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ con_close_on_NOTICE */
static enum_hnd_func_status
con_close_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_connection_close_ctx * ctx = static_cast<struct st_xmysqlnd_connection_close_ctx *>(context);
	ctx->server_message_type = XMSG_NOTICE;
	return HND_AGAIN;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers con_close_handlers =
{
	con_close_on_OK,		// on_OK
	con_close_on_ERROR,		// on_ERROR
	NULL,					// on_CAPABILITIES
	NULL,					// on_AUTHENTICATE_CONTINUE
	NULL,					// on_AUTHENTICATE_OK
	con_close_on_NOTICE,	// on_NOTICE
	NULL,					// on_RSET_COLUMN_META
	NULL,					// on_RSET_ROW
	NULL,					// on_RSET_FETCH_DONE
	NULL,					// on_RESULTSET_FETCH_SUSPENDED
	NULL,					// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,					// on_SQL_STMT_EXECUTE_OK
	NULL,					// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,					// on_UNEXPECTED
	NULL,					// on_UNKNOWN
};


/* {{{ xmysqlnd_con_close__read_response */
extern "C" enum_func_status
xmysqlnd_con_close__read_response(struct st_xmysqlnd_connection_close_ctx * msg)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_con_close__read_response");
	ret = xmysqlnd_receive_message(&con_close_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_con_close__send_request */
extern "C" enum_func_status
xmysqlnd_con_close__send_request(struct st_xmysqlnd_connection_close_ctx * msg)
{
	size_t bytes_sent;
	Mysqlx::Session::Close message;

	return xmysqlnd_send_message(COM_CONN_CLOSE, message, msg->vio, msg->pfc, msg->stats, msg->error_info, &bytes_sent);
}
/* }}} */



/* {{{ xmysqlnd_con_close__get_message */
static struct st_xmysqlnd_connection_close_ctx
xmysqlnd_con_close__get_message(MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_connection_close_ctx ctx = 
	{
		xmysqlnd_con_close__send_request,
		xmysqlnd_con_close__read_response,
		vio,
		pfc,
		stats,
		error_info,
		XMSG_NONE,
	};
	return ctx;
}
/* }}} */


/**************************************  FACTORY **************************************************/

/* {{{ xmysqlnd_get_capabilities_get_message_aux */
static struct st_xmysqlnd_capabilities_get_message_ctx
xmysqlnd_get_capabilities_get_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_get_capabilities_get_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_capabilities_set_message_aux */
static struct st_xmysqlnd_capabilities_set_message_ctx
xmysqlnd_get_capabilities_set_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_get_capabilities_set_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_auth_start_message_aux */
static struct st_xmysqlnd_auth_start_message_ctx
xmysqlnd_get_auth_start_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_get_auth_start_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_auth_continue_message_aux */
static struct st_xmysqlnd_auth_continue_message_ctx
xmysqlnd_get_auth_continue_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_get_auth_continue_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_sql_stmt_execute_message_aux */
static struct st_xmysqlnd_sql_stmt_execute_message_ctx
xmysqlnd_get_sql_stmt_execute_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_get_sql_stmt_execute_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_con_close_message_aux */
static struct st_xmysqlnd_connection_close_ctx
xmysqlnd_get_con_close_message_aux(const struct st_xmysqlnd_message_factory * const factory)
{
	return xmysqlnd_con_close__get_message(factory->vio, factory->pfc, factory->stats, factory->error_info);
}
/* }}} */


/* {{{ xmysqlnd_get_message_factory */
extern "C" struct st_xmysqlnd_message_factory
xmysqlnd_get_message_factory(const XMYSQLND_L3_IO * const io, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	struct st_xmysqlnd_message_factory factory = 
	{
		io->vio,
		io->pfc,
		stats,
		error_info,
		xmysqlnd_get_capabilities_get_message_aux,
		xmysqlnd_get_capabilities_set_message_aux,
		xmysqlnd_get_auth_start_message_aux,
		xmysqlnd_get_auth_continue_message_aux,
		xmysqlnd_get_sql_stmt_execute_message_aux,
		xmysqlnd_get_con_close_message_aux,
	};
	return factory;
}
/* }}} */


/*
 * Local variables:{
 * tab-width: 4
 * c-basic-offset: 4
 * End:{
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
