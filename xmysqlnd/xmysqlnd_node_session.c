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
#include "ext/mysqlnd/mysqlnd_charset.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h"
#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_wireprotocol.h"


/* {{{ xmysqlnd_node_session_state::get */
static enum xmysqlnd_node_session_state
MYSQLND_METHOD(xmysqlnd_node_session_state, get)(const XMYSQLND_NODE_SESSION_STATE * const state_struct)
{
	DBG_ENTER("xmysqlnd_node_session_state::get");
	DBG_INF_FMT("State=%u", state_struct->state);
	DBG_RETURN(state_struct->state);
}
/* }}} */


/* {{{ xmysqlnd_node_session_state::set */
static void
MYSQLND_METHOD(xmysqlnd_node_session_state, set)(XMYSQLND_NODE_SESSION_STATE * const state_struct, const enum xmysqlnd_node_session_state state)
{
	DBG_ENTER("xmysqlnd_node_session_state::set");
	DBG_INF_FMT("New state=%u", state);
	state_struct->state = state;
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session_state)
	MYSQLND_METHOD(xmysqlnd_node_session_state, get),
	MYSQLND_METHOD(xmysqlnd_node_session_state, set),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_session_state_init */
PHPAPI void
xmysqlnd_node_session_state_init(XMYSQLND_NODE_SESSION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_node_session_state_init");
	state->m = &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_session_state);
	state->state = NODE_SESSION_ALLOCED;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_scheme */
static MYSQLND_STRING
MYSQLND_METHOD(xmysqlnd_node_session_data, get_scheme)(XMYSQLND_NODE_SESSION_DATA * session,
													  MYSQLND_CSTRING hostname,
													  MYSQLND_CSTRING socket_or_pipe,
													  unsigned int port,
													  zend_bool * unix_socket,
													  zend_bool * named_pipe)
{
	MYSQLND_STRING transport;
	DBG_ENTER("xmysqlnd_node_session_data::get_scheme");
#ifdef PHP_WIN32
	if (hostname.l == sizeof(".") - 1 && hostname.s[0] == '.') {
		/* named pipe in socket */
		if (!socket_or_pipe.s) {
			socket_or_pipe.s = "\\\\.\\pipe\\MySQL";
			socket_or_pipe.l = strlen(socket_or_pipe.s);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "pipe://%s", socket_or_pipe.s);
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


/* {{{ proto xmysqlnd_get_tls_capability */
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


/* {{{ proto xmysqlnd_is_mysql41_supported */
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
MYSQLND_METHOD(xmysqlnd_node_session_data, authenticate)(XMYSQLND_NODE_SESSION_DATA * session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING username,
						const MYSQLND_CSTRING password,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret = FAIL;
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->io, session->stats, session->error_info);
	struct st_xmysqlnd_capabilities_get_message_ctx caps_get;
	DBG_ENTER("xmysqlnd_node_session_data::authenticate");

	
	caps_get = msg_factory.get__capabilities_get(&msg_factory);

	if (PASS == caps_get.send_request(&caps_get)) {
		zval capabilities;
		ZVAL_NULL(&capabilities);
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
				struct st_xmysqlnd_auth_start_message_ctx auth_start_msg = msg_factory.get__auth_start(&msg_factory);

				ret = auth_start_msg.send_request(&auth_start_msg, mech_name, username);
				if (ret == PASS) {
					ret = auth_start_msg.read_response(&auth_start_msg, NULL);
					if (PASS == ret) {
						if (auth_start_msg.continue_auth(&auth_start_msg) && auth_start_msg.out_auth_data.s) {
							struct st_xmysqlnd_auth_continue_message_ctx auth_cont_msg = msg_factory.get__auth_continue(&msg_factory);
							const MYSQLND_CSTRING salt_par = {auth_start_msg.out_auth_data.s, auth_start_msg.out_auth_data.l};

							ret = auth_cont_msg.send_request(&auth_cont_msg, database, username, password, salt_par);
							if (PASS == ret) {
								ret = auth_cont_msg.read_response(&auth_cont_msg, NULL);
								if (PASS == ret && auth_cont_msg.finished(&auth_cont_msg)) {
									DBG_INF("AUTHENTICATED. YAY!");
								}
							}
						}
					}
				}
				auth_start_msg.free_resources(&auth_start_msg);
			}
		}
		zval_dtor(&capabilities);
	}

	DBG_RETURN(ret);
}
/* }}} */



/* {{{ xmysqlnd_node_session_data::connect_handshake */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session_data, connect_handshake)(XMYSQLND_NODE_SESSION_DATA * session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING username,
						const MYSQLND_CSTRING password,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_session_data::connect_handshake");

	if (PASS == session->io.vio->data->m.connect(session->io.vio, scheme, session->persistent, session->stats, session->error_info) &&
		PASS == session->io.pfc->data->m.reset(session->io.pfc, session->stats, session->error_info))
	{
		SET_CONNECTION_STATE(&session->state, NODE_SESSION_NON_AUTHENTICATED);
		ret = session->m->authenticate(session, scheme, username, password, database, set_capabilities);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::connect */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session_data, connect)(XMYSQLND_NODE_SESSION_DATA * session,
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

	transport = session->m->get_scheme(session, hostname, socket_or_pipe, port, &unix_socket, &named_pipe);

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
MYSQLND_METHOD(xmysqlnd_node_session_data, escape_string)(XMYSQLND_NODE_SESSION_DATA * const session, char * newstr, const char * to_escapestr, const size_t to_escapestr_len)
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


/* {{{ xmysqlnd_node_session_data::create_statement */
static XMYSQLND_NODE_STMT *
MYSQLND_METHOD(xmysqlnd_node_session_data, create_statement)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING query, enum_mysqlnd_send_query_type type)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), create_statement);
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_session_data::create_statement");
	DBG_INF_FMT("query=%s", query.s);

	if (type == MYSQLND_SEND_QUERY_IMPLICIT || PASS == session->m->local_tx_start(session, this_func))
	{
		stmt = xmysqlnd_node_stmt_init(session, query, session->persistent, &session->object_factory, session->stats, session->error_info);

		if (type == MYSQLND_SEND_QUERY_EXPLICIT) {
			session->m->local_tx_end(session, this_func, stmt ? PASS:FAIL);
		}
	}
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_error_no */
static unsigned int
MYSQLND_METHOD(xmysqlnd_node_session_data, get_error_no)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->error_no;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::error */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_error)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->error;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::sqlstate */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, sqlstate)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->error_info->sqlstate[0] ? session->error_info->sqlstate : MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_server_version */
static zend_ulong
MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_version)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	zend_long major, minor, patch;
	char *p;

	if (!(p = session->server_version_string)) {
		return 0;
	}

	major = ZEND_STRTOL(p, &p, 10);
	p += 1; /* consume the dot */
	minor = ZEND_STRTOL(p, &p, 10);
	p += 1; /* consume the dot */
	patch = ZEND_STRTOL(p, &p, 10);

	return (zend_ulong)(major * Z_L(10000) + (zend_ulong)(minor * Z_L(100) + patch));
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_server_info */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_version_string)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->server_version_string;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_host_info */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_host_info)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->server_host_info;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_protocol_info */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_protocol_info)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return "X Protocol";
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_charset_name */
static const char *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_charset_name)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	return session->charset->name;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_server_option */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session_data, set_server_option)(XMYSQLND_NODE_SESSION_DATA * const session, const enum_xmysqlnd_server_option option, const char * const value)
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
MYSQLND_METHOD(xmysqlnd_node_session_data, set_client_option)(XMYSQLND_NODE_SESSION_DATA * const session, enum_xmysqlnd_client_option option, const char * const value)
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
			ret = session->io.vio->data->m.set_client_option(session->io.vio, option, value);
			break;
		default:
			ret = FAIL;
	}
	session->m->local_tx_end(session, this_func, ret);
	DBG_RETURN(ret);
//oom:
	SET_OOM_ERROR(session->error_info);
	session->m->local_tx_end(session, this_func, FAIL);
end:
	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_contents */
static void
MYSQLND_METHOD(xmysqlnd_node_session_data, free_contents)(XMYSQLND_NODE_SESSION_DATA * session)
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
	if (session->server_version_string) {
		mnd_pefree(session->server_version_string, pers);
		session->server_version_string = NULL;
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
MYSQLND_METHOD(xmysqlnd_node_session_data, dtor)(XMYSQLND_NODE_SESSION_DATA * session)
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

	if (session->stats) {
		mysqlnd_stats_end(session->stats, session->persistent);
	}

	mnd_pefree(session, session->persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_options */
static void
MYSQLND_METHOD(xmysqlnd_node_session_data, free_options)(XMYSQLND_NODE_SESSION_DATA * session)
{
//	const zend_bool pers = session->persistent;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_reference */
static XMYSQLND_NODE_SESSION_DATA *
MYSQLND_METHOD(xmysqlnd_node_session_data, get_reference)(XMYSQLND_NODE_SESSION_DATA * const session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_reference");
	++session->refcount;
	DBG_INF_FMT("session=%p new_refcount=%u", session, session->refcount);
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_reference */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session_data, free_reference)(XMYSQLND_NODE_SESSION_DATA * const session)
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
MYSQLND_METHOD(xmysqlnd_node_session_data, send_close)(XMYSQLND_NODE_SESSION_DATA * const session)
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
			struct st_xmysqlnd_connection_close_ctx conn_close_msg = msg_factory.get__connection_close(&msg_factory);
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
MYSQLND_METHOD(xmysqlnd_node_session_data, ssl_set)(XMYSQLND_NODE_SESSION_DATA * const session,
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
MYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_start)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_start");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::local_tx_end */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_end)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func, const enum_func_status status)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_end");
	DBG_RETURN(status);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::negotiate_client_api_capabilities */
static size_t
MYSQLND_METHOD(xmysqlnd_node_session_data, negotiate_client_api_capabilities)(XMYSQLND_NODE_SESSION_DATA * const session, const size_t flags)
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
MYSQLND_METHOD(xmysqlnd_node_session_data, get_client_api_capabilities)(const XMYSQLND_NODE_SESSION_DATA * const session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_client_api_capabilities");
	DBG_RETURN(session? session->client_api_capabilities : 0);
}
/* }}} */



MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session_data)
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_scheme),
	MYSQLND_METHOD(xmysqlnd_node_session_data, connect_handshake),
	MYSQLND_METHOD(xmysqlnd_node_session_data, authenticate),
	MYSQLND_METHOD(xmysqlnd_node_session_data, connect),
	MYSQLND_METHOD(xmysqlnd_node_session_data, escape_string),
	MYSQLND_METHOD(xmysqlnd_node_session_data, create_statement),

	MYSQLND_METHOD(xmysqlnd_node_session_data, get_error_no),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_error),
	MYSQLND_METHOD(xmysqlnd_node_session_data, sqlstate),

	MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_version),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_version_string),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_server_host_info),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_protocol_info),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_charset_name),

	MYSQLND_METHOD(xmysqlnd_node_session_data, set_server_option),
	MYSQLND_METHOD(xmysqlnd_node_session_data, set_client_option),

	MYSQLND_METHOD(xmysqlnd_node_session_data, free_contents),
	MYSQLND_METHOD(xmysqlnd_node_session_data, free_options),
	MYSQLND_METHOD(xmysqlnd_node_session_data, dtor),

	MYSQLND_METHOD(xmysqlnd_node_session_data, get_reference),
	MYSQLND_METHOD(xmysqlnd_node_session_data, free_reference),

	MYSQLND_METHOD(xmysqlnd_node_session_data, send_close),
	MYSQLND_METHOD(xmysqlnd_node_session_data, ssl_set),

	MYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_start),
	MYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_end),

	MYSQLND_METHOD(xmysqlnd_node_session_data, negotiate_client_api_capabilities),
	MYSQLND_METHOD(xmysqlnd_node_session_data, get_client_api_capabilities),
MYSQLND_CLASS_METHODS_END;



/* {{{ xmysqlnd_node_session::test */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session, test)(XMYSQLND_NODE_SESSION * session_handle, const char * test_data)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), close);
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	enum_func_status ret = PASS;

	DBG_ENTER("xmysqlnd_node_session::test");

	if (PASS == session->m->local_tx_start(session, this_func)) {

		printf("TEST %s!!!!\n", test_data);

		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_node_session::connect */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session, connect)(XMYSQLND_NODE_SESSION * session_handle,
						MYSQLND_CSTRING hostname,
						MYSQLND_CSTRING username,
						MYSQLND_CSTRING password,
						MYSQLND_CSTRING database,
						MYSQLND_CSTRING socket_or_pipe,
						unsigned int port,
						size_t set_capabilities)
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
			DBG_INF_FMT("no password given, using empty string");
			database.s = "";
			database.l = 0;
		}

		ret = session->m->connect(session, hostname, username, password, database, socket_or_pipe, port, set_capabilities);

		session->m->local_tx_end(session, this_func, FAIL);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::dtor */
static void
MYSQLND_METHOD(xmysqlnd_node_session, dtor)(XMYSQLND_NODE_SESSION * session)
{
	DBG_ENTER("xmysqlnd_node_session::dtor");

	session->data->m->free_reference(session->data);

	mnd_pefree(session, session->persistent);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session::select_db */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session, select_db)(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING db)
{
	enum_func_status ret = FAIL;
	char query[3+ 1 + 2 + 64*4 + 1];
	const size_t query_len = snprintf(query, sizeof(query), "USE `%*s`", db.l, db.s);
	const MYSQLND_CSTRING select_query = { query, query_len };

	DBG_ENTER("xmysqlnd_node_session::select_db");
	DBG_INF_FMT("db=%s", db);

	ret = session_handle->m->query(session_handle, select_query);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session, query)(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING query)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), query);
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	enum_func_status ret = FAIL;

	DBG_ENTER("xmysqlnd_node_session::close");
	if (PASS == session->m->local_tx_start(session, this_func)) {
		XMYSQLND_NODE_STMT * stmt = session->m->create_statement(session, query, MYSQLND_SEND_QUERY_IMPLICIT);
		if (stmt) {
			if (PASS == stmt->data->m.send_query(stmt, session->stats, session->error_info)) {
				zend_bool has_more = FALSE;
				do {
					if (PASS == stmt->data->m.get_buffered_result(stmt, &has_more, session->stats, session->error_info)) {
						ret = PASS;
					}
				} while (has_more == TRUE);
			}
			xmysqlnd_node_stmt_free(stmt, session->stats, session->error_info);
		}

		/* If we do it after free_reference/dtor then we might crash */
		session->m->local_tx_end(session, this_func, ret);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::close */
static enum_func_status
MYSQLND_METHOD(xmysqlnd_node_session, close)(XMYSQLND_NODE_SESSION * session_handle, const enum_xmysqlnd_node_session_close_type close_type)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), close);
	XMYSQLND_NODE_SESSION_DATA * session = session_handle->data;
	enum_func_status ret = FAIL;

	DBG_ENTER("xmysqlnd_node_session::close");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		if (GET_SESSION_STATE(&session->state) >= NODE_SESSION_READY) {
			static enum_mysqlnd_collected_stats close_type_to_stat_map[XMYSQLND_CLOSE_LAST] = {
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

		session_handle->m->dtor(session_handle);
	}
	DBG_RETURN(ret);
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session)
	MYSQLND_METHOD(xmysqlnd_node_session, test),
	MYSQLND_METHOD(xmysqlnd_node_session, connect),
	MYSQLND_METHOD(xmysqlnd_node_session, select_db),
	MYSQLND_METHOD(xmysqlnd_node_session, query),
	MYSQLND_METHOD(xmysqlnd_node_session, dtor),
	MYSQLND_METHOD(xmysqlnd_node_session, close),
MYSQLND_CLASS_METHODS_END;


/* {{{ xmysqlnd_node_session_init */
PHPAPI XMYSQLND_NODE_SESSION *
xmysqlnd_node_session_init(const size_t client_flags, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory)
{
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory = object_factory? object_factory : &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
	XMYSQLND_NODE_SESSION * session;

	DBG_ENTER("xmysqlnd_node_session_init");
	session = factory->get_node_session(factory, persistent);
	if (session && session->data) {
		session->data->m->negotiate_client_api_capabilities(session->data, client_flags);
	}
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_connect */
PHPAPI XMYSQLND_NODE_SESSION *
xmysqlnd_node_session_connect(XMYSQLND_NODE_SESSION * session,
							  const MYSQLND_CSTRING hostname,
							  const MYSQLND_CSTRING username,
							  const MYSQLND_CSTRING password,
							  const MYSQLND_CSTRING database,
							  const MYSQLND_CSTRING socket_or_pipe,
							  unsigned int port,
							  size_t set_capabilities,
							  size_t client_api_flags
						)
{
	enum_func_status ret = FAIL;
	zend_bool self_alloced = FALSE;

	DBG_ENTER("xmysqlnd_node_session_connect");
	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%llu", hostname.s, username.s, database.s, port, (unsigned long long) set_capabilities);

	if (!session) {
		self_alloced = TRUE;
		if (!(session = xmysqlnd_node_session_init(client_api_flags, FALSE, NULL))) {
			/* OOM */
			DBG_RETURN(NULL);
		}
	}

	ret = session->m->connect(session, hostname, username, password, database, socket_or_pipe, port, set_capabilities);

	if (ret == FAIL) {
		if (self_alloced) {
			/*
			  We have alloced, thus there are no references to this
			  object - we are free to kill it!
			*/
			session->m->dtor(session);
		}
		DBG_RETURN(NULL);
	}
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_module_init */
PHPAPI void
xmysqlnd_node_session_module_init()
{
	xmysqlnd_node_session_set_methods(&MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_session));
	xmysqlnd_node_session_data_set_methods(&MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_session_data));
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
