/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_charset.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_auth.h> /* php_mysqlnd_scramble */
}
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h"
#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_environment.h"
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
#include "xmysqlnd_utils.h"
#include "mysqlx_base_session.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "SAPI.h"
#include "php_variables.h"
#include "mysqlx_exception.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/url_utils.h"
#include <utility>
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace mysqlx {

namespace drv {

const std::vector<std::string> st_xmysqlnd_session_auth_data::supported_ciphers = {
	"AES128-GCM-SHA256",
	"AES128-RMD",
	"AES128-SHA",
	"AES128-SHA256",
	"AES256-GCM-SHA384",
	"AES256-RMD",
	"AES256-SHA",
	"AES256-SHA256",
	"DES-CBC3-RMD",
	"DES-CBC3-SHA",
	"DES-CBC-SHA",
	"DH-DSS-AES128-GCM-SHA256",
	"DH-DSS-AES128-SHA",
	"DH-DSS-AES128-SHA256",
	"DH-DSS-AES256-GCM-SHA384",
	"DH-DSS-AES256-SHA",
	"DH-DSS-AES256-SHA256",
	"DHE-DSS-AES128-GCM-SHA256",
	"DHE-DSS-AES128-SHA",
	"DHE-DSS-AES128-SHA256",
	"DHE-DSS-AES256-GCM-SHA384",
	"DHE-DSS-AES256-SHA256",
	"DHE-ECDSA-AES128-GCM-SHA256",
	"DHE-RSA-AES128-GCM-SHA256",
	"DHE-RSA-AES128-RMD",
	"DHE-RSA-AES128-SHA",
	"DHE-RSA-AES128-SHA256",
	"DHE-RSA-AES256-GCM-SHA384",
	"DHE-RSA-AES256-RMD",
	"DHE-RSA-AES256-SHA",
	"DHE-RSA-AES256-SHA256",
	"DHE-RSA-DES-CBC3-RMD",
	"DH-RSA-AES128-GCM-SHA256",
	"DH-RSA-AES128-SHA",
	"DH-RSA-AES128-SHA256",
	"DH-RSA-AES256-GCM-SHA384",
	"DH-RSA-AES256-SHA",
	"DH-RSA-AES256-SHA256",
	"ECDH-ECDSA-AES128-GCM-SHA256",
	"ECDH-ECDSA-AES128-SHA",
	"ECDH-ECDSA-AES128-SHA256",
	"ECDH-ECDSA-AES256-GCM-SHA384",
	"ECDH-ECDSA-AES256-SHA",
	"ECDH-ECDSA-AES256-SHA384",
	"ECDHE-ECDSA-AES128-SHA",
	"ECDHE-ECDSA-AES128-SHA256",
	"ECDHE-ECDSA-AES256-GCM-SHA384",
	"ECDHE-ECDSA-AES256-SHA",
	"ECDHE-ECDSA-AES256-SHA384",
	"ECDHE-RSA-AES128-GCM-SHA256",
	"ECDHE-RSA-AES128-SHA",
	"ECDHE-RSA-AES128-SHA256",
	"ECDHE-RSA-AES256-GCM-SHA384",
	"ECDHE-RSA-AES256-SHA",
	"ECDHE-RSA-AES256-SHA384",
	"ECDH-RSA-AES128-GCM-SHA256",
	"ECDH-RSA-AES128-SHA",
	"ECDH-RSA-AES128-SHA256",
	"ECDH-RSA-AES256-GCM-SHA384",
	"ECDH-RSA-AES256-SHA",
	"ECDH-RSA-AES256-SHA384",
	"EDH-RSA-DES-CBC3-SHA",
	"EDH-RSA-DES-CBC-SHA",
	"RC4-MD5",
	"RC4-SHA",
	//What follows is the list of deprecated
	//TLS ciphers
	"!DHE-DSS-DES-CBC3-SHA",
	"!DHE-RSA-DES-CBC3-SHA",
	"!ECDH-RSA-DES-CBC3-SHA",
	"!ECDH-ECDSA-DES-CBC3-SHA",
	"!ECDHE-RSA-DES-CBC3-SHA",
	"!ECDHE-ECDSA-DES-CBC3-SHA",
	//What follows is the list of unacceptables
	//TLS ciphers
	"!eNULL",
	"!EXPORT",
	"!LOW",
	"!MD5",
	"!PSK",
	"!RC2",
	"!RC4",
	"!aNULL",
	"!DES",
//	"!SSLv3" This is not acceptable, but required by the server!!! BAD
};

const MYSQLND_CSTRING namespace_mysqlx{ "mysqlx", sizeof("mysqlx") - 1 };
const MYSQLND_CSTRING namespace_sql{ "sql", sizeof("sql") - 1 };
const MYSQLND_CSTRING namespace_xplugin{ "xplugin", sizeof("xplugin") - 1 };

st_xmysqlnd_session_auth_data::st_xmysqlnd_session_auth_data() :
	port{ 0 },
	ssl_mode{ SSL_mode::not_specified },
	ssl_enabled{ false },
	ssl_no_defaults{ true } {
}

namespace {

const char* Auth_method_mysql41 = "MYSQL41";
const char* Auth_method_plain = "PLAIN";
const char* Auth_method_external = "EXTERNAL";
const char* Auth_method_unspecified = "";

/* {{{ xmysqlnd_throw_exception_from_session_if_needed */
zend_bool
xmysqlnd_throw_exception_from_session_if_needed(const XMYSQLND_NODE_SESSION_DATA session)
{
	const unsigned int error_num = session->m->get_error_no(session);
	DBG_ENTER("xmysqlnd_throw_exception_from_session_if_needed");
	if (error_num) {
		MYSQLND_CSTRING sqlstate = { session->m->get_sqlstate(session) , 0 };
		MYSQLND_CSTRING errmsg = { session->m->get_error_str(session) , 0 };
		sqlstate.l = strlen(sqlstate.s);
		errmsg.l = strlen(errmsg.s);
		devapi::mysqlx_new_exception(error_num, sqlstate, errmsg);
		DBG_RETURN(TRUE);
	}
	DBG_RETURN(FALSE);
}
/* }}} */

} // anonymous namespace

const st_xmysqlnd_node_session_query_bind_variable_bind noop__var_binder = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_result_start_bind noop__on_result_start = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_row_bind noop__on_row = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_warning_bind noop__on_warning = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_error_bind noop__on_error = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_result_end_bind noop__on_result_end = { nullptr, nullptr };
const st_xmysqlnd_node_session_on_statement_ok_bind noop__on_statement_ok = { nullptr, nullptr };

namespace {

/* {{{ xmysqlnd_node_session_state::get */
enum xmysqlnd_node_session_state
XMYSQLND_METHOD(xmysqlnd_node_session_state, get)(const XMYSQLND_NODE_SESSION_STATE * const state_struct)
{
	DBG_ENTER("xmysqlnd_node_session_state::get");
	DBG_INF_FMT("State=%u", state_struct->state);
	DBG_RETURN(state_struct->state);
}
/* }}} */


/* {{{ xmysqlnd_node_session_state::set */
void
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

} // anonymous namespace

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


/* {{{ xmysqlnd_node_session_data::st_xmysqlnd_node_session_data */
st_xmysqlnd_node_session_data::st_xmysqlnd_node_session_data(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
						MYSQLND_STATS * mysqlnd_stats,
						MYSQLND_ERROR_INFO * mysqlnd_error_info)
{
	DBG_ENTER("xmysqlnd_node_session_data::st_xmysqlnd_node_session_data");
	object_factory = factory;

	if (error_info) {
		error_info = mysqlnd_error_info? mysqlnd_error_info : &error_info_impl;
	} else {
		if (FAIL == mysqlnd_error_info_init(&error_info_impl, persistent)) {
			throw std::runtime_error("mysqlnd_error_info_init failed");
		}
		error_info = &error_info_impl;
	}

	options = &(options_impl);

	xmysqlnd_node_session_state_init(&state);

	if (stats) {
		stats = mysqlnd_stats;
		own_stats = FALSE;
	} else {
		mysqlnd_stats_init(&stats, STAT_LAST, persistent);
		own_stats = TRUE;
	}

	io.pfc = xmysqlnd_pfc_create(persistent, object_factory, mysqlnd_stats, error_info);
	io.vio = mysqlnd_vio_init(persistent, nullptr, mysqlnd_stats, error_info);
	charset = mysqlnd_find_charset_name(XMYSQLND_NODE_SESSION_CHARSET);

	if (!io.pfc || !io.vio || !charset) {
		cleanup();
		free_contents();
		throw std::runtime_error("Unable to create the object");
	}

	savepoint_name_seed = 1;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::~st_xmysqlnd_node_session_data */
st_xmysqlnd_node_session_data::~st_xmysqlnd_node_session_data()
{
	DBG_ENTER("xmysqlnd_node_session_data::~st_xmysqlnd_node_session_data");
	cleanup();
	free_contents();
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::cleanup */
void st_xmysqlnd_node_session_data::cleanup()
{
	DBG_ENTER("xmysqlnd_node_session_data::reset");
	zend_bool pers = persistent;

	if (io.pfc) {
		io.pfc->data->m.free_contents(io.pfc);
	}

	if (io.vio) {
		io.vio->data->m.free_contents(io.vio);
	}

	if(auth) {
		delete auth;
		auth = nullptr;
	}
	if (current_db.s) {
		mnd_pefree(current_db.s, pers);
		current_db.s = nullptr;
	}

	m->free_options(this);

	if (scheme.s) {
		mnd_pefree(scheme.s, pers);
		scheme.s = nullptr;
	}
	if (server_host_info) {
		mnd_pefree(server_host_info, pers);
		server_host_info = nullptr;
	}
	if (error_info->error_list) {
		zend_llist_clean(error_info->error_list);
		mnd_pefree(error_info->error_list, pers);
		error_info->error_list = nullptr;
	}
	charset = nullptr;

	if (stats && own_stats) {
		mysqlnd_stats_end(stats, persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_contents */
void st_xmysqlnd_node_session_data::free_contents()
{
	DBG_ENTER("xmysqlnd_node_session_data::free_contents");
	if( io.pfc ) {
		xmysqlnd_pfc_free(io.pfc, stats, error_info);
		io.pfc = nullptr;
	}
	if (io.vio) {
		mysqlnd_vio_free(io.vio, stats, error_info);
		io.vio = nullptr;
	}
	DBG_VOID_RETURN;
}
/* }}} */


namespace {

/* {{{ xmysqlnd_node_session_data::init *//* FILIP:
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, init)(st_xmysqlnd_node_session_data * object,
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
	object->io.vio = mysqlnd_vio_init(object->persistent, nullptr, object->stats, object->error_info);

	object->charset = mysqlnd_find_charset_name(XMYSQLND_NODE_SESSION_CHARSET);

	if (!object->io.pfc || !object->io.vio || !object->charset) {
		DBG_RETURN(FAIL);
	}

	object->savepoint_name_seed = 1;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_scheme */
MYSQLND_STRING
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_scheme)(XMYSQLND_NODE_SESSION_DATA session,
                                                        const util::string& hostname,
                                                        unsigned int port)
{
	MYSQLND_STRING transport;
	DBG_ENTER("xmysqlnd_node_session_data::get_scheme");
	/* MY-305: Add support for windows pipe */
	if( session->transport_type == transport_types::network ) {
		if (!port) {
			port = drv::Environment::get_as_int(drv::Environment::Variable::Mysql_port);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "tcp://%s:%u", hostname.c_str(), port);
	} else if( session->transport_type == transport_types::unix_domain_socket ){
		transport.l = mnd_sprintf(&transport.s, 0, "unix://%s",
								 session->socket_path.c_str());
	} else if( session->transport_type == transport_types::windows_pipe ) {
#ifdef PHP_WIN32
		/* Somewhere here?! (This is old code) */
		if (hostname == ".") {
			/* named pipe in socket */
			session->socket_path = "\\\\.\\pipe\\MySQL";
		}
		transport.l = mnd_sprintf(&transport.s, 0, "pipe://%s", session->socket_path.c_str());
#else
		DBG_ERR_FMT("Windows pipe not supported!.");
		devapi::RAISE_EXCEPTION( err_msg_internal_error );
#endif
	} else {
		/*
		 * At this point, there must be a selected transport type!
		 */
		DBG_ERR_FMT("Transport type invalid.");
		devapi::RAISE_EXCEPTION( err_msg_internal_error );
	}
	DBG_INF_FMT("transport=%s", transport.s? transport.s:"OOM");
	DBG_RETURN(transport);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt::handler_on_warning */
const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_error)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
        st_xmysqlnd_node_session_data * session = (st_xmysqlnd_node_session_data *) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_error");
	if (session->error_info) {
		SET_CLIENT_ERROR(session->error_info, code, sql_state.s, message.s);
	}
	php_error_docref(nullptr, E_WARNING, "[%4u][%s] %s", code, sql_state.s? sql_state.s:"", message.s? message.s:"");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

struct st_xmysqlnd_auth_41_ctx
{
        XMYSQLND_NODE_SESSION_DATA session;
	MYSQLND_CSTRING scheme;
	util::string username;
	util::string password;
	MYSQLND_CSTRING database;

};

const char hexconvtab[] = "0123456789abcdef";


/* {{{ xmysqlnd_node_stmt::handler_on_auth_continue */
const enum_hnd_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, handler_on_auth_continue)(void * context,
																const MYSQLND_CSTRING input,
																MYSQLND_STRING * const output)
{
	const MYSQLND_CSTRING salt = input;
	st_xmysqlnd_auth_41_ctx* ctx = (st_xmysqlnd_auth_41_ctx*) context;
	DBG_ENTER("xmysqlnd_node_stmt::handler_on_auth_continue");
	DBG_INF_FMT("salt[%d]=%s", salt.l, salt.s);
	if (salt.s) {
		const zend_bool to_hex = !ctx->password.empty();
		char hexed_hash[SCRAMBLE_LENGTH*2];
		if (to_hex) {
			zend_uchar hash[SCRAMBLE_LENGTH];

			php_mysqlnd_scramble(hash, (zend_uchar*) salt.s, (const zend_uchar*) ctx->password.c_str(),
								 ctx->password.size());
			for (unsigned int i{0}; i < SCRAMBLE_LENGTH; i++) {
				hexed_hash[i*2] = hexconvtab[hash[i] >> 4];
				hexed_hash[i*2 + 1] = hexconvtab[hash[i] & 15];
			}
			DBG_INF_FMT("hexed_hash=%s", hexed_hash);
		}

		{
			const std::size_t answer_length{ctx->database.l + 1 + ctx->username.size() + 1 +
				1 + (to_hex ? SCRAMBLE_LENGTH * 2 : 0) + 1};
			util::string answer_str(answer_length, '\0');
			char* answer{&answer_str.front()};
			char* p{answer};
			memcpy(p, ctx->database.s, ctx->database.l);
			p+= ctx->database.l;
			*p++ = '\0';
			memcpy(p, ctx->username.c_str(), ctx->username.size());
			p+= ctx->username.size();
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
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_client_id */
size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_id)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session? session->client_id : 0;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_client_id */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_id)(void * context, const size_t id)
{
	enum_func_status ret{FAIL};
        st_xmysqlnd_node_session_data * session = (st_xmysqlnd_node_session_data *) context;
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
zend_bool
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
zend_bool
xmysqlnd_is_mysql41_supported(const zval * capabilities)
{
	zval* entry{nullptr};
	const zval * auth_mechs = zend_hash_str_find(Z_ARRVAL_P(capabilities), "authentication.mechanisms", sizeof("authentication.mechanisms") - 1);
	if (!capabilities || Z_TYPE_P(auth_mechs) != IS_ARRAY) {
		return FALSE;
	}
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(auth_mechs), entry) {
		if (!strcasecmp(Z_STRVAL_P(entry), Auth_method_mysql41)) {
			return TRUE;
		}
	} ZEND_HASH_FOREACH_END();
	return FALSE;
}
/* }}} */


/* {{{ setup_crypto_options */
void setup_crypto_options(
		php_stream_context* stream_context,
                XMYSQLND_NODE_SESSION_DATA session)
{
	DBG_ENTER("setup_crypto_options");
	const XMYSQLND_SESSION_AUTH_DATA * auth = session->auth;
	zval string;

	//Add client key
	if( false == auth->ssl_local_pk.empty() ) {
		DBG_INF_FMT("Setting client-key %s",
					auth->ssl_local_pk.c_str());
		ZVAL_STRING(&string, auth->ssl_local_pk.c_str());
		php_stream_context_set_option(stream_context,"ssl","local_pk",&string);
		zval_ptr_dtor(&string);
	}

	//Add client certificate
	if( false == auth->ssl_local_cert.empty() ) {
		DBG_INF_FMT("Setting client-cert %s",
					auth->ssl_local_cert.c_str());
		ZVAL_STRING(&string, auth->ssl_local_cert.c_str());
		php_stream_context_set_option(stream_context,"ssl","local_cert",&string);
		zval_ptr_dtor(&string);
	}


	//Set CA
	if( false == auth->ssl_cafile.empty() ) {
		DBG_INF_FMT("Setting CA %s",
					auth->ssl_cafile.c_str());
		ZVAL_STRING(&string, auth->ssl_cafile.c_str());
		php_stream_context_set_option(stream_context,"ssl","cafile",&string);
		zval_ptr_dtor(&string);
	}

	//Provide the list of supported/unsupported ciphers
	util::string cipher_list;
	for( auto& cipher : auth->supported_ciphers ) {
		cipher_list += cipher.c_str();
		cipher_list += ':';
	}

	ZVAL_STRING(&string, cipher_list.c_str());
	php_stream_context_set_option(stream_context,"ssl","ciphers",&string);
	zval_ptr_dtor(&string);

	//Self certification accepted
	DBG_INF_FMT("Accepting self-certified CA!");
	zval verify_peer_zval;
	ZVAL_TRUE(&verify_peer_zval);
	php_stream_context_set_option(stream_context,"ssl","allow_self_signed",&verify_peer_zval);

	ZVAL_FALSE(&verify_peer_zval);
	php_stream_context_set_option(stream_context,"ssl","verify_peer_name",&verify_peer_zval);
	DBG_VOID_RETURN;
}

/* }}} */


/* {{{ setup_crypto_connection */
enum_func_status setup_crypto_connection(
                XMYSQLND_NODE_SESSION_DATA session,
		st_xmysqlnd_msg__capabilities_get& caps_get,
		const st_xmysqlnd_message_factory& msg_factory)
{
	DBG_ENTER("setup_crypto_connection");
	enum_func_status ret{FAIL};
	const struct st_xmysqlnd_on_error_bind on_error =
                        { session->m->handler_on_error, (void*) session.get() };
	//Attempt to set the TLS capa. flag.
	st_xmysqlnd_msg__capabilities_set caps_set;
	caps_set = msg_factory.get__capabilities_set(&msg_factory);

	zval ** capability_names = (zval **) mnd_ecalloc(1, sizeof(zval*));
	zval ** capability_values = (zval **) mnd_ecalloc(1, sizeof(zval*));
	zval  name, value;

	const MYSQLND_CSTRING cstr_name = { "tls", 3 };

	ZVAL_STRINGL(&name,cstr_name.s,cstr_name.l);

	capability_names[0] = &name;
	ZVAL_TRUE(&value);
	capability_values[0] = &value;

	if( PASS == caps_set.send_request(&caps_set,
							1,
							capability_names,
							capability_values))
	{
		DBG_INF_FMT("Cap. send request with tls=true success, reading response..!");
		zval value;
		caps_get.init_read(&caps_get, on_error);
		ret = caps_get.read_response(&caps_get,
									 &value);
		if( ret == PASS ) {
			DBG_INF_FMT("Cap. response OK, setting up TLS options.!");
			php_stream_context * context = php_stream_context_alloc();
			MYSQLND_VIO * vio = session->io.vio;
			php_stream * net_stream = vio->data->m.get_stream(vio);
			//Setup all the options
			setup_crypto_options(context,session);
			//Attempt to enable the stream with the crypto
			//settings.
			php_stream_context_set(net_stream, context);
			if (php_stream_xport_crypto_setup(net_stream, STREAM_CRYPTO_METHOD_TLS_CLIENT, nullptr) < 0 ||
				php_stream_xport_crypto_enable(net_stream, 1) < 0)
			{
				DBG_ERR_FMT("Cannot connect to MySQL by using SSL");
				php_error_docref(nullptr, E_WARNING, "Cannot connect to MySQL by using SSL");
				ret = FAIL;
			} else {
				php_stream_context_set( net_stream, nullptr );
			}
		} else {
			DBG_ERR_FMT("Negative response from the server, not able to setup TLS.");
		}

	}

	//Cleanup
	zval_ptr_dtor(&name);
	zval_ptr_dtor(&value);
	if( capability_names ) {
		mnd_efree(capability_names);
	}
	if( capability_values ) {
		mnd_efree(capability_values);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::authenticate */
util::string auth_mode_to_str(const Auth_mode auth_mode)
{
	using auth_mode_to_label = std::map<Auth_mode, std::string>;
	static const auth_mode_to_label auth_mode_to_labels = {
		{ Auth_mode::mysql41, Auth_method_mysql41 },
		{ Auth_mode::plain, Auth_method_plain },
		{ Auth_mode::external, Auth_method_external },
		{ Auth_mode::unspecified, Auth_method_unspecified }
	};

	return auth_mode_to_labels.at(auth_mode).c_str();
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::authenticate */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, authenticate)(
                                                XMYSQLND_NODE_SESSION_DATA session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret{FAIL};
	const XMYSQLND_SESSION_AUTH_DATA * auth = session->auth;
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->io, session->stats, session->error_info);
        const struct st_xmysqlnd_on_error_bind on_error = { session->m->handler_on_error, (void*) session.get() };
        const struct st_xmysqlnd_on_client_id_bind on_client_id = { session->m->set_client_id, (void*) session.get() };
        const struct st_xmysqlnd_on_warning_bind on_warning = { nullptr, (void*) session.get() };
        const struct st_xmysqlnd_on_session_var_change_bind on_session_var_change = { nullptr, (void*) session.get() };
	struct st_xmysqlnd_auth_41_ctx auth_ctx = { session, scheme, auth->username, auth->password, database };
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
			zend_bool tls_set{FALSE};
			const zend_bool tls = xmysqlnd_get_tls_capability(&capabilities, &tls_set);
			const zend_bool mysql41_supported = xmysqlnd_is_mysql41_supported(&capabilities);

			//The X Protocol currently supports:
			//   1) PLAIN over TLS (see RFC 4616)
			//   2) MYSQL41

			DBG_INF_FMT("tls=%d tls_set=%d", tls, tls_set);
			DBG_INF_FMT("4.1 supported=%d", mysql41_supported);

			const Auth_mode user_auth_mode{auth->auth_mode};
			util::string auth_mech_name;

			if( auth->ssl_mode != SSL_mode::disabled ) {
				if( TRUE == tls_set ) {
					ret = setup_crypto_connection(session,caps_get,msg_factory);
					if (user_auth_mode == Auth_mode::unspecified) {
						auth_mech_name = Auth_method_plain;
					} else {
						auth_mech_name = auth_mode_to_str(user_auth_mode);
					}
				} else {
					ret = FAIL;
					DBG_ERR_FMT("Cannot connect to MySQL by using SSL, unsupported by the server");
					php_error_docref(nullptr, E_WARNING, "Cannot connect to MySQL by using SSL, unsupported by the server");
				}
			} else {
				if (user_auth_mode == Auth_mode::unspecified) {
					if (mysql41_supported) {
						auth_mech_name = Auth_method_mysql41;
					}
				} else {
					auth_mech_name = auth_mode_to_str(user_auth_mode);
				}
			}

			DBG_INF_FMT("Authentication mechanism: %s",
						auth_mech_name.c_str());

			if( ret == PASS ) {
				//Complete the authentication procedure!
				const MYSQLND_CSTRING method = { auth_mech_name.c_str(),auth_mech_name.size() };
				const util::string authdata = '\0' + auth->username + '\0' + auth->password;
				st_xmysqlnd_msg__auth_start auth_start_msg = msg_factory.get__auth_start(&msg_factory);
				ret = auth_start_msg.send_request(&auth_start_msg,
									method, {authdata.c_str(), authdata.size()});

				if (ret == PASS) {
					auth_start_msg.init_read(&auth_start_msg,
									on_auth_continue,
									on_warning,
									on_error,
									on_client_id,
									on_session_var_change);
					ret = auth_start_msg.read_response(&auth_start_msg, nullptr);
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
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, connect_handshake)(XMYSQLND_NODE_SESSION_DATA session,
						const MYSQLND_CSTRING scheme,
						const MYSQLND_CSTRING database,
						const size_t set_capabilities)
{
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_node_session_data::connect_handshake");

	if (PASS == session->io.vio->data->m.connect(session->io.vio,
									scheme,
									session->persistent,
									session->stats,
									session->error_info) &&
		PASS == session->io.pfc->data->m.reset(session->io.pfc,
									session->stats,
									session->error_info)) {
		SET_CONNECTION_STATE(&session->state, NODE_SESSION_NON_AUTHENTICATED);
		ret = session->m->authenticate(session, scheme, database, set_capabilities);
	}
	DBG_RETURN(ret);
}
/* }}} */

char* get_server_host_info(const util::string& format,
					const util::string& name,
					zend_bool session_persistent)
{
	char *hostname{ nullptr }, *host_info{ nullptr };
	mnd_sprintf(&hostname, 0, format.c_str(), name.c_str());
	if (hostname) {
		host_info = mnd_pestrdup(hostname, session_persistent);
		mnd_sprintf_free(hostname);
	}
	return host_info;
}

/* {{{ xmysqlnd_node_session_data::connect */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, connect)(XMYSQLND_NODE_SESSION_DATA session,
						MYSQLND_CSTRING database,
						unsigned int port,
						size_t set_capabilities)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), connect);
	zend_bool reconnect{FALSE};
	MYSQLND_STRING transport = { nullptr, 0 };
	const XMYSQLND_SESSION_AUTH_DATA * auth = session->auth;
	enum_func_status ret{PASS};

	DBG_ENTER("xmysqlnd_node_session_data::connect");
	DBG_INF_FMT("session=%p", session.get());

	SET_EMPTY_ERROR(session->error_info);

	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%u persistent=%u state=%u",
				!auth->hostname.empty()?auth->hostname.c_str():"",
				!auth->username.empty()?auth->username.c_str():"",
				database.s?database.s:"", port, (uint) set_capabilities,
				session? session->persistent:0,
				session? GET_SESSION_STATE(&session->state):-1);

	if (GET_SESSION_STATE(&session->state) > NODE_SESSION_ALLOCED) {
		DBG_INF("Connecting on a connected handle.");

		if (GET_SESSION_STATE(&session->state) < NODE_SESSION_CLOSE_SENT) {
			XMYSQLND_INC_SESSION_STATISTIC(session->stats, XMYSQLND_STAT_CLOSE_IMPLICIT);
			reconnect = TRUE;
			session->m->send_close(session);
		}

		session->cleanup();
	}

	/* Setup the relevant variables! */
	session->current_db.l = database.l;
	session->current_db.s = mnd_pestrndup(database.s,
					session->current_db.l, session->persistent);

	transport = session->m->get_scheme(session,
					auth->hostname,
					port);

	if( nullptr == transport.s || transport.l == 0 ) {
		ret = FAIL;
	} else {
		session->scheme.s = mnd_pestrndup(transport.s, transport.l,
						session->persistent);
		session->scheme.l = transport.l;

		mnd_sprintf_free(transport.s);
		transport.s = nullptr;

		if (!session->scheme.s || !session->current_db.s) {
			SET_OOM_ERROR(session->error_info);
			ret = FAIL;
		}
	}

	/* Attempt to connect */
	if( ret == PASS ) {
		const MYSQLND_CSTRING scheme = { session->scheme.s, session->scheme.l };
		ret = session->m->connect_handshake(session,
						scheme, database,
						set_capabilities);
		if( ret != PASS ) {
			SET_OOM_ERROR(session->error_info);
		}
	}

	/* Setup server host information */
	if( ret == PASS ) {
		SET_CONNECTION_STATE(&session->state, NODE_SESSION_READY);
		transport_types transport = session->transport_type;

		if ( transport == transport_types::network ) {
			session->server_host_info = get_server_host_info("%s via TCP/IP",
								session->auth->hostname.c_str(),
								session->persistent);
		} else if( transport == transport_types::unix_domain_socket ) {
			session->server_host_info = mnd_pestrdup("Localhost via UNIX socket",
												session->persistent);
		} else if( transport == transport_types::windows_pipe) {
			session->server_host_info = get_server_host_info("%s via named pipe",
												session->socket_path.c_str(),
												session->persistent);
		}

		if ( !session->server_host_info ) {
			SET_OOM_ERROR(session->error_info);
			ret = FAIL;
		}
	}

	/* Done, setup remaining information */
	if( ret == PASS ) {
		SET_EMPTY_ERROR(session->error_info);

		XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(session->stats,
							XMYSQLND_STAT_CONNECT_SUCCESS,
							1,
							XMYSQLND_STAT_OPENED_CONNECTIONS,
							1);
		if (reconnect) {
			XMYSQLND_INC_GLOBAL_STATISTIC(XMYSQLND_STAT_RECONNECT);
		}
		if (session->persistent) {
			XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(session->stats,
								XMYSQLND_STAT_PCONNECT_SUCCESS,
								1,
								XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS,
								1);
		}

		session->m->local_tx_end(session, this_func, PASS);
	} else {
		DBG_ERR_FMT("[%u] %.128s (trying to connect via %s)",
				session->error_info->error_no,
				session->error_info->error,
				session->scheme.s);

		if (!session->error_info->error_no) {
			SET_CLIENT_ERROR(session->error_info,
					CR_CONNECTION_ERROR,
					UNKNOWN_SQLSTATE,
					session->error_info->error[0] ? session->error_info->error:"Unknown error");
			php_error_docref(nullptr, E_WARNING, "[%u] %.128s (trying to connect via %s)",
					session->error_info->error_no, session->error_info->error, session->scheme.s);
		}
		session->cleanup();
		XMYSQLND_INC_SESSION_STATISTIC(session->stats, XMYSQLND_STAT_CONNECT_FAILURE);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::escape_string */
size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, escape_string)(XMYSQLND_NODE_SESSION_DATA  session,
                                                           char * newstr,
                                                           const char * to_escapestr,
                                                           const size_t to_escapestr_len)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), escape_string);
	zend_ulong ret{FAIL};
	DBG_ENTER("xmysqlnd_node_session_data::escape_string");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		ret = mysqlnd_cset_escape_slashes(session->charset, newstr, to_escapestr, to_escapestr_len);
		session->m->local_tx_end(session, this_func, PASS);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::quote_name */
MYSQLND_STRING
XMYSQLND_METHOD(xmysqlnd_node_session_data, quote_name)(XMYSQLND_NODE_SESSION_DATA session,
                                                        const MYSQLND_CSTRING name)
{
	MYSQLND_STRING ret = { nullptr, 0 };
	DBG_ENTER("xmysqlnd_node_session_data::quote_name");
	DBG_INF_FMT("name=%s", name.s);
	if (name.s && name.l) {
		unsigned int occurs{0};
		for (unsigned int i{0}; i < name.l; ++i) {
			if (name.s[i] == '`') {
				++occurs;
			}
		}
		ret.l = name.l + occurs + 2 /* quotes */;
		ret.s = static_cast<char*>(mnd_emalloc(ret.l + 1));
		ret.s[0] = '`';
		if (occurs) {
			char *p = &ret.s[0];				/* should start at 0 because we pre-increment in the loop */
			for (unsigned int i{0}; i < name.l; ++i) {
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
unsigned int
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error_no)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session->error_info->error_no;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::error */
const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_error)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session->error_info->error;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::sqlstate */
const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, sqlstate)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session->error_info->sqlstate[0] ? session->error_info->sqlstate : MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_host_info */
const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_server_host_info)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session->server_host_info;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_protocol_info */
const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_protocol_info)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return "X Protocol";
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_charset_name */
const char *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_charset_name)(const XMYSQLND_NODE_SESSION_DATA session)
{
	return session->charset->name;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_server_option */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_server_option)(XMYSQLND_NODE_SESSION_DATA  session,
                                                               const enum_xmysqlnd_server_option option,
                                                               const char * const value)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), set_server_option);
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_node_session_data::set_server_option");
	if (PASS == session->m->local_tx_start(session, this_func)) {
		/* Handle COM_SET_OPTION here */

		session->m->local_tx_end(session, this_func, ret);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::set_client_option */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, set_client_option)(XMYSQLND_NODE_SESSION_DATA session,
                                                               enum_xmysqlnd_client_option option,
                                                               const char * const value)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), set_client_option);
	enum_func_status ret{PASS};
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


/* {{{ xmysqlnd_node_session_data::free_contents *//*
void
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_contents)(st_xmysqlnd_node_session_data * session)
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

	if(session->auth) {
		delete session->auth;
		session->auth = nullptr;
	}
	if (session->current_db.s) {
		mnd_pefree(session->current_db.s, pers);
		session->current_db.s = nullptr;
	}
	DBG_INF_FMT("scheme=%s", session->scheme.s);
	if (session->scheme.s) {
		mnd_pefree(session->scheme.s, pers);
		session->scheme.s = nullptr;
	}
	if (session->server_host_info) {
		mnd_pefree(session->server_host_info, pers);
		session->server_host_info = nullptr;
	}
	if (session->error_info->error_list) {
		zend_llist_clean(session->error_info->error_list);
		mnd_pefree(session->error_info->error_list, pers);
		session->error_info->error_list = nullptr;
	}
	session->charset = nullptr;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_options */
void
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_options)(st_xmysqlnd_node_session_data * session)
{
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_reference */
st_xmysqlnd_node_session_data *
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_reference)(st_xmysqlnd_node_session_data * session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_reference");
	++session->refcount;
        DBG_INF_FMT("session=%p new_refcount=%u", session, session->refcount);
        DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::free_reference */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, free_reference)(XMYSQLND_NODE_SESSION_DATA session)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_node_session_data::free_reference");
	DBG_INF_FMT("session=%p old_refcount=%u", session.get(), session->refcount);
	if (!(--session->refcount)) {
		/*
		  No multithreading issues as we don't share the connection :)
		  This will free the object too, of course because references has
		  reached zero.
		*/
		ret = session->m->send_close(session);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_send_close */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, send_close)(XMYSQLND_NODE_SESSION_DATA session)
{
	enum_func_status ret{PASS};
	MYSQLND_VIO * vio = session->io.vio;
	php_stream * net_stream = vio->data->m.get_stream(vio);
	const enum xmysqlnd_node_session_state state = GET_SESSION_STATE(&session->state);

	DBG_ENTER("mysqlnd_send_close");
	DBG_INF_FMT("session=%p vio->data->stream->abstract=%p", session.get(), net_stream? net_stream->abstract:nullptr);
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
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, ssl_set)(XMYSQLND_NODE_SESSION_DATA session,
                                                        const char * const key,
                                                        const char * const cert,
                                                        const char * const ca,
                                                        const char * const capath,
                                                        const char * const cipher)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data), ssl_set);
	enum_func_status ret{FAIL};
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
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_start)(XMYSQLND_NODE_SESSION_DATA session, const size_t this_func)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_start");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::local_tx_end */
enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session_data, local_tx_end)(XMYSQLND_NODE_SESSION_DATA session, const size_t this_func, const enum_func_status status)
{
	DBG_ENTER("xmysqlnd_node_session_data::local_tx_end");
	DBG_RETURN(status);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::negotiate_client_api_capabilities */
size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, negotiate_client_api_capabilities)(XMYSQLND_NODE_SESSION_DATA session, const size_t flags)
{
	size_t ret{0};
	DBG_ENTER("xmysqlnd_node_session_data::negotiate_client_api_capabilities");
	if (session) {
		ret = session->client_api_capabilities;
		session->client_api_capabilities = flags;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session_data::get_client_api_capabilities */
size_t
XMYSQLND_METHOD(xmysqlnd_node_session_data, get_client_api_capabilities)(const XMYSQLND_NODE_SESSION_DATA session)
{
	DBG_ENTER("xmysqlnd_node_session_data::get_client_api_capabilities");
	DBG_RETURN(session? session->client_api_capabilities : 0);
}
/* }}} */

MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session_data)
	//FILIP:XMYSQLND_METHOD(xmysqlnd_node_session_data, init),
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

	//FILIP: XMYSQLND_METHOD(xmysqlnd_node_session_data, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_session_data, free_options),


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

} // anonymous namespace

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_session_data);

namespace {

/* {{{ xmysqlnd_node_session::init */
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, init)(XMYSQLND_NODE_SESSION * session_handle,
                                        const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
                                        MYSQLND_STATS * stats,
                                        MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_session::init");

	session_handle->session_uuid = new Uuid_generator();
        st_xmysqlnd_node_session_data * session_data = factory->get_node_session_data(factory, session_handle->persistent, stats, error_info);
	if (session_data) {
                session_handle->data = std::shared_ptr<st_xmysqlnd_node_session_data>(session_data);
	}
	DBG_RETURN(session_data? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_node_session::connect */
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, connect)(XMYSQLND_NODE_SESSION * session_handle,
                                                MYSQLND_CSTRING database,
                                                const unsigned int port,
                                                const size_t set_capabilities)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session),
										   connect);
	enum_func_status ret{FAIL};
        XMYSQLND_NODE_SESSION_DATA session = session_handle->data;

	DBG_ENTER("xmysqlnd_node_session::connect");

	if (PASS == session->m->local_tx_start(session, this_func)) {
		ret = session->m->connect(session,
					database,port, set_capabilities);
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

} // anonymous namespace

/* {{{ Uuid_format::Uuid_format */
Uuid_format::Uuid_format() :
	clock_seq{ 0 },
	time_hi_and_version{ 0 },
	time_mid{ 0 },
	time_low{ 0 }
{
	node_id.fill( 0 );
}
/* }}} */


/* {{{ Uuid_format::get_uuid */
Uuid_format::uuid_t Uuid_format::get_uuid()
{
	using uchar = unsigned char;
	const std::array< unsigned char, sizeof( Uuid_format ) > raw_uuid
				{{
					//node id
					node_id.at( 0 ), node_id.at( 1 ), node_id.at( 2 ),
					node_id.at( 3 ), node_id.at( 4 ), node_id.at( 5 ),
					//clock seq
					(uchar)(( clock_seq >> 8 ) & 0xFF),
					(uchar)(clock_seq & 0xFF),
					//time_hi_and_version
					(uchar)(( time_hi_and_version >> 8 ) & 0xFF),
					(uchar)(time_hi_and_version & 0xFF),
					//time_mid
					(uchar)(( time_mid >> 8 ) & 0xFF),
					(uchar)(time_mid & 0xFF),
					//time_low
					(uchar)(( time_low >> 24 ) & 0xFF),
					(uchar)(( time_low >> 16 ) & 0xFF),
					(uchar)(( time_low >> 8 ) & 0xFF),
					(uchar)(time_low & 0xFF)
				}}
				;
	static const char hex[] = "0123456789ABCDEF";
	Uuid_format::uuid_t uuid;
	uuid.fill( 0 );
	for( size_t i{ 0 }; i < raw_uuid.size() ; ++i ) {
		uuid[ i * 2 ] = hex[ raw_uuid[ i ] >> 4 ];
		uuid[ i * 2 + 1 ] = hex[ raw_uuid[ i ] & 0xF ];
	}
	return uuid;
}
/* }}} */


/* {{{ Uuid_generator::Uuid_generator */
Uuid_generator::Uuid_generator() :
		last_timestamp{ 0 }
{
	generate_session_node_info();
}
/* }}} */


/* {{{ Uuid_generator::generate */
Uuid_format::uuid_t Uuid_generator::generate()
{
	Uuid_format uuid;
	assign_node_id( uuid );
	assign_timestamp( uuid );
	return uuid.get_uuid();
}
/* }}} */


/* {{{ Uuid_generator::generate_session_node_info */
void Uuid_generator::generate_session_node_info()
{
	std::random_device rd;
	std::seed_seq seed{rd(), rd(),
				rd(), rd(), rd(),
				rd(), rd(), rd()};
	std::mt19937 eng( seed );
	std::uniform_int_distribution<uint64_t> dist( (uint64_t)1 << 48,
				std::numeric_limits< uint64_t >::max() );

	uint64_t random_id{ dist( eng ) };
	for( int i{ 0 } ; i < UUID_NODE_ID_SIZE; ++i ) {
		session_node_id[ i ] = random_id & 0xFF;
		random_id >>= 8;
	}

	clock_sequence = dist( eng ) & 0xFFFF;
}
/* }}} */


/* {{{ Uuid_generator::assign_node_id */
void Uuid_generator::assign_node_id( Uuid_format &uuid )
{
	uuid.node_id = session_node_id;
}
/* }}} */


/* {{{ Uuid_generator::assign_timestamp */
void Uuid_generator::assign_timestamp( Uuid_format& uuid )
{
	/*
	 * from http://www.ietf.org/rfc/rfc4122.txt:
	 *
	 * The timestamp is a 60-bit value.  For UUID version 1, this is
	 * represented by Coordinated Universal Time (UTC) as a count of 100-
	 * nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of
	 * Gregorian reform to the Christian calendar).
	 *
	 * std::chrono use as epoch date: Wed Dec 31 19:00:00 1969,
	 * we need to account this while calculating the number of 100-nanosecond
	 * intervals
	 */
	static uint64_t timestamp_epoch_offset = (
				(uint64_t)141427 * 24 * 60 * 60 * 1000 * 1000 * 10);
	auto time_point = std::chrono::high_resolution_clock::now();
	uint64_t nsec = std::chrono::duration_cast< std::chrono::nanoseconds >(
				time_point.time_since_epoch() ).count() / 100;
	nsec -= timestamp_epoch_offset;

	if( last_timestamp >= nsec ) {
		/*
		 * Possibly the system clock has been changed or
		 * two consecutive request for the timestamp were issued
		 * at a very close distance in time.
		 *
		 * to avoid duplicated UUID rengenerate the unique
		 * node session ID
		 */
		generate_session_node_info();
	}

	/*
	 * Assign the values to Uuid_format
	 */
	uuid.time_low = (uint32_t)(nsec & 0xFFFFFFFF);
	uuid.time_mid = (uint16_t)((nsec >> 32) & 0xFFFF);
	uuid.time_hi_and_version = (uint16_t)((nsec >> 48) | UUID_VERSION);
	uuid.clock_seq = clock_sequence;

	last_timestamp = nsec;
}
/* }}} */

namespace {

/* {{{ xmysqlnd_schema_operation */
enum_func_status
xmysqlnd_schema_operation(XMYSQLND_NODE_SESSION * session_handle, const MYSQLND_CSTRING operation, const MYSQLND_CSTRING db)
{
	enum_func_status ret{FAIL};
	const MYSQLND_STRING quoted_db = session_handle->data->m->quote_name(session_handle->data, db);
	DBG_ENTER("xmysqlnd_schema_operation");
	DBG_INF_FMT("db=%s", db);

	if (quoted_db.s && quoted_db.l) {
		util::string query(operation.s, operation.l);
		query.append(quoted_db.s, quoted_db.l);
		const MYSQLND_CSTRING select_query = { query.c_str(), query.length() };
		mnd_efree(quoted_db.s);

		ret = session_handle->m->query(session_handle, namespace_sql, select_query, noop__var_binder);
	}
	DBG_RETURN(ret);

}
/* }}} */


/* {{{ xmysqlnd_node_session::select_db */
const enum_func_status
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
const enum_func_status
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
const enum_func_status
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
const enum_hnd_func_status
query_cb_handler_on_result_start(void * context, XMYSQLND_NODE_STMT * const stmt)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_result_start");
	if (ctx && ctx->session && ctx->handler_on_result_start.handler) {
		ret = ctx->handler_on_result_start.handler(ctx->handler_on_result_start.ctx, ctx->session, stmt);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_row */
const enum_hnd_func_status
query_cb_handler_on_row(void * context,
						XMYSQLND_NODE_STMT * const stmt,
						const st_xmysqlnd_node_stmt_result_meta* const meta,
						const zval * const row,
						MYSQLND_STATS * const stats,
						MYSQLND_ERROR_INFO * const error_info)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_row");
	if (ctx && ctx->session && ctx->handler_on_row.handler && row) {
		ret = ctx->handler_on_row.handler(ctx->handler_on_row.ctx, ctx->session, stmt, meta, row, stats, error_info);
	}
	ret = HND_AGAIN; // for now we don't allow fetching to be suspended and continued later
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_warning */
const enum_hnd_func_status
query_cb_handler_on_warning(void * context,
							XMYSQLND_NODE_STMT * const stmt,
							const enum xmysqlnd_stmt_warning_level level,
							const unsigned int code,
							const MYSQLND_CSTRING message)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_warning");
	if (ctx && ctx->session && ctx->handler_on_warning.handler) {
		ret = ctx->handler_on_warning.handler(ctx->handler_on_warning.ctx, ctx->session, stmt, level, code, message);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_error */
const enum_hnd_func_status
query_cb_handler_on_error(void * context,
						  XMYSQLND_NODE_STMT * const stmt,
						  const unsigned int code,
						  const MYSQLND_CSTRING sql_state,
						  const MYSQLND_CSTRING message)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_error");
	if (ctx && ctx->session && ctx->handler_on_error.handler) {
		ret = ctx->handler_on_error.handler(ctx->handler_on_error.ctx, ctx->session, stmt, code, sql_state, message);
	}
	ret = HND_PASS_RETURN_FAIL;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_result_end */
const enum_hnd_func_status
query_cb_handler_on_result_end(void * context, XMYSQLND_NODE_STMT * const stmt, const zend_bool has_more)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_result_end");
	if (ctx && ctx->session && ctx->handler_on_result_end.handler) {
		ret = ctx->handler_on_result_end.handler(ctx->handler_on_result_end.ctx, ctx->session, stmt, has_more);
	}
	ret = HND_AGAIN;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ query_cb_handler_on_statement_ok */
const enum_hnd_func_status
query_cb_handler_on_statement_ok(void * context, XMYSQLND_NODE_STMT * const stmt, const st_xmysqlnd_stmt_execution_state* const exec_state)
{
	enum_hnd_func_status ret;
	const st_xmysqlnd_query_cb_ctx* ctx = (const st_xmysqlnd_query_cb_ctx* ) context;
	DBG_ENTER("query_cb_handler_on_result_end");
	if (ctx && ctx->session && ctx->handler_on_statement_ok.handler) {
		ret = ctx->handler_on_statement_ok.handler(ctx->handler_on_statement_ok.ctx, ctx->session, stmt, exec_state);
	}
	ret = HND_PASS;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query_cb */
const enum_func_status
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
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_node_session::query_cb");
	if (session_handle) {
                XMYSQLND_NODE_SESSION_DATA session = session_handle->data;
		XMYSQLND_NODE_STMT * const stmt = session_handle->m->create_statement_object(session_handle);
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
		if (stmt && stmt_execute) {
			ret = PASS;
			if (var_binder.handler) {
				zend_bool loop{TRUE};
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
                                (PASS == (ret = stmt->data->m.send_raw_message(stmt,
                                                xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute),
                                                session->stats, session->error_info))))
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
					handler_on_row.handler? query_cb_handler_on_row : nullptr,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = {
					handler_on_warning.handler? query_cb_handler_on_warning : nullptr,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_error_bind on_error = {
					handler_on_error.handler? query_cb_handler_on_error : nullptr,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_result_start_bind on_result_start = {
					handler_on_result_start.handler? query_cb_handler_on_result_start : nullptr,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_result_end_bind on_result_end = {
					handler_on_result_end.handler? query_cb_handler_on_result_end : nullptr,
					&query_cb_ctx
				};
				const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok = {
					handler_on_statement_ok.handler? query_cb_handler_on_statement_ok : nullptr,
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
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, query_cb_ex)(XMYSQLND_NODE_SESSION * session_handle,
													const MYSQLND_CSTRING namespace_,
													st_xmysqlnd_query_builder* query_builder,
													const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder,
													const struct st_xmysqlnd_node_session_on_result_start_bind handler_on_result_start,
													const struct st_xmysqlnd_node_session_on_row_bind handler_on_row,
													const struct st_xmysqlnd_node_session_on_warning_bind handler_on_warning,
													const struct st_xmysqlnd_node_session_on_error_bind handler_on_error,
													const struct st_xmysqlnd_node_session_on_result_end_bind handler_on_result_end,
													const struct st_xmysqlnd_node_session_on_statement_ok_bind handler_on_statement_ok)
{
	enum_func_status ret{FAIL};
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
const enum_hnd_func_status
xmysqlnd_node_session_on_warning(void * context, XMYSQLND_NODE_STMT * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message)
{
	DBG_ENTER("xmysqlnd_node_session_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.l, message.s);
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_session::query */
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, query)(XMYSQLND_NODE_SESSION * session_handle,
											  const MYSQLND_CSTRING namespace_,
											  const MYSQLND_CSTRING query,
											  const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), query);
        XMYSQLND_NODE_SESSION_DATA session = session_handle->data;
	enum_func_status ret{FAIL};

	DBG_ENTER("xmysqlnd_node_session::query");
	if (PASS == session->m->local_tx_start(session, this_func)) {
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
		XMYSQLND_NODE_STMT * stmt = session_handle->m->create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			ret = PASS;
			if (var_binder.handler) {
				zend_bool loop{TRUE};
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
					const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = { xmysqlnd_node_session_on_warning, nullptr };
					const struct st_xmysqlnd_node_stmt_on_error_bind on_error = { nullptr, nullptr };
					zend_bool has_more{FALSE};
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
zend_ulong
XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version)(XMYSQLND_NODE_SESSION * const session_handle)
{
        XMYSQLND_NODE_SESSION_DATA session = session_handle->data;
	zend_long major, minor, patch;
	char *p;
	DBG_ENTER("xmysqlnd_node_session::get_server_version");
	if (!(p = session_handle->server_version_string)) {
		const MYSQLND_CSTRING query = { "SELECT VERSION()", sizeof("SELECT VERSION()") - 1 };
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_sql, query);
		XMYSQLND_NODE_STMT * stmt = session_handle->m->create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			if (PASS == stmt->data->m.send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), session->stats, session->error_info)) {
				const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning = { nullptr, nullptr };
				const struct st_xmysqlnd_node_stmt_on_error_bind on_error = { nullptr, nullptr };
				zend_bool has_more{FALSE};
				XMYSQLND_NODE_STMT_RESULT * res = stmt->data->m.get_buffered_result(stmt, &has_more, on_warning, on_error, session->stats, session->error_info);
				if (res) {
					zval* set{nullptr};
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
const char *
XMYSQLND_METHOD(xmysqlnd_node_session, get_server_version_string)(const XMYSQLND_NODE_SESSION * const session_handle)
{
	return session_handle->server_version_string;
}
/* }}} */


/* {{{ xmysqlnd_node_session::create_statement_object */
XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_session, create_statement_object)(XMYSQLND_NODE_SESSION * const session_handle)
{
	XMYSQLND_NODE_STMT* stmt{nullptr};
	DBG_ENTER("xmysqlnd_node_session_data::create_statement_object");

	stmt = xmysqlnd_node_stmt_create(session_handle, session_handle->persistent, session_handle->data->object_factory, session_handle->data->stats, session_handle->data->error_info);
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_session::create_schema_object */
XMYSQLND_NODE_SCHEMA *
XMYSQLND_METHOD(xmysqlnd_node_session, create_schema_object)(XMYSQLND_NODE_SESSION * const session_handle, const MYSQLND_CSTRING schema_name)
{
	XMYSQLND_NODE_SCHEMA* schema{nullptr};
	DBG_ENTER("xmysqlnd_node_session::create_schema_object");
	DBG_INF_FMT("schema_name=%s", schema_name.s);

	schema = xmysqlnd_node_schema_create(session_handle, schema_name, session_handle->persistent, session_handle->data->object_factory, session_handle->data->stats, session_handle->data->error_info);

	DBG_RETURN(schema);
}
/* }}} */


/* {{{ xmysqlnd_node_session::close */
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, close)(XMYSQLND_NODE_SESSION * session_handle, const enum_xmysqlnd_node_session_close_type close_type)
{
	const size_t this_func = STRUCT_OFFSET(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session), close);
        XMYSQLND_NODE_SESSION_DATA session = session_handle->data;
	enum_func_status ret{FAIL};

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
XMYSQLND_NODE_SESSION *
XMYSQLND_METHOD(xmysqlnd_node_session, get_reference)(XMYSQLND_NODE_SESSION * const session)
{
	DBG_ENTER("xmysqlnd_node_session::get_reference");
	++session->refcount;
        DBG_INF_FMT("session=%p new_refcount=%u", session, session->refcount);
	DBG_RETURN(session);
}
/* }}} */


/* {{{ xmysqlnd_node_session::free_reference */
const enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_session, free_reference)(XMYSQLND_NODE_SESSION * const session)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_node_session::free_reference");
	DBG_INF_FMT("session=%p old_refcount=%u", session, session->refcount);
	if (!(--session->refcount)) {
		session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
		session->m->dtor(session);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_session::free_contents */
void
XMYSQLND_METHOD(xmysqlnd_node_session, free_contents)(XMYSQLND_NODE_SESSION * session_handle)
{
	zend_bool pers = session_handle->persistent;

	DBG_ENTER("xmysqlnd_node_session::free_contents");

	DBG_INF("Freeing memory of members");

	if (session_handle->server_version_string) {
		mnd_pefree(session_handle->server_version_string, pers);
		session_handle->server_version_string = nullptr;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_session::dtor */
void
XMYSQLND_METHOD(xmysqlnd_node_session, dtor)(XMYSQLND_NODE_SESSION * session_handle)
{
	DBG_ENTER("xmysqlnd_node_session::dtor");
	session_handle->m->free_contents(session_handle);
	if (session_handle->data) {
		session_handle->data->m->free_reference(session_handle->data);
		session_handle->data = nullptr;
	}
	if(session_handle->session_uuid) {
		delete session_handle->session_uuid;
	}
	mnd_pefree(session_handle, session_handle->persistent);
	DBG_VOID_RETURN;
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_node_session)
	XMYSQLND_METHOD(xmysqlnd_node_session, init),
	XMYSQLND_METHOD(xmysqlnd_node_session, connect),
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
	XMYSQLND_METHOD(xmysqlnd_node_session, close),
	XMYSQLND_METHOD(xmysqlnd_node_session, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_session, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_session, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_session, dtor),
MYSQLND_CLASS_METHODS_END;

} // anonymous namespace

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
								XMYSQLND_SESSION_AUTH_DATA * auth,
								const MYSQLND_CSTRING database,
								unsigned int port,
								const size_t set_capabilities)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory =
			MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	enum_func_status ret{FAIL};
	zend_bool self_allocated{FALSE};
	const size_t client_api_flags{0}; //This is not used at the moment..
	/* may need to pass these from outside */
	MYSQLND_STATS* stats{nullptr};
	MYSQLND_ERROR_INFO* error_info{nullptr};

	DBG_ENTER("xmysqlnd_node_session_connect");
	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%llu",
			auth->hostname.c_str(), auth->username.c_str(), database.s,
			port, static_cast<unsigned long long>(set_capabilities));

	if (!session) {
		self_allocated = TRUE;
		if (!(session = xmysqlnd_node_session_create(client_api_flags,
										FALSE, factory,
										stats, error_info))) {
			/* OOM */
			DBG_RETURN(nullptr);
		}
	}
	session->data->auth = auth;
	ret = session->m->connect(session,database,
							port, set_capabilities);

	if (ret == FAIL) {
		if (self_allocated) {
			/*
			  We have allocated, thus there are no references to this
			  object - we are free to kill it!
			*/
			session->m->dtor(session);
		}
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(session);
}
/* }}} */

namespace {

/* {{{ create_new_session */
struct mysqlx::devapi::st_mysqlx_session * create_new_session(zval * session_zval)
{
	DBG_ENTER("create_new_session");
	struct mysqlx::devapi::st_mysqlx_session* object{nullptr};
	if (PASS == mysqlx::devapi::mysqlx_new_node_session(session_zval)) {
		object = (struct mysqlx::devapi::st_mysqlx_session *) Z_MYSQLX_P(session_zval)->ptr;

		if (!object && !object->session) {
			if (object->closed) {
				php_error_docref(nullptr, E_WARNING, "closed session");
			} else {
				php_error_docref(nullptr, E_WARNING, "invalid object of class %s",
								 ZSTR_VAL(Z_MYSQLX_P(session_zval)->zo.ce->name)); \
			}
		}
	} else {
		zval_ptr_dtor(session_zval);
		ZVAL_NULL(session_zval);
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ establish_connection */
enum_func_status establish_connection(struct mysqlx::devapi::st_mysqlx_session * object,
								XMYSQLND_SESSION_AUTH_DATA * auth,
								const util::Url& url,
								transport_types tr_type)
{
	DBG_ENTER("establish_connection");
	enum_func_status ret{PASS};
	mysqlx::drv::XMYSQLND_NODE_SESSION * new_session;
	size_t set_capabilities{0};
	if( tr_type != transport_types::network ) {
		DBG_INF_FMT("Connecting with the provided socket/pipe: %s",
					url.host.c_str());
		if( url.host.empty() ) {
			//This should never happen!
			DBG_ERR_FMT("Expecting socket/pipe location, found nothing!");
			ret = FAIL;
		} else {
			object->session->data->socket_path = url.host;
		}
	}

	if( ret != FAIL ) {
		const MYSQLND_CSTRING path = { url.path.c_str(), url.path.length() };
		object->session->data->transport_type = tr_type;
		new_session = xmysqlnd_node_session_connect(object->session,
									auth,
									path,
									url.port,
									set_capabilities);
		if(new_session == nullptr) {
			ret = FAIL;
		}

		if (ret == PASS && object->session != new_session) {
			if (new_session) {
				php_error_docref(nullptr, E_WARNING, "Different object returned");
			}
			object->session = new_session;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ extract_transport
 *
 * Be aware that extract_transport will modify
 * the string argument!
 */
std::pair<util::string, transport_types>
extract_transport(util::string& uri)
{
	util::string transport;
	transport_types tr_type = transport_types::network;
	std::size_t idx = uri.find_last_of('@'),
			not_found = util::string::npos;
	if( idx == not_found ) {
		return { transport, transport_types::none };
	}
	char tr = uri[ ++idx ];
	if( tr == '.' || tr == '/' || tr == '(' || tr == '\\' ) {
		/*
		 * Attempt to use a windows pipe or unix
		 * domain socket.
		 *
		 * the allowed formats are:
		 * ...@(/path/to/sock)
		 * ...@(/path/to/sock)/schema
		 * ...@/path%2Fto%2Fsock
		 * ...@/path%2Fto%2Fsock/schema
		 * ...@..%2Fpath%2Fto%2Fsock
		 *
		 * ..etc..
		 *
		 * Windows pipe's MUST begin with \
		 */
		if( tr == '\\' ) {
			tr_type = transport_types::windows_pipe;
		} else {
			tr_type = transport_types::unix_domain_socket;
		}
		bool double_dot{ uri[ idx ] == '.' && uri[ idx + 1 ] == '.' };
		char exp_term = ( tr == '(' ? ')' : '/' );
		std::size_t end_idx{ uri.size() -1 };
		for( ; end_idx >= idx ; --end_idx ) {
			if( uri[ end_idx ] == exp_term ) {
			break;
			}
		}
		/*
		 * The second and third condition in the next IF is needed
		 * to cover the input URI: @../socket or @../socket/schema
		 * and @./socket or @./socket/schema
		 */
		if( end_idx <= idx ||
				( !double_dot && end_idx == idx + 1 ) ||
				( double_dot && end_idx == idx + 2 ) ) {
			/*
			 * No schema provided or using 2%F,
			 * it might be also a wrong formatted URI,
			 * if that's the case the connection will
			 * fail later.
			 */
			end_idx = uri.size();
		} else if( exp_term == ')' ){
			++end_idx;
		}
		transport = uri.substr( idx , end_idx - idx );
		if( false == transport.empty() ) {
			//Remove the PCT encoding, if any.
			util::string decoded = decode_pct_path( transport );
			//Remove ( and )
			decoded.erase( std::remove_if( decoded.begin(),
								decoded.end(),
								[](const char ch) {
									return ch == '(' || ch == ')';
								}), decoded.end());
			transport = decoded;
		}
		//This is needed, otherwise php_url_parse would
		//not be able to parse the URI
		uri.erase( idx, end_idx - idx);
		/*
		 * We're inserting a string of size transport.size
		 * to force the url_parser to allocate a 'host'
		 * string with that size, we will copy
		 * in that memory the value of 'transport', in this
		 * way we wil move down the flow the socket location
		 * without additional variables.
		 */
		uri.insert( idx, transport.size() + 1 ,'x');
	}

	return { transport, tr_type };
}
/* }}} */


/* {{{ extract_uri_information */
std::pair<util::Url, transport_types> extract_uri_information(const char * uri_string)
{
	DBG_ENTER("extract_uri_information");
	DBG_INF_FMT("URI string: %s\n",uri_string);
	util::string uri = uri_string;
	/*
	 * Check whether there's an attempt to connect
	 * using a unix domain socket or windows pipe.
	 * php_url_parse do not understand URI's with
	 * those information
	 */
	auto transport = extract_transport(uri);
	util::string tr_path = transport.first;
	php_url * raw_node_url = php_url_parse(uri.c_str());
	if( nullptr == raw_node_url ) {
		DBG_ERR_FMT("URI parsing failed!");
		return { util::Url(), transport_types::none };
	}

	util::Url node_url(raw_node_url);
	php_url_free(raw_node_url);
	raw_node_url = nullptr;
	enum_func_status ret{PASS};
	//host is required
	if( node_url.host.empty() ) {
		DBG_ERR_FMT("Missing required host name!");
		ret = FAIL;
	} else if( !tr_path.empty() ) {
		//Copy the transport domain socket location
		node_url.host = tr_path;

		//Make sure the transport type is not 'none' or 'network'
		if( transport.second == transport_types::none ||
			transport.second == transport_types::network ) {
			//Something bad!
			DBG_ERR_FMT("Identified a local transport path, but no proper type selected!");
			return { nullptr, transport_types::none };
		}
		//Set port to ZERO, signaling a non network socket
		node_url.port = 0;
	}
	//Username is required
	if( node_url.user.empty() ) {
		DBG_ERR_FMT("Missing required user name!");
		ret = FAIL;
	}
	//Port required (If no alternative transport provided)
	if( 0 == node_url.port && tr_path.empty() ) {
		DBG_INF_FMT("Missing port number, trying to get from env or set default!");
		node_url.port = drv::Environment::get_as_int(drv::Environment::Variable::Mysqlx_port);
	}
	//Password optional, but print a log
	if( node_url.pass.empty() ) {
		DBG_INF_FMT("no password given, using empty string");
	}
	if( ret == PASS ) {
		DBG_INF_FMT("URI information: host: %s, port: %d,user: %s,pass: %s,path: %s, query: %s\n",
					 node_url.host.c_str(), node_url.port,
					 node_url.user.c_str(), node_url.pass.c_str(),
					 node_url.path.c_str(), node_url.query.c_str());
		if( !tr_path.empty() ) {
			DBG_INF_FMT("Selected local socket: ", tr_path.c_str());
		}
	}
	return { node_url, transport.second };
}
/* }}} */

enum_func_status set_ssl_mode( XMYSQLND_SESSION_AUTH_DATA* auth,
							   SSL_mode mode )
{
	DBG_ENTER("set_ssl_mode");
	enum_func_status ret{PASS};
	if( auth->ssl_mode != SSL_mode::not_specified ) {
		if( auth->ssl_mode != mode ) {
			DBG_ERR_FMT("Selected two incompatible ssl modes");
			devapi::RAISE_EXCEPTION( err_msg_invalid_ssl_mode );
			ret = FAIL;
		}
	} else {
		DBG_INF_FMT("Selected mode: %d",
					static_cast<int>(mode) );
		auth->ssl_mode = mode;
	}
	DBG_RETURN( ret );
}

/* {{{ parse_ssl_mode */
enum_func_status parse_ssl_mode( XMYSQLND_SESSION_AUTH_DATA* auth,
						const util::string& mode )
{
	DBG_ENTER("parse_ssl_mode");
	enum_func_status ret{FAIL};
	using modestr_to_enum = std::map<std::string, SSL_mode, util::iless>;
	static modestr_to_enum mode_mapping = {
		{ "required", SSL_mode::required },
		{ "disabled", SSL_mode::disabled },
		{ "verify_ca", SSL_mode::verify_ca },
		{ "verify_identity", SSL_mode::verify_identity }
	};
	auto it = mode_mapping.find( mode.c_str() );
	if( it != mode_mapping.end() ) {
		ret = set_ssl_mode( auth, it->second );
	} else {
		DBG_ERR_FMT("Unknown SSL mode selector: %s",
					mode.c_str());
	}
	DBG_RETURN( ret );
}
/* }}} */


/* {{{ set_auth_mode */
enum_func_status set_auth_mode(
	XMYSQLND_SESSION_AUTH_DATA* auth,
	Auth_mode auth_mode)
{
	DBG_ENTER("set_auth_mode");
	enum_func_status ret{FAIL};
	if (auth->auth_mode == Auth_mode::unspecified) {
		DBG_INF_FMT("Selected auth mode: %d", static_cast<int>(auth_mode));
		auth->auth_mode = auth_mode;
		ret = PASS;
	} else if (auth->auth_mode != auth_mode) {
		DBG_ERR_FMT("Selected two incompatible auth modes %d vs %d",
			static_cast<int>(auth->auth_mode),
			static_cast<int>(auth_mode));
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_auth_mode);
	}
	DBG_RETURN( ret );
}
/* }}} */

/* {{{ parse_auth_mode */
enum_func_status parse_auth_mode(
	XMYSQLND_SESSION_AUTH_DATA* auth,
	const util::string& auth_mode)
{
	DBG_ENTER("parse_auth_mode");
	enum_func_status ret{FAIL};
	using str_to_auth_mode = std::map<std::string, Auth_mode, util::iless>;
	static const str_to_auth_mode str_to_auth_modes = {
		{ Auth_method_mysql41, Auth_mode::mysql41 },
		{ Auth_method_plain, Auth_mode::plain },
		{ Auth_method_external, Auth_mode::external }
	};
	auto it = str_to_auth_modes.find(auth_mode.c_str());
	if (it != str_to_auth_modes.end()) {
		ret = set_auth_mode(auth, it->second);
	} else {
		devapi::RAISE_EXCEPTION(err_msg_invalid_auth_method);
		ret = FAIL;
		DBG_ERR_FMT("Unknown Auth mode: %s", auth_mode.c_str());
	}
	DBG_RETURN( ret );
}
/* }}} */

/* {{{ extract_ssl_information */
enum_func_status extract_ssl_information(
	const util::string& auth_option_variable,
	const util::string& auth_option_value,
	XMYSQLND_SESSION_AUTH_DATA* auth)
{
	DBG_ENTER("extract_ssl_information");
	enum_func_status ret{PASS};
	using ssl_option_to_data_member = std::map<std::string,
		util::string st_xmysqlnd_session_auth_data::*,
		util::iless>;
	// Map the ssl option to the proper member, according to:
	// https://dev.mysql.com/doc/refman/5.7/en/encrypted-connection-options.html
	static const ssl_option_to_data_member ssl_option_to_data_members = {
		{ "ssl-key", &st_xmysqlnd_session_auth_data::ssl_local_pk },
		{ "ssl-cert",&st_xmysqlnd_session_auth_data::ssl_local_cert },
		{ "ssl-ca", &st_xmysqlnd_session_auth_data::ssl_cafile },
		{ "ssl-capath", &st_xmysqlnd_session_auth_data::ssl_capath },
		{ "ssl-cipher", &st_xmysqlnd_session_auth_data::ssl_ciphers },
		{ "ssl-crl", &st_xmysqlnd_session_auth_data::ssl_crl },
		{ "ssl-crlpath", &st_xmysqlnd_session_auth_data::ssl_crlpath },
		{ "tls-version", &st_xmysqlnd_session_auth_data::tls_version }
	};

	auto it = ssl_option_to_data_members.find(auth_option_variable.c_str());
	if (it != ssl_option_to_data_members.end()) {
		if( auth_option_value.empty() ) {
			DBG_ERR_FMT("The argument to %s shouldn't be empty!",
				auth_option_variable.c_str() );
			php_error_docref(nullptr, E_WARNING, "The argument to %s shouldn't be empty!",
				auth_option_variable.c_str() );
			ret = FAIL;
		} else {
			auth->*(it->second) = auth_option_value;
			/*
				* some SSL options provided, assuming
				* 'required' mode if not specified yet.
				*/
			if( auth->ssl_mode == SSL_mode::not_specified ) {
				ret = set_ssl_mode( auth, SSL_mode::required );
			} else if (auth->ssl_mode == SSL_mode::disabled) {
				/*
					WL#10400 DevAPI: Ensure all Session connections are secure by default
					- if ssl-mode=disabled is used appearance of any ssl option
					such as ssl-ca would result in an error
					- inconsistent options such as  ssl-mode=disabled&ssl-ca=xxxx
					would result in an error returned to the user
				*/
				devapi::RAISE_EXCEPTION(err_msg_inconsistent_ssl_options);
				ret = FAIL;
			}
		}
	} else {
		if (boost::iequals(auth_option_variable, "ssl-mode")) {
			ret = parse_ssl_mode(auth, auth_option_value);
		} else if (boost::iequals(auth_option_variable, "ssl-no-defaults")) {
			auth->ssl_no_defaults = true;
		} else if (boost::iequals(auth_option_variable, "auth")) {
			ret = parse_auth_mode(auth, auth_option_value);
		} else {
			php_error_docref(nullptr,
				E_WARNING,
				"Unknown secure connection option %s.",
				auth_option_variable.c_str());
			ret = FAIL;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ extract_auth_information */
XMYSQLND_SESSION_AUTH_DATA * extract_auth_information(const util::Url& node_url)
{
	DBG_ENTER("extract_auth_information");
	enum_func_status ret{PASS};
	XMYSQLND_SESSION_AUTH_DATA * auth = new XMYSQLND_SESSION_AUTH_DATA;

	if( nullptr == auth ) {
		php_error_docref(nullptr, E_WARNING, "Coulnd't allocate %u bytes",
						sizeof(XMYSQLND_SESSION_AUTH_DATA));
		DBG_RETURN(nullptr);
	}

	if( !node_url.query.empty() ) {
		DBG_INF_FMT("Query string: %s",
					node_url.query.c_str());
		/*
		 * In case of multiple variables with the same
		 * name, extract_ssl_information will pick the value from the last one.
		 * This mean that if the user provide ssl_mode=required&ssl_mode=disabled
		 * the value of SSL mode will be disabled (instead of ERROR)
		 * We need to parse each variable by hand..
		 */
		const util::string& query{ node_url.query };
		util::vector<util::string> auth_data;
		boost::split(auth_data, query, boost::is_any_of("&"));

		for (const auto& auth_option : auth_data) {
			if (auth_option.empty()) {
				ret = FAIL;
				break;
			}

			auto separator_pos = auth_option.find_first_of('=');
			const util::string variable{ auth_option.substr(0, separator_pos) };
			if (variable.empty()) {
				ret = FAIL;
				break;
			}


			const util::string value{
				separator_pos == util::string::npos
					? util::string() : auth_option.substr(separator_pos + 1)
			};


			DBG_INF_FMT("URI query option: %s=%s",
				variable.c_str(),
				value.c_str()
			);

			if( FAIL == extract_ssl_information(variable, value, auth) ) {
				ret = FAIL;
				break;
			}
		}
	}

	if( ret != PASS ){
		delete auth;
		auth = nullptr;
		DBG_RETURN(auth);
	}

	/*
	 * If no SSL mode is selected explicitly then
	 * assume 'required'
	 */
	if( auth->ssl_mode == SSL_mode::not_specified ) {
		DBG_INF_FMT("Setting default SSL mode to REQUIRED");
		ret = set_ssl_mode( auth, SSL_mode::required );
	}


	if( node_url.port != 0 ) {
		//If is 0, then we're using win pipe
		//or unix sockets.
		auth->port = node_url.port;
		auth->hostname = node_url.host;
	}
	auth->username = node_url.user;
	auth->password = node_url.pass;

	DBG_RETURN(auth);
}
/* }}} */

namespace
{

using vec_of_addresses = util::vector< std::pair<util::string,long> >;

/* {{{ list_of_addresses_parser */
class list_of_addresses_parser
{
	void invalidate();
	bool parse_round_token( const util::string& str );
	void add_address(vec_of_addresses::value_type addr);
public:
	list_of_addresses_parser() = default;
	list_of_addresses_parser(util::string uri);
	vec_of_addresses parse();
private:
	enum class cur_bracket {
		none, //Still need to find the first one
		square_bracket,
		round_bracket,
	};

	std::size_t beg{ 0 };
	std::size_t end{ 0 };
	util::string uri_string = {};
	util::string unformatted_uri = {};
	vec_of_addresses list_of_addresses;
};

list_of_addresses_parser::list_of_addresses_parser(util::string uri)
{
	/*
	 * Remove spaces and tabs..
	 */
	uri.erase( std::remove_if(
				   uri.begin(),
				   uri.end(),
				   [](char ch){ return std::isspace( ch ); }),
			   uri.end());
	/*
	 * Initial parse the input URI
	 */
	beg = uri.find_first_of('@');
	if( beg != util::string::npos ) {
		++beg;
	} else {
		/*
		 * Bad formatted URI
		 */
		invalidate();
		return;
	}
	/*
	 * Enable parsing only if this is a list
	 * of addresses
	 */
	bool valid_list{ false };
	/*
	 * Find the first opening [ (start of the list) and
	 * the relative closing ] (end of the list). Verify
	 * if that can possibly be a list.
	 *
	 * (This code is not safe from ill-formatted URI's.. nobody is)
	 */
	int brk_cnt{ uri[ beg ] == '[' };
	for( end = beg + 1 ; brk_cnt > 0 && end < uri.size() ; ++end ) {
		switch( uri[ end ] )
		{
		case '[':
			if( ++brk_cnt > 1 ) {
				//possible only if this is a list
				valid_list = true;
			}
			break;
		case ']':
			if( 0 == --brk_cnt ) {
				//done..
				--end;
			}
			break;
		case ','://Found a list separator
			valid_list = true;
			break;
		case '(':
		case ')':
			valid_list = true;
			break;
		default:
			break;
		}
	}

	if( brk_cnt != 0 ) {
		//Ill-formed URI
		invalidate();
		return;
	}

	if( valid_list ) {
		uri_string = uri;
		/*
		 * The unformatted_uri string is used
		 * to build the proper addresses for each
		 * host provided in the list
		 */
		unformatted_uri = uri_string;
		unformatted_uri.erase( beg , end - beg + 1 );
		/*
		 * Skip the brackets
		 */
		++beg; --end;
	} else {
		/*
		 * Only one address
		 */
		list_of_addresses.push_back({
					uri,
					MAX_HOST_PRIORITY });
		invalidate(); //Signal to 'parse' to actually not parse.
	}
}


vec_of_addresses list_of_addresses_parser::parse()
{
	DBG_ENTER("list_of_addresses_parser::parse");
	bool success{ true };
	std::size_t pos{ beg },
			last_pos{ beg };
	for( std::size_t idx{ beg }; success && idx <= end; ++idx ) {
		assert(idx < uri_string.length());
		if( uri_string[ idx ] == '(' ) {
			pos = uri_string.find_first_of( ')', idx );
			if( pos == util::string::npos ) {
				success = false;
				break;
			}
			success = parse_round_token( uri_string.substr(idx, pos - idx + 1) );
			last_pos = pos + 2;
			idx = pos;
		} else if( ( uri_string[ idx ] == ',' || idx == end ) && last_pos < idx) {
			add_address( { uri_string.substr( last_pos, idx - last_pos + ( idx == end ) ), -1 } );
			last_pos = ++idx;
		}
	}
	if( false == success ) {
		//Nothing more to do here.
		return {};
	}
	/*
	 * Either all -1 (no priority) or every
	 * host shall have its own priority
	 */
	size_t prio_cnt{ 0 };
	for( auto&& elem : list_of_addresses ) {
		if( elem.second >= 0 ) {
			++prio_cnt;
		}
	}
	if( prio_cnt != 0 &&
		prio_cnt != list_of_addresses.size() ) {
		devapi::RAISE_EXCEPTION( err_msg_invalid_prio_assignment );
		list_of_addresses.clear();
	} else {
		/*
		 * Setup default priorities if they were
		 * not provided
		 */
		if( prio_cnt == 0 ) {
			int current_prio{ MAX_HOST_PRIORITY };
			for( auto&& elem : list_of_addresses ) {
				elem.second = std::max( current_prio--, 0 );
			}
		} else {
			/*
			 * The priority define the order
			 * of connections and failovers
			 */
			std::sort( list_of_addresses.begin(),
					   list_of_addresses.end(),
					   [](const vec_of_addresses::value_type& elem_1,
					   const vec_of_addresses::value_type& elem_2) {
				return elem_1.second > elem_2.second;
			});
		}
	}
	DBG_RETURN( list_of_addresses );
}

void list_of_addresses_parser::invalidate()
{
	//Signal to 'parse' to actually not parse.
	beg = 1;
	end = 0;
}

bool list_of_addresses_parser::parse_round_token(const util::string &str)
{
	/*
	 * Assuming no ill-formed input, round squares are
	 * allowed only when addresses and priorities are provided:
	 *
	 * addresspriority  ::= "(address=" address ", priority=" prio ")"
	 */
	static const std::string address = "address";
	static const std::string priority = "priority";
	auto addr_beg = str.find(address.c_str()),
			prio_beg = str.find(priority.c_str());
	if( addr_beg == util::string::npos ||
		prio_beg == util::string::npos ||
		prio_beg < addr_beg ) {
		//Ill-formed input
		return false;
	}
	addr_beg += address.size();
	prio_beg += priority.size();
	/*
	 * Extract address and priority, with very minium error
	 * checking (ill-formed input string)
	 */
	std::size_t idx = addr_beg;
	std::size_t ss_pos[2] = {0,0};
	util::string output[2];
	for(int i{ 0 } ; i < 2; ++i ) {
		ss_pos[0] = 0;
		ss_pos[1] = 0;
		for( ; idx < str.size(); ++idx ) {
			if( str[ idx ] == '=' ) {
				if( ss_pos[0] != 0 ) {
					return false;
				}
				ss_pos[0] = idx + 1;
			} else if( str[ idx ] == ',' ||
					   (i == 1 && str[ idx ] == ')') ) {
				if( ss_pos[1] != 0 ) {
					return false;
				}
				ss_pos[1] = idx;
				break;
			}
		}
		if( ( i == 0 && idx >= prio_beg ) ||
			ss_pos[0] > ss_pos[1] ) {
			return false;
		}
		//Trim the tabs and spaces
		for( auto&& ch : str.substr( ss_pos[0], ss_pos[1] - ss_pos[0]) ) {
			if( ch != ' ' && ch != '\t' ) {
				output[i] += ch;
			}
		}
		idx = prio_beg;
	}

	vec_of_addresses::value_type new_addr = { output[0], std::atoi( output[1].c_str() ) };
	/*
	 * Allowed priorities between 0 and 100 inclusive
	 */
	if( new_addr.second < 0 || new_addr.second > MAX_HOST_PRIORITY ) {
		devapi::RAISE_EXCEPTION( err_msg_prio_values_not_inrange );
		return false;
	}
	add_address(new_addr);
	return true;
}


void list_of_addresses_parser::add_address( vec_of_addresses::value_type addr )
{
	/*
	 * Prepare the correct format for the address
	 * before pushing
	 */
	auto new_addr = unformatted_uri;
	new_addr.insert( beg - 1 , addr.first );
	list_of_addresses.push_back( { new_addr, addr.second } );
}

}
/* }}} */

/* {{{ extract_uri_addresses */
vec_of_addresses extract_uri_addresses(const util::string& uri)
{
	/*
	 * The URI string might contain a list of alternative
	 * address to routers which are provided to
	 * implement the client side failover feature
	 *
	 * The function a vector with the proper URI for
	 * each provided address, those URI are sorted
	 * on the base of their priority
	 */

	std::size_t idx = uri.find_last_of('@'),
			not_found = util::string::npos,
			len = uri.size();
	if( idx == not_found || ( len - idx <= 2) ) {
		//Ill formed URI
		devapi::RAISE_EXCEPTION(err_msg_uri_string_fail);
		return {};
	}

	list_of_addresses_parser parser( uri );
	return parser.parse();
}
/* }}} */

} // anonymous namespace


/* {{{ xmysqlnd_node_new_session_connect */
PHP_MYSQL_XDEVAPI_API
enum_func_status xmysqlnd_node_new_session_connect(const char* uri_string,
							zval * return_value)
{
	DBG_ENTER("xmysqlnd_node_new_session_connect");
	DBG_INF_FMT("URI: %s",uri_string);
	enum_func_status ret{FAIL};
	if( nullptr == uri_string ) {
		DBG_ERR_FMT("The provided URI string is null!");
		return ret;
	}
	/*
	 * Verify whether a list of addresses is provided,
	 * if that's the case we need to parse those addresses
	 * and their priority in order to implement the
	 * Client Side Failover
	 */
	auto uris = extract_uri_addresses( uri_string );
	/*
	 * For each address attempt to perform a connection
	 * (The addresses are sorted by priority)
	 */
	for( auto&& current_uri : uris ) {
		DBG_INF_FMT("Attempting to connect with: %s\n",
					current_uri.first.c_str());

		auto url = extract_uri_information( current_uri.first.c_str() );
		if( !url.first.empty() ) {
			mysqlx::devapi::st_mysqlx_session * session = create_new_session(return_value);
			if( nullptr == session ) {
				devapi::RAISE_EXCEPTION( err_msg_internal_error );
				DBG_RETURN(ret);
			}
			XMYSQLND_SESSION_AUTH_DATA * auth = extract_auth_information(url.first);
			if( nullptr != auth ) {
				/*
				 * If Unix sockets are used then TLS connections
				 * are not allowed either implicitly nor explicitly
				 */
				if( auth->ssl_mode != SSL_mode::disabled &&
					url.second == transport_types::unix_domain_socket ) {
					DBG_ERR_FMT("Connection aborted, TLS not supported for Unix sockets!");
					devapi::RAISE_EXCEPTION( err_msg_tls_not_supported_1 );
					DBG_RETURN(FAIL);
				}
				else
				{
					DBG_INF_FMT("Attempting to connect...");
					ret = establish_connection(session,auth,
									url.first,url.second);
				}
			} else {
				DBG_ERR_FMT("Connection aborted, not able to setup the 'auth' structure.");
			}
		} else {
			devapi::RAISE_EXCEPTION( err_msg_uri_string_fail );
			DBG_RETURN(FAIL);
		}
		if( ret == PASS ) {
			//Ok, connection accepted with this host.
			break;
		}
	}
	if( uris.size() > 1 && ret == FAIL ) {
		devapi::RAISE_EXCEPTION( err_msg_all_routers_failed );
	}
	/*
	 * Do not worry about the allocated auth if
	 * the connection fails, the dtor of 'session'
	 * will clean it up
	 */
	if( FAIL == ret ) {
		zval_dtor(return_value);
		ZVAL_NULL(return_value);
	}
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
