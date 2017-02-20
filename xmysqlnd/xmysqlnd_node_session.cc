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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_charset.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_auth.h" /* php_mysqlnd_scramble */
}
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h"
#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_protocol_dumper.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_structs.h"
#include "php_mysqlx.h"

namespace mysqlx {

namespace drv {

const MYSQLND_CSTRING namespace_sql = { "sql", sizeof("sql") - 1 };
const MYSQLND_CSTRING namespace_xplugin = { "xplugin", sizeof("xplugin") - 1 };


const struct st_xmysqlnd_node_session_query_bind_variable_bind noop__var_binder = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_result_start_bind noop__on_result_start = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_row_bind noop__on_row = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_warning_bind noop__on_warning = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_error_bind noop__on_error = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_result_end_bind noop__on_result_end = { NULL, NULL };
const struct st_xmysqlnd_node_session_on_statement_ok_bind noop__on_statement_ok = { NULL, NULL };


/* {{{ xmysqlnd_node_session_state::get */
static enum xmysqlnd_node_session_state
XMYSQLND_METHOD(xmysqlnd_node_session_state, get)(const XMYSQLND_NODE_SESSION_STATE * const state_struct)
{
	DBG_ENTER("xmysqlnd_node_session_state::get");
	DBG_INF_FMT("State=%u", state_struct->state);
	DBG_RETURN(state_struct->state);
}
/* }}} */


/* {{{ xmysqlnd_node_session_state::set */
static void
XMYSQLND_METHOD(xmysqlnd_node_session_state, set)(XMYSQLND_NODE_SESSION_STATE * const state_struct, const enum xmysqlnd_node_session_state state)
{
	DBG_ENTER("xmysqlnd_node_session_state::set");
	DBG_INF_FMT("New state=%u", state);
	state_struct->state = state;
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session_state)
	XMYSQLND_METHOD(xmysqlnd_node_session_state, get),
	XMYSQLND_METHOD(xmysqlnd_node_session_state, set),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_session_state_init */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_session_state_init(XMYSQLND_NODE_SESSION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_node_session_state_init");
	state->m = &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_session_state);
	state->state = NODE_SESSION_ALLOCED;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, init)(XMYSQLND_NODE_SESSION_DATA * object,
												 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
												 MYSQLND_STATS * stats,
												 MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_session_data::init");

	object->object_factory = factory;

	if (error_info) {
		object->error_info = error_info? error_info : &object->error_info_impl;
	} else {
		if (FAIL == mysqlnd_error_info_init(&object->error_info_impl, object->persistent)) {
			DBG_RETURN(FAIL);
		}
		object->error_info = &object->error_info_impl;
	}

	object->options = &(object->options_impl);

	xmysqlnd_node_session_state_init(&object->state);

	if (stats) {
		object->stats = stats;
		object->own_stats = FALSE;
	} else {
		mysqlnd_stats_init(&object->stats, STAT_LAST, object->persistent);
		object->own_stats = TRUE;
	}

	object->io.pfc = xmysqlnd_pfc_create(object->persistent, object->object_factory, object->stats, object->error_info);
	object->io.vio = mysqlnd_vio_init(object->persistent, NULL, object->stats, object->error_info);

	object->charset = mysqlnd_find_charset_name(XMYSQLND_NODE_SESSION_CHARSET);

	if (!object->io.pfc || !object->io.vio || !object->charset) {
		DBG_RETURN(FAIL);
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_scheme */
static MYSQLND_STRING
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_scheme)(XMYSQLND_NODE_SESSION_DATA * session,
														MYSQLND_CSTRING hostname,
														MYSQLND_CSTRING * socket_or_pipe,
														unsigned int port,
														zend_bool * unix_socket,
														zend_bool * named_pipe)
{
	MYSQLND_STRING transport;
	DBG_ENTER("xmysqlnd_node_session_data::get_scheme");
#ifdef PHP_WIN32
	if (hostname.l == sizeof(".") - 1 && hostname.s[0] == '.') {
		/* named pipe in socket */
		if (!socket_or_pipe->s) {
			socket_or_pipe->s = "\\\\.\\pipe\\MySQL";
			socket_or_pipe->l = strlen(socket_or_pipe->s);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "pipe://%s", socket_or_pipe->s);
		*named_pipe = TRUE;
	}
	else
#endif
	{
		if (!port) {
			port = 3306;
		}
		transport.l = mnd_sprintf(&transport.s, 0, "tcp://%s:%u", hostname.s, port);
	}
	DBG_INF_FMT("transport=%s", transport.s? transport.s:"OOM");
	DBG_RETURN(transport);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_warning */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_error)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	XMYSQLND_NODE_SESSION_DATA * session = (XMYSQLND_NODE_SESSION_DATA *) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_error");
	if (session->error_info) {
		SET_CLIENT_ERROR(session->error_info, code, sql_state.s, message.s);
	}
	php_error_docref(NULL, E_WARNING, "[%4u][%s] %s", code, sql_state.s? sql_state.s:"", message.s? message.s:"");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

struct st_xmysqlnd_auth_41_ctx
{
	XMYSQLND_NODE_SESSION_DATA * session;
	MYSQLND_CSTRING scheme;
	MYSQLND_CSTRING username;
	MYSQLND_CSTRING password;
	MYSQLND_CSTRING database;

};

static const char hexconvtab[] = "0123456789abcdef";


/* {{{ xmysqlnd_node_stmt::handler_on_auth_continue */
static const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_auth_continue)(void * context, const MYSQLND_CSTRING input, MYSQLND_STRING * const output)
{
	const MYSQLND_CSTRING salt = input;
	struct st_xmysqlnd_auth_41_ctx * ctx = (struct st_xmysqlnd_auth_41_ctx *) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_auth_continue");
	DBG_INF_FMT("salt[%d]=%s", salt.l, salt.s);
	if (salt.s) {
		const zend_bool to_hex = ctx->password.s && ctx->password.l;
		char hexed_hash[SCRAMBLE_LENGTH*2];
		if (to_hex) {
			zend_uchar hash[SCRAMBLE_LENGTH];
			unsigned int i;

			php_mysqlnd_scramble(hash, (zend_uchar*) salt.s, (const zend_uchar*) ctx->password.s, ctx->password.l);
			for (i = 0; i < SCRAMBLE_LENGTH; i++) {
				hexed_hash[i*2] = hexconvtab[hash[i] >> 4];
				hexed_hash[i*2 + 1] = hexconvtab[hash[i] & 15];
			}
			DBG_INF_FMT("hexed_hash=%s", hexed_hash);
		}

		{
			//TODO marines
            //char answer[ctx->database.l + 1 + ctx->username.l + 1 + 1 + (to_hex ? SCRAMBLE_LENGTH * 2 : 0) + 1];
            char* answer = static_cast<char*>(mnd_emalloc((ctx->database.l + 1 + ctx->username.l + 1 + 1 + (to_hex ? SCRAMBLE_LENGTH * 2 : 0) + 1) * sizeof(char)));
            char *p = answer;
			memcpy(p, ctx->database.s, ctx->database.l);
			p+= ctx->database.l;
			*p++ = '\0';
			memcpy(p, ctx->username.s, ctx->username.l);
			p+= ctx->username.l;
			*p++ = '\0';
			if (to_hex) {
				*p++ = '*';
				memcpy(p, hexed_hash, SCRAMBLE_LENGTH*2);
				p+= SCRAMBLE_LENGTH*2;
			}
			*p++ = '\0';
			output->l = p - answer;
			output->s = static_cast<char*>(mnd_emalloc(output->l));
			memcpy(output->s, answer, output->l);

			xmysqlnd_dump_string_to_log("output", output->s, output->l);
			//TODO marines
			mnd_efree(answer);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_client_id */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_id)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session? session->client_id : 0;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_client_id */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_id)(void * context, const size_t id)
{
	enum_func_status ret = FAIL;
	XMYSQLND_NODE_SESSION_DATA * session = (XMYSQLND_NODE_SESSION_DATA *) context;
	DBG_ENTER("xmysqlnd_node_session_data::set_client_id");
	DBG_INF_FMT("id=" MYSQLND_LLU_SPEC, id);
	if (context) {
		session->client_id = id;
		ret = PASS;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_get_tls_capability */
static zend_bool
xmysqlnd_get_tls_capability(const zval * capabilities, zend_bool * found)
{
	zval * zv = zend_hash_str_find(Z_ARRVAL_P(capabilities), "tls", sizeof("tls") - 1);
	if (!zv || Z_TYPE_P(zv) == IS_UNDEF) {
		*found = FALSE;
		return FALSE;
	}
	*found = TRUE;
	convert_to_boolean(zv);
	return Z_TYPE_P(zv) == IS_TRUE? TRUE:FALSE;
}
/* }}} */


/* {{{ xmysqlnd_is_mysql41_supported */
static zend_bool
xmysqlnd_is_mysql41_supported(const zval * capabilities)
{
	zval * entry;
	const zval * auth_mechs = zend_hash_str_find(Z_ARRVAL_P(capabilities), "authentication.mechanisms", sizeof("authentication.mechanisms") - 1);
	if (!capabilities || Z_TYPE_P(auth_mechs) != IS_ARRAY) {
		return FALSE;
	}
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(auth_mechs), entry) {
		if (!strcasecmp(Z_STRVAL_P(entry), "MYSQL41")) {
			return TRUE;
		}
	} ZEND_HASH_FOREACH_END();
	return FALSE;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::authenticate */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, authenticate)(XMYSQLND_NODE_SESSION_DATA * session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING username,
						const MYSQLND_CSTRING password,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret = FAIL;
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->io, session->stats, session->error_info);
	const struct st_xmysqlnd_on_error_bind on_error = { session->m->handler_on_error, (void*) session };
	const struct st_xmysqlnd_on_client_id_bind on_client_id = { session->m->set_client_id, (void*) session };
	const struct st_xmysqlnd_on_warning_bind on_warning = { NULL, (void*) session };
	const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change = { NULL, (void*) session };
	struct st_xmysqlnd_auth_41_ctx auth_ctx = { session, scheme, username, password, database };
	const struct st_xmysqlnd_on_auth_continue_bind on_auth_continue = { session->m->handler_on_auth_continue, &auth_ctx };

	struct st_xmysqlnd_msg__capabilities_get caps_get;
	DBG_ENTER("xmysqlnd_node_session_data::authenticate");

	caps_get = msg_factory.get__capabilities_get(&msg_factory);

	if (PASS == caps_get.send_request(&caps_get)) {
		zval capabilities;
		ZVAL_NULL(&capabilities);
		caps_get.init_read(&caps_get, on_error);
		ret = caps_get.read_response(&caps_get, &capabilities);
		if (PASS == ret) {
			zend_bool tls_set;
			const zend_bool tls = xmysqlnd_get_tls_capability(&capabilities, &tls_set);
			const zend_bool mysql41_supported = xmysqlnd_is_mysql41_supported(&capabilities);
			DBG_INF_FMT("tls=%d tls_set=%d", tls, tls_set);
			DBG_INF_FMT("4.1 supported=%d", mysql41_supported);
			if (0 && tls) {

			} else if (mysql41_supported) {
				const MYSQLND_CSTRING mech_name = {"MYSQL41", sizeof("MYSQL41") - 1};
				struct st_xmysqlnd_msg__auth_start auth_start_msg = msg_factory.get__auth_start(&msg_factory);

				ret = auth_start_msg.send_request(&auth_start_msg, mech_name, username);
				if (ret == PASS) {
					auth_start_msg.init_read(&auth_start_msg,
									on_auth_continue,
									on_warning,
									on_error,
									on_client_id,
									on_session_var_change);
					ret = auth_start_msg.read_response(&auth_start_msg, NULL);
					if (PASS == ret) {
						DBG_INF("AUTHENTICATED. YAY!");
					}
				}
			}
		}
		zval_dtor(&capabilities);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::connect_handshake */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, connect_handshake)(XMYSQLND_NODE_SESSION_DATA * session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING username,
						const MYSQLND_CSTRING password,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session_data::connect_handshake");

	if (PASS == session->io.vio->data->m.connect(session->io.vio,
									scheme,
									session->persistent,
									session->stats,
									session->error_info) &&
		PASS == session->io.pfc->data->m.reset(session->io.pfc,
									session->stats,
									session->error_info))

	SET_CONNECTION_STATE(&session->state, NODE_SESSION_NON_AUTHENTICATED);
	ret = session->m->authenticate(session, scheme, username,
							password, database, set_capabilities);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::connect */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, connect)(XMYSQLND_NODE_SESSION_DATA * session,
						MYSQLND_CSTRING hostname,
						MYSQLND_CSTRING username,
						MYSQLND_CSTRING password,
						MYSQLND_CSTRING database,
						MYSQLND_CSTRING socket_or_pipe,
						unsigned int port,
						size_t set_capabilities
					)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), connect);
	zend_bool unix_socket = FALSE;
	zend_bool named_pipe = FALSE;
	zend_bool reconnect = FALSE;
	MYSQLND_STRING transport = { NULL, 0 };

	DBG_ENTER("xmysqlnd_node_session_data::connect");
	DBG_INF_FMT("session=%p", session);

	SET_EMPTY_ERROR(session->error_info);

	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%u persistent=%u state=%u",
				hostname.s?hostname.s:"", username.s?username.s:"", database.s?database.s:"", port, (uint) set_capabilities,
				session? session->persistent:0, session? GET_SESSION_STATE(&session->state):-1);

	if (GET_SESSION_STATE(&session->state) > NODE_SESSION_ALLOCED) {
		DBG_INF("Connecting on a connected handle.");

		if (GET_SESSION_STATE(&session->state) < NODE_SESSION_CLOSE_SENT) {
			XMYSQLND_INC_SESSION_STATISTIC(session->stats, XMYSQLND_STAT_CLOSE_IMPLICIT);
			reconnect = TRUE;
			session->m->send_close(session);
		}

		session->m->free_contents(session);
	}

	transport = session->m->get_scheme(session, hostname, &socket_or_pipe, port, &unix_socket, &named_pipe);

	{
		const MYSQLND_CSTRING scheme = { transport.s, transport.l };
		if (FAIL == session->m->connect_handshake(session, scheme, username, password, database, set_capabilities)) {
			goto err;
		}

	}

	{
		SET_CONNECTION_STATE(&session->state, NODE_SESSION_READY);

		session->scheme.s = mnd_pestrndup(transport.s, transport.l, session->persistent);
		session->scheme.l = transport.l;
		if (transport.s) {
			mnd_sprintf_free(transport.s);
			transport.s = NULL;
		}

		if (!session->scheme.s) {
			goto err; /* OOM */
		}

		session->username.l		= username.l;
		session->username.s		= mnd_pestrndup(username.s, session->username.l, session->persistent);
		session->password.l		= password.l;
		session->password.s		= mnd_pestrndup(password.s, session->password.l, session->persistent);
		session->port			= port;
		session->current_db.l	= database.l;
		session->current_db.s	= mnd_pestrndup(database.s, session->current_db.l, session->persistent);

		if (!session->username.s || !session->password.s|| !session->current_db.s) {
			SET_OOM_ERROR(session->error_info);
			goto err; /* OOM */
		}

		if (!unix_socket && !named_pipe) {
			session->hostname.s = mnd_pestrndup(hostname.s, hostname.l, session->persistent);
			if (!session->hostname.s) {
				SET_OOM_ERROR(session->error_info);
				goto err; /* OOM */
			}
			session->hostname.l = hostname.l;
			{
				char *p;
				mnd_sprintf(&p, 0, "%s via TCP/IP", session->hostname);
				if (!p) {
					SET_OOM_ERROR(session->error_info);
					goto err; /* OOM */
				}
				session->server_host_info = mnd_pestrdup(p, session->persistent);
				mnd_sprintf_free(p);
				if (!session->server_host_info) {
					SET_OOM_ERROR(session->error_info);
					goto err; /* OOM */
				}
			}
		} else {
			session->unix_socket.s = mnd_pestrdup(socket_or_pipe.s, session->persistent);
			if (unix_socket) {
				session->server_host_info = mnd_pestrdup("Localhost via UNIX socket", session->persistent);
			} else if (named_pipe) {
				char *p;
				mnd_sprintf(&p, 0, "%s via named pipe", session->unix_socket.s);
				if (!p) {
					SET_OOM_ERROR(session->error_info);
					goto err; /* OOM */
				}
				session->server_host_info =  mnd_pestrdup(p, session->persistent);
				mnd_sprintf_free(p);
				if (!session->server_host_info) {
					SET_OOM_ERROR(session->error_info);
					goto err; /* OOM */
				}
			} else {
				php_error_docref(NULL, E_WARNING, "Impossible. Should be either socket or a pipe. Report a bug!");
			}
			if (!session->unix_socket.s || !session->server_host_info) {
				SET_OOM_ERROR(session->error_info);
				goto err; /* OOM */
			}
			session->unix_socket.l = strlen(session->unix_socket.s);
		}

		SET_EMPTY_ERROR(session->error_info);

		XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(session->stats, XMYSQLND_STAT_CONNECT_SUCCESS, 1, XMYSQLND_STAT_OPENED_CONNECTIONS, 1);
		if (reconnect) {
			XMYSQLND_INC_GLOBAL_STATISTIC(XMYSQLND_STAT_RECONNECT);
		}
		if (session->persistent) {
			XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(session->stats, XMYSQLND_STAT_PCONNECT_SUCCESS, 1, XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS, 1);
		}

		session->m->local_tx_end(session, this_func, PASS);
		DBG_RETURN(PASS);
	}
err:
	if (transport.s) {
		mnd_sprintf_free(transport.s);
		transport.s = NULL;
	}

	DBG_ERR_FMT("[%u] %.128s (trying to connect via %s)", session->error_info->error_no, session->error_info->error, session->scheme.s);
	if (!session->error_info->error_no) {
		SET_CLIENT_ERROR(session->error_info, CR_CONNECTION_ERROR, UNKNOWN_SQLSTATE, session->error_info->error? session->error_info->error:"Unknown error");
		php_error_docref(NULL, E_WARNING, "[%u] %.128s (trying to connect via %s)", session->error_info->error_no, session->error_info->error, session->scheme.s);
	}

	session->m->free_contents(session);
	XMYSQLND_INC_SESSION_STATISTIC(session->stats, XMYSQLND_STAT_CONNECT_FAILURE);

	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::escape_string */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, escape_string)(XMYSQLND_NODE_SESSION_DATA * const session, char * newstr, const char * to_escapestr, const size_t to_escapestr_len)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), escape_string);
	zend_ulong ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session_data::escape_string");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		ret = mysqlnd_cset_escape_slashes(session->charset, newstr, to_escapestr, to_escapestr_len);
		session->m->local_tx_end(session, this_func, PASS);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::quote_name */
static MYSQLND_STRING
XMYSQLND_METHOD(xmysqlnd_node_session_data, quote_name)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING name)
{
	MYSQLND_STRING ret = { NULL, 0 };
	DBG_ENTER("xmysqlnd_node_session_data::quote_name");
	DBG_INF_FMT("name=%s", name.s);
	if (name.s && name.l) {
		unsigned int occurs = 0;
		unsigned int i;
		for (i = 0; i < name.l; ++i) {
			if (name.s[i] == '`') {
				++occurs;
			}
		}
		ret.l = name.l + occurs + 2 /* quotes */;
		ret.s = static_cast<char*>(mnd_emalloc(ret.l + 1));
		ret.s[0] = '`';
		if (occurs) {
			char *p = &ret.s[0];				/* should start at 0 because we pre-increment in the loop */
			for (i = 0; i < name.l; ++i) {
				const char ch = name.s[i];
				*++p = ch;						/* we pre-increment, because we start at 0 (which is `) before the loop */
				if (UNEXPECTED(ch == '`')) {
					*++p = ch;
				}
			}
		} else {
			memcpy(&(ret.s[1]), name.s, name.l);
		}
		ret.s[ret.l - 1] = '`';
		ret.s[ret.l] = '\0';
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_error_no */
static unsigned int
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error_no)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->error_no;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::error */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->error;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::sqlstate */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, sqlstate)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->sqlstate[0] ? session->error_info->sqlstate : MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_host_info */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_server_host_info)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->server_host_info;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_protocol_info */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_protocol_info)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return "X Protocol";
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_charset_name */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_charset_name)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->charset->name;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_server_option */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_server_option)(XMYSQLND_NODE_SESSION_DATA * const session, const enum_xmysqlnd_server_option option, const char * const value)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), set_server_option);
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session_data::set_server_option");
	if (PASS == session->m->local_tx_start(session, this_func)) {
		/* Handle COM_SET_OPTION here */

		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_client_option */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_option)(XMYSQLND_NODE_SESSION_DATA * const session, enum_xmysqlnd_client_option option, const char * const value)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), set_client_option);
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_session_data::set_client_option");
	DBG_INF_FMT("option=%u", option);

	if (PASS != session->m->local_tx_start(session, this_func)) {
		goto end;
	}
	switch (option) {
		case XMYSQLND_OPT_READ_TIMEOUT:
			ret = session->io.vio->data->m.set_client_option(session->io.vio, static_cast<enum_mysqlnd_client_option>(option), value);
			break;
		default:
			ret = FAIL;
	}
	session->m->local_tx_end(session, this_func, ret);
	DBG_RETURN(ret);
end:
	session->m->local_tx_end(session, this_func, FAIL);
	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_contents)(XMYSQLND_NODE_SESSION_DATA * session)
{
	zend_bool pers = session->persistent;

	DBG_ENTER("xmysqlnd_node_session_data::free_contents");

	if (session->io.pfc) {
		session->io.pfc->data->m.free_contents(session->io.pfc);
	}

	if (session->io.vio) {
		session->io.vio->data->m.free_contents(session->io.vio);
	}

	DBG_INF("Freeing memory of members");

	if (session->hostname.s) {
		mnd_pefree(session->hostname.s, pers);
		session->hostname.s = NULL;
	}
	if (session->username.s) {
		mnd_pefree(session->username.s, pers);
		session->username.s = NULL;
	}
	if (session->password.s) {
		mnd_pefree(session->password.s, pers);
		session->password.s = NULL;
	}
	if (session->current_db.s) {
		mnd_pefree(session->current_db.s, pers);
		session->current_db.s = NULL;
	}
	if (session->unix_socket.s) {
		mnd_pefree(session->unix_socket.s, pers);
		session->unix_socket.s = NULL;
	}
	DBG_INF_FMT("scheme=%s", session->scheme.s);
	if (session->scheme.s) {
		mnd_pefree(session->scheme.s, pers);
		session->scheme.s = NULL;
	}
	if (session->server_host_info) {
		mnd_pefree(session->server_host_info, pers);
		session->server_host_info = NULL;
	}
	if (session->error_info->error_list) {
		zend_llist_clean(session->error_info->error_list);
		mnd_pefree(session->error_info->error_list, pers);
		session->error_info->error_list = NULL;
	}
	session->charset = NULL;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_session_data, dtor)(XMYSQLND_NODE_SESSION_DATA * session)
{
	DBG_ENTER("xmysqlnd_node_session_data::dtor");

	session->m->free_contents(session);
	session->m->free_options(session);

	if (session->io.pfc) {
		xmysqlnd_pfc_free(session->io.pfc, session->stats, session->error_info);
		session->io.pfc = NULL;
	}

	if (session->io.vio) {
		mysqlnd_vio_free(session->io.vio, session->stats, session->error_info);
		session->io.vio = NULL;
	}

	if (session->stats && session->own_stats) {
		mysqlnd_stats_end(session->stats, session->persistent);
	}

	mnd_pefree(session, session->persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_options */
static void
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_options)(XMYSQLND_NODE_SESSION_DATA * session)
{
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_reference */
static XMYSQLND_NODE_SESSION_DATA *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_reference)(XMYSQLND_NODE_SESSION_DATA * const session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_reference");
	++session->refcount;
	DBG_INF_FMT("session=%p new_refcount=%u", session, session->refcount);
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_reference)(XMYSQLND_NODE_SESSION_DATA * const session)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_session_data::free_reference");
	DBG_INF_FMT("session=%p old_refcount=%u", session, session->refcount);
	if (!(--session->refcount)) {
		/*
		  No multithreading issues as we don't share the connection :)
		  This will free the object too, of course because references has
		  reached zero.
		*/
		ret = session->m->send_close(session);
		session->m->dtor(session);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_send_close */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, send_close)(XMYSQLND_NODE_SESSION_DATA * const session)
{
	enum_func_status ret = PASS;
	MYSQLND_VIO * vio = session->io.vio;
	php_stream * net_stream = vio->data->m.get_stream(vio);
	const enum xmysqlnd_node_session_state state = GET_SESSION_STATE(&session->state);

	DBG_ENTER("mysqlnd_send_close");
	DBG_INF_FMT("session=%p vio->data->stream->abstract=%p", session, net_stream? net_stream->abstract:NULL);
	DBG_INF_FMT("state=%u", state);

	if (state >= NODE_SESSION_NON_AUTHENTICATED) {
		XMYSQLND_DEC_GLOBAL_STATISTIC(XMYSQLND_STAT_OPENED_CONNECTIONS);
		if (session->persistent) {
			XMYSQLND_DEC_GLOBAL_STATISTIC(XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS);
		}
	}
	switch (state) {
		case NODE_SESSION_NON_AUTHENTICATED:
		case NODE_SESSION_READY: {
			const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->io, session->stats, session->error_info);
			struct st_xmysqlnd_msg__connection_close conn_close_msg = msg_factory.get__connection_close(&msg_factory);
			DBG_INF("Connection clean, sending CON_CLOSE");
			conn_close_msg.send_request(&conn_close_msg);
			conn_close_msg.read_response(&conn_close_msg);

			if (net_stream) {
				/* HANDLE COM_QUIT here */
				vio->data->m.close_stream(vio, session->stats, session->error_info);
			}
			SET_SESSION_STATE(&session->state, NODE_SESSION_CLOSE_SENT);
			break;
		}
		case NODE_SESSION_ALLOCED:
			/*
			  Allocated but not connected or there was failure when trying
			  to connect with pre-allocated connect.

			  Fall-through
			*/
			SET_SESSION_STATE(&session->state, NODE_SESSION_CLOSE_SENT);
			/* Fall-through */
		case NODE_SESSION_CLOSE_SENT:
			/* The user has killed its own connection */
			vio->data->m.close_stream(vio, session->stats, session->error_info);
			break;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::ssl_set */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, ssl_set)(XMYSQLND_NODE_SESSION_DATA * const session,
													const char * const key,
													const char * const cert,
													const char * const ca,
													const char * const capath,
													const char * const cipher)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), ssl_set);
	enum_func_status ret = FAIL;
	MYSQLND_VIO * vio = session->io.vio;
	DBG_ENTER("xmysqlnd_node_session_data::ssl_set");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		ret = (PASS == vio->data->m.set_client_option(vio, MYSQLND_OPT_SSL_KEY, key) &&
			PASS == vio->data->m.set_client_option(vio, MYSQLND_OPT_SSL_CERT, cert) &&
			PASS == vio->data->m.set_client_option(vio, MYSQLND_OPT_SSL_CA, ca) &&
			PASS == vio->data->m.set_client_option(vio, MYSQLND_OPT_SSL_CAPATH, capath) &&
			PASS == vio->data->m.set_client_option(vio, MYSQLND_OPT_SSL_CIPHER, cipher)) ? PASS : FAIL;

		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::local_tx_start */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_start)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_start");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::local_tx_end */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_end)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func, const enum_func_status status)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_end");
	DBG_RETURN(status);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::negotiate_client_api_capabilities */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, negotiate_client_api_capabilities)(XMYSQLND_NODE_SESSION_DATA * const session, const size_t flags)
{
	size_t ret = 0;
	DBG_ENTER("xmysqlnd_node_session_data::negotiate_client_api_capabilities");
	if (session) {
		ret = session->client_api_capabilities;
		session->client_api_capabilities = flags;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_client_api_capabilities */
static size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_api_capabilities)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_client_api_capabilities");
	DBG_RETURN(session? session->client_api_capabilities : 0);
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session_data)
	XMYSQLND_METHOD(xmysqlnd_node_session_data, init),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_scheme),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, connect_handshake),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, authenticate),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, connect),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, escape_string),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, quote_name),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error_no),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, sqlstate),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_server_host_info),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_protocol_info),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_charset_name),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, set_server_option),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_option),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, free_options),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, dtor),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, free_reference),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, send_close),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, ssl_set),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_start),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_end),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, negotiate_client_api_capabilities),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_api_capabilities),

	XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_error),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_auth_continue),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_id),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_id),
MYSQLND_CLASS_METHODS_END;



PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_session_data);

/* {{{ xmysqlnd_node_session::init */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, init)(XMYSQLND_NODE_SESSION * session_handle,
											const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
											MYSQLND_STATS * stats,
											MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_SESSION_DATA * session_data;
	DBG_ENTER("xmysqlnd_node_session::init");

	session_data = factory->get_node_session_data(factory, session_handle->persistent, stats, error_info);
	if (session_data) {
		session_handle->data = session_data;
	}
	DBG_RETURN(session_data? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_session::connect */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, connect)(XMYSQLND_NODE_SESSION * session_handle,
												MYSQLND_CSTRING hostname,
												MYSQLND_CSTRING username,
												MYSQLND_CSTRING password,
												MYSQLND_CSTRING database,
												MYSQLND_CSTRING socket_or_pipe,
												const unsigned int port,
												const size_t set_capabilities)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), connect);
	enum_func_status ret = FAIL;
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;

	DBG_ENTER("xmysqlnd_node_session::connect");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		if (!hostname.s || !hostname.s[0]) {
			hostname.s = "localhost";
			hostname.l = strlen(hostname.s);
		}
		if (!username.s) {
			DBG_INF_FMT("no user given, using empty string");
			username.s = "";
			username.l = 0;
		}
		if (!password.s) {
			DBG_INF_FMT("no password given, using empty string");
			password.s = "";
			password.l = 0;
		}
		if (!database.s) {
			DBG_INF_FMT("no database given, using empty string");
			database.s = "";
			database.l = 0;
		}

		ret = session->m->connect(session, hostname, username, password, database, socket_or_pipe, port, set_capabilities);
#ifdef WANTED_TO_PRECACHE_UUIDS_AT_CONNECT
		if (PASS == ret) {
			ret = session_handle->m->precache_uuids(session_handle);
		}
#endif
		session->m->local_tx_end(session, this_func, FAIL);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::get_uuid */
static const MYSQLND_CSTRING
XMYSQLND_METHOD(xmysqlnd_node_session, get_uuid)(XMYSQLND_NODE_SESSION * const session_handle)
{
	st_xmysqlnd_node_session::st_xmysqlnd_node_session_uuid_cache_list* cache = &session_handle->uuid_cache;
	MYSQLND_CSTRING ret = { NULL, 0 };
	DBG_ENTER("xmysqlnd_node_session::get_uuid");
	DBG_INF_FMT("pool=%p  used=%u  allocated=%u", cache->pool, cache->used, cache->allocated);
	if (!cache->pool) {
		session_handle->m->precache_uuids(session_handle);
	}
	/* !! NO else here !! */
	if (cache->pool) {
		ret.l = XMYSQLND_UUID_LENGTH;
		if (cache->used == cache->allocated && cache->allocated) {
			session_handle->m->precache_uuids(session_handle);
		}
		/* !! NO else here !! */
		if (cache->used < cache->allocated) {
			ret.s = cache->pool + cache->used * XMYSQLND_UUID_LENGTH;
			++cache->used;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_precache_uuids_on_row */
static const enum_hnd_func_status
xmysqlnd_node_session_precache_uuids_on_row(void * context,
											XMYSQLND_NODE_SESSION * const session,
											XMYSQLND_NODE_STMT * const stmt,
											const XMYSQLND_NODE_STMT_RESULT_META * const meta,
											const zval * const row,
											MYSQLND_STATS * const stats,
											MYSQLND_ERROR_INFO * const error_info)
{
	st_xmysqlnd_node_session::st_xmysqlnd_node_session_uuid_cache_list* ctx = static_cast<st_xmysqlnd_node_session::st_xmysqlnd_node_session_uuid_cache_list*>(context);
	DBG_ENTER("xmysqlnd_node_session_precache_uuids_on_row");
	if (!ctx->pool) {
		ctx->persistent = session->persistent;
		ctx->pool = static_cast<char*>(mnd_pemalloc(Z_STRLEN(row[0]), ctx->persistent));
	}
	/* !! NO else here !! */
	if (ctx->pool) {
		memcpy(ctx->pool, Z_STRVAL(row[0]), Z_STRLEN(row[0]));
		ctx->used = 0;
		ctx->allocated = Z_STRLEN(row[0]) / XMYSQLND_UUID_LENGTH;
	}

	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_session_precache_uuids_on_error */
static const enum_hnd_func_status
xmysqlnd_node_session_precache_uuids_on_error(void * context,
											  XMYSQLND_NODE_SESSION * const session,
											  XMYSQLND_NODE_STMT * const stmt,
											  const unsigned int code,
											  const MYSQLND_CSTRING sql_state,
											  const MYSQLND_CSTRING message)
{
	DBG_ENTER("xmysqlnd_node_session_precache_uuids_on_error");
	if (session) {
		session->data->m->handler_on_error(session->data, code, sql_state, message);
	}
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


#define PRECACHE_SQL_PREFIX "SELECT REPLACE(CONCAT(UUID()"
#define PRECACHE_SQL_REPEAT ", UUID()"
#define PRECACHE_SQL_SUFFIX "), '-', '')"
static const unsigned int precache_size = XMYSQLND_UUID_CACHE_ELEMENTS;
static const size_t query_prefix_len = sizeof(PRECACHE_SQL_PREFIX) - 1;
static const size_t query_repeat_len = sizeof(PRECACHE_SQL_REPEAT) - 1;
static const size_t query_suffix_len = sizeof(PRECACHE_SQL_SUFFIX) - 1;


/* {{{ xmysqlnd_node_session::precache_uuids() */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, precache_uuids)(XMYSQLND_NODE_SESSION * const session_handle)
{
	const struct st_xmysqlnd_node_session_on_row_bind on_row = { xmysqlnd_node_session_precache_uuids_on_row, &session_handle->uuid_cache };
	const struct st_xmysqlnd_node_session_on_error_bind on_error = { xmysqlnd_node_session_precache_uuids_on_error, NULL };
	const size_t query_len = query_prefix_len + (precache_size - 1) * query_repeat_len + query_suffix_len;
	const MYSQLND_STRING list_query = { static_cast<char*>(mnd_emalloc(query_len)), query_len };
	enum_func_status ret;
	unsigned int i;
	char * start_pos = NULL;
	DBG_ENTER("xmysqlnd_node_session::precache_uuids");

	if (!precache_size || !list_query.s) {
		if (list_query.s) {
			mnd_efree(list_query.s);
		}
		DBG_RETURN(FAIL);
	}

	memcpy(list_query.s, PRECACHE_SQL_PREFIX, query_prefix_len);

	start_pos = list_query.s + query_prefix_len;
	for (i = 0; i < (precache_size - 1); ++i) {
		memcpy(start_pos, PRECACHE_SQL_REPEAT, query_repeat_len);
		start_pos += query_repeat_len;
	}
	memcpy(start_pos, PRECACHE_SQL_SUFFIX, query_suffix_len);

	session_handle->uuid_cache.used = 0;
	session_handle->uuid_cache.allocated = 0;

	ret = session_handle->m->query_cb(session_handle,
									  namespace_sql,
									  mnd_str2c(list_query),
									  noop__var_binder,
									  noop__on_result_start,
									  on_row,
									  noop__on_warning,
									  on_error,
									  noop__on_result_end,
									  noop__on_statement_ok);
	mnd_efree(list_query.s);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_schema_operation */
static enum_func_status
xmysqlnd_schema_operation(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING operation, const MYSQLND_CSTRING db)
{
	enum_func_status ret = FAIL;
	const MYSQLND_STRING quoted_db = session_handle->data->m->quote_name(session_handle->data, db);
	DBG_ENTER("xmysqlnd_schema_operation");
	DBG_INF_FMT("db=%s", db);

	if (quoted_db.s && quoted_db.l) {
		const size_t query_len = operation.l + quoted_db.l;
		//TODO marines
        //char query[query_len + 1];
        char* query = static_cast<char*>(mnd_emalloc((query_len + 1) * sizeof(char)));
        memcpy(query, operation.s, operation.l);
		memcpy(query + operation.l, quoted_db.s, quoted_db.l);
		query[query_len] = '\0';
		const MYSQLND_CSTRING select_query = { query, query_len };
		mnd_efree(quoted_db.s);

		ret = session_handle->m->query(session_handle, namespace_sql, select_query, noop__var_binder);
		//TODO marines
        mnd_efree(query);
	}
	DBG_RETURN(ret);

}
/* }}} */


/* {{{ xmysqlnd_node_session::select_db */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, select_db)(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "USE ", sizeof("USE ") - 1 };
	DBG_ENTER("xmysqlnd_node_session::select_db");
	ret = xmysqlnd_schema_operation(session_handle, operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::create_db */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, create_db)(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "CREATE DATABASE ", sizeof("CREATE DATABASE ") - 1 };
	DBG_ENTER("xmysqlnd_node_session::create_db");
	ret = xmysqlnd_schema_operation(session_handle, operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::drop_db */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, drop_db)(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "DROP DATABASE ", sizeof("DROP DATABASE ") - 1 };
	DBG_ENTER("xmysqlnd_node_session::drop_db");
	ret = xmysqlnd_schema_operation(session_handle, operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ st_xmysqlnd_query_cb_ctx */
struct st_xmysqlnd_query_cb_ctx
{
	XMYSQLND_NODE_SESSION * session;
	struct st_xmysqlnd_node_session_on_result_start_bind handler_on_result_start;
	struct st_xmysqlnd_node_session_on_row_bind handler_on_row;
	struct st_xmysqlnd_node_session_on_warning_bind handler_on_warning;
	struct st_xmysqlnd_node_session_on_error_bind handler_on_error;
	struct st_xmysqlnd_node_session_on_result_end_bind handler_on_result_end;
	struct st_xmysqlnd_node_session_on_statement_ok_bind handler_on_statement_ok;
};
/* }}} */


/* {{{ query_cb_handler_on_result_start */
static const enum_hnd_func_status
query_cb_handler_on_result_start(void * context, XMYSQLND_NODE_STMT * const stmt)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_result_start");
	if (ctx && ctx->session && ctx->handler_on_result_start.handler) {
		ret = ctx->handler_on_result_start.handler(ctx->handler_on_result_start.ctx, ctx->session, stmt);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_row */
static const enum_hnd_func_status
query_cb_handler_on_row(void * context,
						XMYSQLND_NODE_STMT * const stmt,
						const struct st_xmysqlnd_node_stmt_result_meta * const meta,
						const zval * const row,
						MYSQLND_STATS * const stats,
						MYSQLND_ERROR_INFO * const error_info)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_row");
	if (ctx && ctx->session && ctx->handler_on_row.handler && row) {
		ret = ctx->handler_on_row.handler(ctx->handler_on_row.ctx, ctx->session, stmt, meta, row, stats, error_info);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_warning */
static const enum_hnd_func_status
query_cb_handler_on_warning(void * context,
							XMYSQLND_NODE_STMT * const stmt,
							const enum xmysqlnd_stmt_warning_level level,
							const unsigned int code,
							const MYSQLND_CSTRING message)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_warning");
	if (ctx && ctx->session && ctx->handler_on_warning.handler) {
		ret = ctx->handler_on_warning.handler(ctx->handler_on_warning.ctx, ctx->session, stmt, level, code, message);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_error */
static const enum_hnd_func_status
query_cb_handler_on_error(void * context,
						  XMYSQLND_NODE_STMT * const stmt,
						  const unsigned int code,
						  const MYSQLND_CSTRING sql_state,
						  const MYSQLND_CSTRING message)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_error");
	if (ctx && ctx->session && ctx->handler_on_error.handler) {
		ret = ctx->handler_on_error.handler(ctx->handler_on_error.ctx, ctx->session, stmt, code, sql_state, message);
	}
	ret = HND_PASS_RETURN_FAIL;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_result_end */
static const enum_hnd_func_status
query_cb_handler_on_result_end(void * context, XMYSQLND_NODE_STMT * const stmt, const zend_bool has_more)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_result_end");
	if (ctx && ctx->session && ctx->handler_on_result_end.handler) {
		ret = ctx->handler_on_result_end.handler(ctx->handler_on_result_end.ctx, ctx->session, stmt, has_more);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_statement_ok */
static const enum_hnd_func_status
query_cb_handler_on_statement_ok(void * context, XMYSQLND_NODE_STMT * const stmt, const struct st_xmysqlnd_stmt_execution_state * const exec_state)
{
	enum_hnd_func_status ret;
	const struct st_xmysqlnd_query_cb_ctx * ctx = (const struct st_xmysqlnd_query_cb_ctx *) context;
	DBG_ENTER("query_cb_handler_on_result_end");
	if (ctx && ctx->session && ctx->handler_on_statement_ok.handler) {
		ret = ctx->handler_on_statement_ok.handler(ctx->handler_on_statement_ok.ctx, ctx->session, stmt, exec_state);
	}
	ret = HND_PASS;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query_cb */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, query_cb)(XMYSQLND_NODE_SESSION * session_handle,
												 const MYSQLND_CSTRING namespace_,
												 const MYSQLND_CSTRING query,
												 const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder,
												 const struct st_xmysqlnd_node_session_on_result_start_bind handler_on_result_start,
												 const struct st_xmysqlnd_node_session_on_row_bind handler_on_row,
												 const struct st_xmysqlnd_node_session_on_warning_bind handler_on_warning,
												 const struct st_xmysqlnd_node_session_on_error_bind handler_on_error,
												 const struct st_xmysqlnd_node_session_on_result_end_bind handler_on_result_end,
												 const struct st_xmysqlnd_node_session_on_statement_ok_bind handler_on_statement_ok)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session::query_cb");
	if (session_handle) {
		XMYSQLND_NODE_SESSION_DATA * const session = session_handle->data;
		XMYSQLND_NODE_STMT * const stmt = session_handle->m->create_statement_object(session_handle);
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
		if (stmt && stmt_execute) {
			ret = PASS;
			if (var_binder.handler) {
				zend_bool loop = TRUE;
				do {
					const enum_hnd_func_status var_binder_result = var_binder.handler(var_binder.ctx, session_handle, stmt_execute);
					switch (var_binder_result) {
						case HND_FAIL:
						case HND_PASS_RETURN_FAIL:
							ret = FAIL;
							/* fallthrough */
						case HND_PASS:
							loop = FALSE;
							break;
						case HND_AGAIN: /* do nothing */
						default:
							break;
					}
				} while (loop);
			}
			ret = xmysqlnd_stmt_execute__finalize_bind(stmt_execute);
			if (PASS == ret &&
				(PASS == (ret = stmt->data->m.send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), session->stats, session->error_info))))
			{
				struct st_xmysqlnd_query_cb_ctx query_cb_ctx = {
					session_handle,
					handler_on_result_start,
					handler_on_row,
					handler_on_warning,
					handler_on_error,
					handler_on_result_end,
					handler_on_statement_ok
				};
				const struct st_xmysqlnd_node_stmt_on_row_bind on_row = {
					handler_on_row.handler? query_cb_handler_on_row : NULL,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = {
					handler_on_warning.handler? query_cb_handler_on_warning : NULL,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_error_bind on_error = {
					handler_on_error.handler? query_cb_handler_on_error : NULL,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_result_start_bind on_result_start = {
					handler_on_result_start.handler? query_cb_handler_on_result_start : NULL,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_result_end_bind on_result_end = {
					handler_on_result_end.handler? query_cb_handler_on_result_end : NULL,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok = {
					handler_on_statement_ok.handler? query_cb_handler_on_statement_ok : NULL,
					&query_cb_ctx
				};
				ret = stmt->data->m.read_all_results(stmt, on_row, on_warning, on_error, on_result_start, on_result_end, on_statement_ok,
													 session->stats, session->error_info);
			}
		}
		/* no else, please */
		if (stmt) {
			xmysqlnd_node_stmt_free(stmt, session->stats, session->error_info);
		}
		if (stmt_execute) {
			xmysqlnd_stmt_execute__destroy(stmt_execute);
		}
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query_cb_ex */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, query_cb_ex)(XMYSQLND_NODE_SESSION * session_handle,
													const MYSQLND_CSTRING namespace_,
													struct st_xmysqlnd_query_builder * query_builder,
													const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder,
													const struct st_xmysqlnd_node_session_on_result_start_bind handler_on_result_start,
													const struct st_xmysqlnd_node_session_on_row_bind handler_on_row,
													const struct st_xmysqlnd_node_session_on_warning_bind handler_on_warning,
													const struct st_xmysqlnd_node_session_on_error_bind handler_on_error,
													const struct st_xmysqlnd_node_session_on_result_end_bind handler_on_result_end,
													const struct st_xmysqlnd_node_session_on_statement_ok_bind handler_on_statement_ok)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session::query_cb_ex");
	if (query_builder &&
		query_builder->create &&
		session_handle &&
		PASS == (ret = query_builder->create(query_builder)))
	{
		ret = session_handle->m->query_cb(session_handle,
										  namespace_,
										  mnd_str2c(query_builder->query),
										  var_binder,
										  handler_on_result_start,
										  handler_on_row,
										  handler_on_warning,
										  handler_on_error,
										  handler_on_result_end,
										  handler_on_statement_ok);
		query_builder->destroy(query_builder);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_on_warning */
static const enum_hnd_func_status
xmysqlnd_node_session_on_warning(void * context, XMYSQLND_NODE_STMT * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message)
{
	DBG_ENTER("xmysqlnd_node_session_on_warning");
	//php_error_docref(NULL, E_WARNING, "[%d] %*s", code, message.l, message.s);
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, query)(XMYSQLND_NODE_SESSION * session_handle,
											  const MYSQLND_CSTRING namespace_,
											  const MYSQLND_CSTRING query,
											  const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), query);
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	enum_func_status ret = FAIL;

	DBG_ENTER("xmysqlnd_node_session::query");
	if (PASS == session->m->local_tx_start(session, this_func)) {
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
		XMYSQLND_NODE_STMT * stmt = session_handle->m->create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			ret = PASS;
			if (var_binder.handler) {
				zend_bool loop = TRUE;
				do {
					const enum_hnd_func_status var_binder_result = var_binder.handler(var_binder.ctx, session_handle, stmt_execute);
					switch (var_binder_result) {
						case HND_FAIL:
						case HND_PASS_RETURN_FAIL:
							ret = FAIL;
							/* fallthrough */
						case HND_PASS:
							loop = FALSE;
							break;
						case HND_AGAIN: /* do nothing */
						default:
							break;
					}
				} while (loop);
			}

			if (PASS == ret &&
				(PASS == (ret = stmt->data->m.send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), session->stats, session->error_info))))
			{
				do {
					const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = { xmysqlnd_node_session_on_warning, NULL };
					const struct st_xmysqlnd_node_stmt_on_error_bind on_error = { NULL, NULL };
					zend_bool has_more = FALSE;
					XMYSQLND_NODE_STMT_RESULT * result = stmt->data->m.get_buffered_result(stmt, &has_more, on_warning, on_error, session->stats, session->error_info);
					if (result) {
						ret = PASS;
						xmysqlnd_node_stmt_result_free(result, session->stats, session->error_info);
					} else {
						ret = FAIL;
					}
				} while (stmt->data->m.has_more_results(stmt) == TRUE);
			}
		}
		/* no else, please */
		if (stmt) {
			xmysqlnd_node_stmt_free(stmt, session->stats, session->error_info);
		}
		if (stmt_execute) {
			xmysqlnd_stmt_execute__destroy(stmt_execute);
		}

		/* If we do it after free_reference/dtor then we might crash */
		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::get_server_version */
static zend_ulong
XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version)(XMYSQLND_NODE_SESSION * const session_handle)
{
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	zend_long major, minor, patch;
	char *p;
	DBG_ENTER("xmysqlnd_node_session::get_server_version");
	if (!(p = session_handle->server_version_string)) {
		const MYSQLND_CSTRING query = { "SELECT VERSION()", sizeof("SELECT VERSION()") - 1 };
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_sql, query);
		XMYSQLND_NODE_STMT * stmt = session_handle->m->create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			if (PASS == stmt->data->m.send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), session->stats, session->error_info)) {
				const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = { NULL, NULL };
				const struct st_xmysqlnd_node_stmt_on_error_bind on_error = { NULL, NULL };
				zend_bool has_more = FALSE;
				XMYSQLND_NODE_STMT_RESULT * res = stmt->data->m.get_buffered_result(stmt, &has_more, on_warning, on_error, session->stats, session->error_info);
				if (res) {
					zval * set = NULL;
					if (PASS == res->m.fetch_all_c(res, &set, FALSE /* don't duplicate, reference it */, session->stats, session->error_info) &&
						Z_TYPE(set[0 * 0]) == IS_STRING)
					{
						DBG_INF_FMT("Found %*s", Z_STRLEN(set[0 * 0]), Z_STRVAL(set[0 * 0]));
						session_handle->server_version_string = mnd_pestrndup(Z_STRVAL(set[0 * 0]), Z_STRLEN(set[0 * 0]), session_handle->persistent);
					}
					if (set) {
						mnd_efree(set);
					}
				}
				xmysqlnd_node_stmt_result_free(res, session->stats, session->error_info);
			}
		}
		/* no else, please */
		if (stmt) {
			xmysqlnd_node_stmt_free(stmt, session->stats, session->error_info);
		}
		if (stmt_execute) {
			xmysqlnd_stmt_execute__destroy(stmt_execute);
		}
	} else {
		DBG_INF_FMT("server_version_string=%s", session_handle->server_version_string);
	}
	if (!(p = session_handle->server_version_string)) {
		return 0;
	}
	major = ZEND_STRTOL(p, &p, 10);
	p += 1; /* consume the dot */
	minor = ZEND_STRTOL(p, &p, 10);
	p += 1; /* consume the dot */
	patch = ZEND_STRTOL(p, &p, 10);

	DBG_RETURN( (zend_ulong)(major * Z_L(10000) + (zend_ulong)(minor * Z_L(100) + patch)) );
}
/* }}} */


/* {{{ xmysqlnd_node_session::get_server_info */
static const char *
XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version_string)(const XMYSQLND_NODE_SESSION * const session_handle)
{
	return session_handle->server_version_string;
}
/* }}} */


/* {{{ xmysqlnd_node_session::create_statement_object */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_session, create_statement_object)(XMYSQLND_NODE_SESSION * const session_handle)
{
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_session_data::create_statement_object");

	stmt = xmysqlnd_node_stmt_create(session_handle, session_handle->persistent, session_handle->data->object_factory, session_handle->data->stats, session_handle->data->error_info);
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_session::create_schema_object */
static XMYSQLND_NODE_SCHEMA *
XMYSQLND_METHOD(xmysqlnd_node_session, create_schema_object)(XMYSQLND_NODE_SESSION * const session_handle, const MYSQLND_CSTRING schema_name)
{
	XMYSQLND_NODE_SCHEMA * schema = NULL;
	DBG_ENTER("xmysqlnd_node_session::create_schema_object");
	DBG_INF_FMT("schema_name=%s", schema_name.s);

	schema = xmysqlnd_node_schema_create(session_handle, schema_name, session_handle->persistent, session_handle->data->object_factory, session_handle->data->stats, session_handle->data->error_info);

	DBG_RETURN(schema);
}
/* }}} */



struct st_create_collection_handler_ctx
{
	XMYSQLND_NODE_SESSION * session;
	const struct st_xmysqlnd_node_session_on_error_bind on_error;
};

/* {{{ collection_op_handler_on_error */
static const enum_hnd_func_status
collection_op_handler_on_error(void * context,
							   XMYSQLND_NODE_SESSION * const session,
							   XMYSQLND_NODE_STMT * const stmt,
							   const unsigned int code,
							   const MYSQLND_CSTRING sql_state,
							   const MYSQLND_CSTRING message)
{
	struct st_create_collection_handler_ctx * ctx = (struct st_create_collection_handler_ctx *) context;
	DBG_ENTER("collection_op_handler_on_error");
	ctx->on_error.handler(ctx->on_error.ctx, ctx->session, stmt, code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ collection_op_var_binder */
static const enum_hnd_func_status
collection_op_var_binder(void * context, XMYSQLND_NODE_SESSION * session, XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret = HND_FAIL;
	struct st_collection_op_var_binder_ctx * ctx = (struct st_collection_op_var_binder_ctx *) context;
	const MYSQLND_CSTRING * param = NULL;
	DBG_ENTER("collection_op_var_binder");
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			goto bind;
		case 1:{
			param = &ctx->collection_name;
			ret = HND_PASS;
bind:
			{
				enum_func_status result;
				zval zv;
				ZVAL_UNDEF(&zv);
				ZVAL_STRINGL(&zv, param->s, param->l);
				DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);
				result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, ctx->counter, &zv);
//				result = stmt->data->m.bind_one_stmt_param(stmt, ctx->counter, &zv);

				zval_ptr_dtor(&zv);
				if (FAIL == result) {
					ret = HND_FAIL;
				}
			}
			break;
		}
		default: /* should not happen */
			break;
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_collection_op */
static const enum_func_status
xmysqlnd_collection_op(
	XMYSQLND_NODE_SESSION * const session,
	const MYSQLND_CSTRING schema_name,
	const MYSQLND_CSTRING collection_name,
	const MYSQLND_CSTRING query,
	const struct st_xmysqlnd_node_session_on_error_bind handler_on_error)
{
	enum_func_status ret;

	struct st_collection_op_var_binder_ctx var_binder_ctx = {
		schema_name,
		collection_name,
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	struct st_create_collection_handler_ctx handler_ctx = { session, handler_on_error };
	const struct st_xmysqlnd_node_session_on_error_bind on_error = { handler_on_error.handler? collection_op_handler_on_error : NULL, &handler_ctx };

	DBG_ENTER("xmysqlnd_collection_op");

	ret = session->m->query_cb(session,
							   namespace_xplugin,
							   query,
							   var_binder,
							   noop__on_result_start,
							   noop__on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::drop_collection */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, drop_collection)(
	XMYSQLND_NODE_SESSION * const session,
	const MYSQLND_CSTRING schema_name,
	const MYSQLND_CSTRING collection_name,
	const struct st_xmysqlnd_node_session_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_node_schema::drop_collection");
	DBG_INF_FMT("schema_name=%s", collection_name.s);

	ret = xmysqlnd_collection_op(session, schema_name, collection_name, query, handler_on_error);

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_node_session::drop_table */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, drop_table)(
	XMYSQLND_NODE_SESSION * const session,
	const MYSQLND_CSTRING schema_name,
	const MYSQLND_CSTRING table_name,
	const struct st_xmysqlnd_node_session_on_error_bind handler_on_error)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"drop_collection", sizeof("drop_collection") - 1 };
	DBG_ENTER("xmysqlnd_node_session::drop_table");
	DBG_INF_FMT("schema_name=%s", table_name.s);

	ret = xmysqlnd_collection_op(session, schema_name, table_name, query, handler_on_error);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::close */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, close)(XMYSQLND_NODE_SESSION * session_handle, const enum_xmysqlnd_node_session_close_type close_type)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), close);
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	enum_func_status ret = FAIL;

	DBG_ENTER("xmysqlnd_node_session::close");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		if (GET_SESSION_STATE(&session->state) >= NODE_SESSION_READY) {
			static enum_xmysqlnd_collected_stats close_type_to_stat_map[XMYSQLND_CLOSE_LAST] = {
				XMYSQLND_STAT_CLOSE_EXPLICIT,
				XMYSQLND_STAT_CLOSE_IMPLICIT,
				XMYSQLND_STAT_CLOSE_DISCONNECT
			};
			XMYSQLND_INC_SESSION_STATISTIC(session->stats, close_type_to_stat_map[close_type]);
		}

		/*
		  Close now, free_reference will try,
		  if we are last, but that's not a problem.
		*/
		ret = session->m->send_close(session);

		/* If we do it after free_reference/dtor then we might crash */
		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_node_session::get_reference */
static XMYSQLND_NODE_SESSION *
XMYSQLND_METHOD(xmysqlnd_node_session, get_reference)(XMYSQLND_NODE_SESSION * const session)
{
	DBG_ENTER("xmysqlnd_node_session::get_reference");
	++session->refcount;
	DBG_INF_FMT("session=%p new_refcount=%u", session, session->refcount);
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session::free_reference */
static const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, free_reference)(XMYSQLND_NODE_SESSION * const session)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_session::free_reference");
	DBG_INF_FMT("session=%p old_refcount=%u", session, session->refcount);
	if (!(--session->refcount)) {
		session->m->dtor(session);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_session, free_contents)(XMYSQLND_NODE_SESSION * session_handle)
{
	zend_bool pers = session_handle->persistent;

	DBG_ENTER("xmysqlnd_node_session::free_contents");

	DBG_INF("Freeing memory of members");

	if (session_handle->server_version_string) {
		mnd_pefree(session_handle->server_version_string, pers);
		session_handle->server_version_string = NULL;
	}
	if (session_handle->uuid_cache.pool) {
		session_handle->uuid_cache.used = 0;
		session_handle->uuid_cache.allocated = 0;
		mnd_pefree(session_handle->uuid_cache.pool, session_handle->uuid_cache.persistent);
		session_handle->uuid_cache.pool = NULL;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_session, dtor)(XMYSQLND_NODE_SESSION * session_handle)
{
	DBG_ENTER("xmysqlnd_node_session::dtor");
	session_handle->m->free_contents(session_handle);
	if (session_handle->data) {
		session_handle->data->m->free_reference(session_handle->data);
		session_handle->data = NULL;
	}
	mnd_pefree(session_handle, session_handle->persistent);
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session)
	XMYSQLND_METHOD(xmysqlnd_node_session, init),
	XMYSQLND_METHOD(xmysqlnd_node_session, connect),
	XMYSQLND_METHOD(xmysqlnd_node_session, get_uuid),
	XMYSQLND_METHOD(xmysqlnd_node_session, precache_uuids),
	XMYSQLND_METHOD(xmysqlnd_node_session, create_db),
	XMYSQLND_METHOD(xmysqlnd_node_session, select_db),
	XMYSQLND_METHOD(xmysqlnd_node_session, drop_db),
	XMYSQLND_METHOD(xmysqlnd_node_session, query),
	XMYSQLND_METHOD(xmysqlnd_node_session, query_cb),
	XMYSQLND_METHOD(xmysqlnd_node_session, query_cb_ex),
	XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version),
	XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version_string),
	XMYSQLND_METHOD(xmysqlnd_node_session, create_statement_object),
	XMYSQLND_METHOD(xmysqlnd_node_session, create_schema_object),
	XMYSQLND_METHOD(xmysqlnd_node_session, drop_collection),
	XMYSQLND_METHOD(xmysqlnd_node_session, drop_table),
	XMYSQLND_METHOD(xmysqlnd_node_session, close),
	XMYSQLND_METHOD(xmysqlnd_node_session, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_session, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_session, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_session, dtor),
MYSQLND_CLASS_METHODS_END;



PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_session);

/* {{{ xmysqlnd_node_session_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_SESSION *
xmysqlnd_node_session_create(const size_t client_flags, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("xmysqlnd_node_session_create");
	session = object_factory->get_node_session(object_factory, persistent, stats, error_info);
	if (session && session->data) {
		session->data->m->negotiate_client_api_capabilities(session->data, client_flags);
		session->m->get_reference(session);
	}
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_connect */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_SESSION *
xmysqlnd_node_session_connect(XMYSQLND_NODE_SESSION * session,
							  const MYSQLND_CSTRING hostname,
							  const MYSQLND_CSTRING username,
							  const MYSQLND_CSTRING password,
							  const MYSQLND_CSTRING database,
							  const MYSQLND_CSTRING socket_or_pipe,
							  unsigned int port,
							  const size_t set_capabilities,
							  const size_t client_api_flags)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory =
			MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	enum_func_status ret = FAIL;
	zend_bool self_allocated = FALSE;
	/* may need to pass these from outside */
	MYSQLND_STATS * stats = NULL;
	MYSQLND_ERROR_INFO * error_info = NULL;

	DBG_ENTER("xmysqlnd_node_session_connect");
	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%llu",
			hostname.s, username.s, database.s,
			port, (unsigned long long) set_capabilities);

	if (!session) {
		self_allocated = TRUE;
		if (!(session = xmysqlnd_node_session_create(client_api_flags,
											FALSE, factory,
											stats, error_info))) {
			/* OOM */
			DBG_RETURN(NULL);
		}
	}

	ret = session->m->connect(session, hostname,
						username, password,
						database, socket_or_pipe,
						port, set_capabilities);

	if (ret == FAIL) {
		if (self_allocated) {
			/*
			  We have allocated, thus there are no references to this
			  object - we are free to kill it!
			*/
			session->m->dtor(session);
		}
		DBG_RETURN(NULL);
	}
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_session_free(XMYSQLND_NODE_SESSION* const session)
{
	DBG_ENTER("xmysqlnd_node_session_free");
	DBG_INF_FMT(
		"session=%p  session->data=%p  dtor=%p", 
		session, 
		session ? session->data : nullptr, 
		session ? session->data->m->dtor : nullptr);
	if (session) {
		session->m->free_reference(session);
	}
	DBG_VOID_RETURN;
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
