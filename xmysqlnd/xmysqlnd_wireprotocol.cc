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
#include "xmysqlnd_node_query_result_meta.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "proto_gen/mysqlx_notice.pb.h"
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


/* {{{ stmt_execute_on_NOTICE */
static enum_hnd_func_status
stmt_execute_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_NOTICE;
	return HND_AGAIN;
}
/* }}} */


#include "mysqlx_resultset__column_metadata.h"

/* {{{ stmt_execute_on_COLUMN_META */
static enum_hnd_func_status
stmt_execute_on_COLUMN_META(const Mysqlx::Resultset::ColumnMetaData & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	const zend_bool persistent = ctx->session? ctx->session->persistent : FALSE;
	DBG_ENTER("stmt_execute_on_COLUMN_META");

	ctx->server_message_type = XMSG_COLUMN_METADATA;
	if (ctx->session) {
		if (!ctx->result_meta) {
			ctx->result_meta = xmysqlnd_node_query_result_meta_init(ctx->session->persistent, &ctx->session->object_factory, ctx->stats, ctx->error_info);
			if (!ctx->result_meta) {
				SET_OOM_ERROR(ctx->error_info);
				DBG_RETURN(HND_FAIL);
			}
		}
		XMYSQLND_RESULT_FIELD_META * field = xmysqlnd_result_field_meta_init(ctx->session->persistent, &ctx->session->object_factory, ctx->stats, ctx->error_info);
		if (!field) {
			ctx->result_meta->m->dtor(ctx->result_meta, ctx->stats, ctx->error_info);
			ctx->result_meta = NULL;
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
		ctx->result_meta->m->add_field(ctx->result_meta, field, ctx->stats, ctx->error_info);
		
//		xmysqlnd_node_query_result_meta_free(ctx->result_meta, ctx->stats, ctx->error_info);
		DBG_RETURN(HND_AGAIN);
	} else if (ctx->response_zval) {
		mysqlx_new_column_metadata(ctx->response_zval, message);
		DBG_RETURN(HND_PASS); /* typically this should be HND_AGAIN */
	}
}
/* }}} */

#include "mysqlx_resultset__data_row.h"

/* {{{ stmt_execute_on_RSET_ROW */
static enum_hnd_func_status
stmt_execute_on_RSET_ROW(const Mysqlx::Resultset::Row & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_RSET_ROW;
	if (ctx->response_zval) {
		mysqlx_new_data_row(ctx->response_zval, message);
	}
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE(const Mysqlx::Resultset::FetchDone & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_RSET_FETCH_DONE;
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_SUSPENDED */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_SUSPENDED(void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMGS_RSET_FETCH_SUSPENDED;
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context)
{
	struct st_xmysqlnd_sql_stmt_execute_message_ctx * ctx = static_cast<struct st_xmysqlnd_sql_stmt_execute_message_ctx *>(context);
	ctx->server_message_type = XMSG_RSET_FETCH_DONE_MORE_RSETS;
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
xmysqlnd_sql_stmt_execute__read_response(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg, XMYSQLND_NODE_SESSION_DATA * const session, zval * response)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_read__stmt_execute");
	msg->session = session;
	msg->response_zval = response;
	ret = xmysqlnd_receive_message(&stmt_execute_handlers, msg, msg->vio, msg->pfc, msg->stats, msg->error_info);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_sql_stmt_execute__send_request */
extern "C" enum_func_status
xmysqlnd_sql_stmt_execute__send_request(struct st_xmysqlnd_sql_stmt_execute_message_ctx * msg,
										const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt, const zend_bool compact_meta)
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
		NULL, /* session */
		NULL, /* result_meta */
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
