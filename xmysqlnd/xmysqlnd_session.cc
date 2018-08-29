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
#include "mysqlnd_api.h"
extern "C" {
#undef L64
#include <ext/hash/php_hash.h>
#include <ext/hash/php_hash_sha.h> // PHP_SHA256 functions
}
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h"
#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_environment.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_schema.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_protocol_dumper.h"
#include "xmysqlnd_stmt_result.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_structs.h"
#include "php_mysqlx.h"
#include "xmysqlnd_utils.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "SAPI.h"
#include "php_variables.h"
#include "mysqlx_exception.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/url_utils.h"
#include "util/zend_utils.h"
#include <utility>
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>
#include <memory>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

namespace mysqlx {

namespace drv {

const std::vector<std::string> xmysqlnd_session_auth_data::supported_ciphers = {
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

namespace {

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

} // anonymous namespace

/* {{{ set_connection_timeout */
bool set_connection_timeout(
	const boost::optional<int>& connection_timeout,
	MYSQLND_VIO* vio)
{
	const int Dont_set_connection_timeout{ 0 };
	int timeout{ Dont_set_connection_timeout };
	if (connection_timeout) {
		timeout = connection_timeout.get();
	} else {
		timeout = drv::Environment::get_as_int(
			drv::Environment::Variable::Mysqlx_connection_timeout);
	}

	if (timeout == Dont_set_connection_timeout) return true;

	if (timeout < 0) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_timeout);
	}

	st_mysqlnd_vio_options& vio_options = vio->data->options;
	vio_options.timeout_connect = static_cast<unsigned int>(timeout);
	return true;
}
/* }}} */

namespace {

/* {{{ set_connection_options */
bool set_connection_options(
	const xmysqlnd_session_auth_data* auth_data,
	MYSQLND_VIO* vio)
{
	auto& connection_timeout = auth_data->connection_timeout;
	return set_connection_timeout(connection_timeout, vio);
}
/* }}} */

} // anonymous namespace

xmysqlnd_session_auth_data::xmysqlnd_session_auth_data() :
	port{ 0 },
	ssl_mode{ SSL_mode::not_specified },
	ssl_enabled{ false },
	ssl_no_defaults{ true } {
}

/* {{{ xmysqlnd_session_data::get_scheme */
MYSQLND_STRING
xmysqlnd_session_data::get_scheme(
		const util::string& hostname,
		unsigned int port)
{
	MYSQLND_STRING transport{nullptr, 0};
	DBG_ENTER("xmysqlnd_session_data::get_scheme");
	/* MY-305: Add support for windows pipe */
	if( transport_type == transport_types::network ) {
		if (!port) {
			port = drv::Environment::get_as_int(drv::Environment::Variable::Mysql_port);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "tcp://%s:%u", hostname.c_str(), port);
	} else if( transport_type == transport_types::unix_domain_socket ){
		transport.l = mnd_sprintf(&transport.s, 0, "unix://%s",
								  socket_path.c_str());
	} else if( transport_type == transport_types::windows_pipe ) {
#ifdef PHP_WIN32
		/* Somewhere here?! (This is old code) */
		if (hostname == ".") {
			/* named pipe in socket */
			socket_path = "\\\\.\\pipe\\MySQL";
		}
		transport.l = mnd_sprintf(&transport.s, 0, "pipe://%s", socket_path.c_str());
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

/* {{{ xmysqlnd_session_data::connect_handshake */
enum_func_status
xmysqlnd_session_data::connect_handshake(const MYSQLND_CSTRING scheme_name,
											const MYSQLND_CSTRING database,
											const size_t set_capabilities)
{
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_session_data::connect_handshake");

	if (set_connection_options(auth, io.vio)
		&& (PASS == io.vio->data->m.connect(io.vio,
											scheme_name,
											persistent,
											stats,
											error_info))
		&& (PASS == io.pfc->data->m.reset(io.pfc,
										  stats,
										  error_info))) {
		state.set(SESSION_NON_AUTHENTICATED);
		ret = authenticate(scheme_name, database, set_capabilities);
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_session_data::authenticate */
enum_func_status
xmysqlnd_session_data::authenticate(const MYSQLND_CSTRING scheme_name,
									   const MYSQLND_CSTRING database,
									   const size_t /*set_capabilities*/)
{
	DBG_ENTER("xmysqlnd_session_data::authenticate");
	Authenticate authenticate(this, scheme_name, database);
	enum_func_status ret{FAIL};
	if (authenticate.run()) {
		DBG_INF("AUTHENTICATED. YAY!");
		ret = PASS;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session_data::connect */
enum_func_status
xmysqlnd_session_data::connect(
		MYSQLND_CSTRING database,
		unsigned int port,
		size_t set_capabilities)
{
	zend_bool reconnect{FALSE};
	MYSQLND_STRING transport_name = { nullptr, 0 };
	enum_func_status ret{PASS};

	DBG_ENTER("xmysqlnd_session_data::connect");

	SET_EMPTY_ERROR(error_info);

	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%u persistent=%u state=%u",
				!auth->hostname.empty()?auth->hostname.c_str():"",
				!auth->username.empty()?auth->username.c_str():"",
				database.s?database.s:"", port, (uint) set_capabilities,
				persistent,
				state.get());

	if (state.get() > SESSION_ALLOCATED) {
		DBG_INF("Connecting on a connected handle.");

		if (state.get() < SESSION_CLOSE_SENT) {
			XMYSQLND_INC_SESSION_STATISTIC(stats, XMYSQLND_STAT_CLOSE_IMPLICIT);
			reconnect = TRUE;
			send_close();
		}

		cleanup();
	}

	/* Setup the relevant variables! */
	current_db.l = database.l;
	current_db.s = mnd_pestrndup(database.s, current_db.l, 0);

	transport_name = get_scheme(auth->hostname,
						   port);

	if( nullptr == transport_name.s || transport_name.l == 0 ) {
		ret = FAIL;
	} else {
		scheme.s = mnd_pestrndup(transport_name.s, transport_name.l, 0);
		scheme.l = transport_name.l;

		mnd_sprintf_free(transport_name.s);
		transport_name.s = nullptr;

		if (!scheme.s || !current_db.s) {
			SET_OOM_ERROR(error_info);
			ret = FAIL;
		}
	}

	/* Attempt to connect */
	if( ret == PASS ) {
		const MYSQLND_CSTRING local_scheme = { scheme.s, scheme.l };
		ret = connect_handshake( local_scheme, database,
								 set_capabilities);
		if( (ret != PASS) && (error_info->error_no == 0)) {
			SET_OOM_ERROR(error_info);
		}
	}

	/* Setup server host information */
	if( ret == PASS ) {
		state.set(SESSION_READY);
		transport_types transport = transport_type;

		if ( transport == transport_types::network ) {
			server_host_info = build_server_host_info("%s via TCP/IP",
													  auth->hostname.c_str(),
													  persistent);
		} else if( transport == transport_types::unix_domain_socket ) {
			server_host_info = mnd_pestrdup("Localhost via UNIX socket", 0);
		} else if( transport == transport_types::windows_pipe) {
			server_host_info = build_server_host_info("%s via named pipe",
													  socket_path.c_str(),
													  0);
		}

		if ( !server_host_info ) {
			SET_OOM_ERROR(error_info);
			ret = FAIL;
		}
	}

	/* Done, setup remaining information */
	if( ret == PASS ) {
		SET_EMPTY_ERROR(error_info);

		XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(stats,
												XMYSQLND_STAT_CONNECT_SUCCESS,
												1,
												XMYSQLND_STAT_OPENED_CONNECTIONS,
												1);
		if (reconnect) {
			XMYSQLND_INC_GLOBAL_STATISTIC(XMYSQLND_STAT_RECONNECT);
		}
		if (persistent) {
			XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(stats,
													XMYSQLND_STAT_PCONNECT_SUCCESS,
													1,
													XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS,
													1);
		}

	} else {
		DBG_ERR_FMT("[%u] %.128s (trying to connect via %s)",
					error_info->error_no,
					error_info->error,
					scheme.s);

		if (!error_info->error_no) {
			SET_CLIENT_ERROR(error_info,
							 CR_CONNECTION_ERROR,
							 UNKNOWN_SQLSTATE,
							 error_info->error[0] ? error_info->error:"Unknown error");
			php_error_docref(nullptr, E_WARNING, "[%u] %.128s (trying to connect via %s)",
							 error_info->error_no, error_info->error, scheme.s);
		}
		cleanup();
		XMYSQLND_INC_SESSION_STATISTIC(stats, XMYSQLND_STAT_CONNECT_FAILURE);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session_data::quote_name */
MYSQLND_STRING
xmysqlnd_session_data::quote_name(const MYSQLND_CSTRING name)
{
	MYSQLND_STRING ret = { nullptr, 0 };
	DBG_ENTER("xmysqlnd_session_data::quote_name");
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

/* {{{ xmysqlnd_session_data::get_error_no */
unsigned int
xmysqlnd_session_data::get_error_no()
{
	return error_info->error_no;
}
/* }}} */

/* {{{ xmysqlnd_session_data::get_error_str */
const char *
xmysqlnd_session_data::get_error_str()
{
	return error_info->error;
}
/* }}} */

/* {{{ xmysqlnd_session_data::get_sqlstate */
const char *
xmysqlnd_session_data::get_sqlstate()
{
	return error_info->sqlstate[0] ? error_info->sqlstate : MYSQLND_SQLSTATE_NULL;
}
/* }}} */

/* {{{ xmysqlnd_session_data::get_error_info */
const MYSQLND_ERROR_INFO*
xmysqlnd_session_data::get_error_info() const
{
	return error_info;
}
/* }}} */


/* {{{ xmysqlnd_session_data::set_client_option */
enum_func_status
xmysqlnd_session_data::set_client_option(enum_xmysqlnd_client_option option,
											const char * const value)
{
	enum_func_status ret{PASS};
	DBG_ENTER("xmysqlnd_session_data::set_client_option");
	DBG_INF_FMT("option=%u", option);

	switch (option) {
	case XMYSQLND_OPT_READ_TIMEOUT:
		ret = io.vio->data->m.set_client_option(io.vio, static_cast<enum_mysqlnd_client_option>(option), value);
		break;
	default:
		ret = FAIL;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_send_close */
enum_func_status
xmysqlnd_session_data::send_close()
{
	enum_func_status ret{PASS};
	MYSQLND_VIO * vio = io.vio;
	php_stream * net_stream = vio->data->m.get_stream(vio);
	const enum xmysqlnd_session_state state_val = state.get();

	DBG_ENTER("mysqlnd_send_close");
	DBG_INF_FMT("session=%p vio->data->stream->abstract=%p", this, net_stream? net_stream->abstract:nullptr);
	DBG_INF_FMT("state=%u", state_val);

	if (state_val >= SESSION_NON_AUTHENTICATED) {
		XMYSQLND_DEC_GLOBAL_STATISTIC(XMYSQLND_STAT_OPENED_CONNECTIONS);
		if (persistent) {
			XMYSQLND_DEC_GLOBAL_STATISTIC(XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS);
		}
	}
	switch (state_val) {
	case SESSION_NON_AUTHENTICATED:
	case SESSION_READY: {
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&io, stats, error_info);
		struct st_xmysqlnd_msg__connection_close conn_close_msg = msg_factory.get__connection_close(&msg_factory);
		DBG_INF("Connection clean, sending CON_CLOSE");
		conn_close_msg.send_request(&conn_close_msg);
		conn_close_msg.read_response(&conn_close_msg);

		if (net_stream) {
			/* HANDLE COM_QUIT here */
			vio->data->m.close_stream(vio, stats, error_info);
		}
		state.set(SESSION_CLOSE_SENT);
		break;
	}
	case SESSION_ALLOCATED:
		/*
			  Allocated but not connected or there was failure when trying
			  to connect with pre-allocated connect.

			  Fall-through
			*/
		state.set(SESSION_CLOSE_SENT);
		/* Fall-through */
	case SESSION_CLOSE_SENT:
		/* The user has killed its own connection */
		vio->data->m.close_stream(vio, stats, error_info);
		break;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session_data::negotiate_client_api_capabilities */
size_t
xmysqlnd_session_data::negotiate_client_api_capabilities(const size_t flags)
{
	size_t ret{0};
	DBG_ENTER("xmysqlnd_session_data::negotiate_client_api_capabilities");

	ret = client_api_capabilities;
	client_api_capabilities = flags;

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session_data::get_client_id */
size_t
xmysqlnd_session_data::get_client_id()
{
	return client_id;
}
/* }}} */

const char* Auth_mechanism_mysql41 = "MYSQL41";
const char* Auth_mechanism_plain = "PLAIN";
const char* Auth_mechanism_external = "EXTERNAL";
const char* Auth_mechanism_sha256_memory = "SHA256_MEMORY";
const char* Auth_mechanism_unspecified = "";


const st_xmysqlnd_session_query_bind_variable_bind noop__var_binder = { nullptr, nullptr };
const st_xmysqlnd_session_on_result_start_bind noop__on_result_start = { nullptr, nullptr };
const st_xmysqlnd_session_on_row_bind noop__on_row = { nullptr, nullptr };
const st_xmysqlnd_session_on_warning_bind noop__on_warning = { nullptr, nullptr };
const st_xmysqlnd_session_on_error_bind noop__on_error = { nullptr, nullptr };
const st_xmysqlnd_session_on_result_end_bind noop__on_result_end = { nullptr, nullptr };
const st_xmysqlnd_session_on_statement_ok_bind noop__on_statement_ok = { nullptr, nullptr };

/* {{{ xmysqlnd_session_state::get */
enum xmysqlnd_session_state
st_xmysqlnd_session_state::get()
{
	DBG_ENTER("xmysqlnd_session_state::get");
	DBG_INF_FMT("State=%u", state);
	DBG_RETURN(state);
}
/* }}} */


/* {{{ xmysqlnd_session_state::set */
void
st_xmysqlnd_session_state::set(const enum xmysqlnd_session_state new_state)
{
	DBG_ENTER("xmysqlnd_session_state::set");
	DBG_INF_FMT("New state=%u", state);
	state = new_state;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_session_state_init */
st_xmysqlnd_session_state::st_xmysqlnd_session_state()
{
	DBG_ENTER("st_xmysqlnd_session_state constructor");
	state = SESSION_ALLOCATED;
}
/* }}} */


/* {{{ xmysqlnd_session_data::xmysqlnd_session_data */
xmysqlnd_session_data::xmysqlnd_session_data(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
												   MYSQLND_STATS * mysqlnd_stats,
												   MYSQLND_ERROR_INFO * mysqlnd_error_info)
{
	DBG_ENTER("xmysqlnd_session_data::xmysqlnd_session_data");
	object_factory = factory;

	if (error_info) {
		error_info = mysqlnd_error_info? mysqlnd_error_info : &error_info_impl;
	} else {
		if (FAIL == mysqlnd_error_info_init(&error_info_impl, persistent)) {
			throw std::runtime_error("mysqlnd_error_info_init failed");
		}
		error_info = &error_info_impl;
	}

	if (stats) {
		stats = mysqlnd_stats;
		own_stats = FALSE;
	} else {
		mysqlnd_stats_init(&stats, STAT_LAST, persistent);
		own_stats = TRUE;
	}

	io.pfc = xmysqlnd_pfc_create(persistent, object_factory, mysqlnd_stats, error_info);
	io.vio = mysqlnd_vio_init(persistent, nullptr, mysqlnd_stats, error_info);
	charset = mysqlnd_find_charset_name(XMYSQLND_SESSION_CHARSET);

	if (!io.pfc || !io.vio || !charset) {
		cleanup();
		free_contents();
		throw std::runtime_error("Unable to create the object");
	}

	savepoint_name_seed = 1;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_session_data::~xmysqlnd_session_data */
xmysqlnd_session_data::~xmysqlnd_session_data()
{
	DBG_ENTER("xmysqlnd_session_data::~xmysqlnd_session_data");
	send_close();
	cleanup();
	free_contents();
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_session_data::cleanup */
void xmysqlnd_session_data::cleanup()
{
	DBG_ENTER("xmysqlnd_session_data::reset");
	zend_bool pers = persistent;

	if (io.pfc) {
		io.pfc->data->m.free_contents(io.pfc);
	}

	if (io.vio) {
		io.vio->data->m.free_contents(io.vio);
	}

	DBG_INF("Freeing memory of members");

	if(auth) {
		delete auth;
		auth = nullptr;
	}
	if (current_db.s) {
		mnd_efree(current_db.s);
		current_db.s = nullptr;
	}

	if (scheme.s) {
		mnd_efree(scheme.s);
		scheme.s = nullptr;
	}
	if (server_host_info) {
		mnd_efree(server_host_info);
		server_host_info = nullptr;
	}
	util::zend::free_error_info_list(error_info, pers);
	charset = nullptr;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_session_data::free_contents */
void xmysqlnd_session_data::free_contents()
{
	DBG_ENTER("xmysqlnd_session_data::free_contents");
	if( io.pfc ) {
		xmysqlnd_pfc_free(io.pfc, stats, error_info);
		io.pfc = nullptr;
	}
	if (io.vio) {
		mysqlnd_vio_free(io.vio, stats, error_info);
		io.vio = nullptr;
	}
	if (stats && own_stats) {
		mysqlnd_stats_end(stats, persistent);
		stats = nullptr;
	}
	DBG_VOID_RETURN;
}
/* }}} */

// ----------------------------------------------------------------------------

/* {{{ raise_session_error */
void raise_session_error(
	xmysqlnd_session_data* session,
	const unsigned int code,
	const char* sql_state,
	const char* message)
{
	if (session->error_info) {
		SET_CLIENT_ERROR(session->error_info, code, sql_state, message);
	}
	const util::string& what{ util::prepare_reason_msg(code, sql_state, message) };
	php_error_docref(nullptr, E_WARNING, "%s", what.c_str());
}
/* }}} */


/* {{{ handler_on_error */
const enum_hnd_func_status
xmysqlnd_session_data_handler_on_error(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	xmysqlnd_session_data* session = static_cast<xmysqlnd_session_data*>(context);
	DBG_ENTER("xmysqlnd_stmt::handler_on_error");
	raise_session_error(session, code, sql_state.s, message.s);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

/* {{{ xmysqlnd_session_data_handler_on_auth_continue */
const enum_hnd_func_status
xmysqlnd_session_data_handler_on_auth_continue(
		void* context,
		const MYSQLND_CSTRING input,
		MYSQLND_STRING* const output)
{
	DBG_ENTER("xmysqlnd_stmt::handler_on_auth_continue");

	const MYSQLND_CSTRING salt{ input };
	DBG_INF_FMT("salt[%d]=%s", salt.l, salt.s);

	Auth_plugin* auth_plugin{ static_cast<Auth_plugin*>(context) };
	const util::string& auth_data{ auth_plugin->prepare_continue_auth_data(salt) };
	output->l = auth_data.length();
	output->s = static_cast<char*>(mnd_emalloc(output->l));
	memcpy(output->s, auth_data.c_str(), output->l);

	xmysqlnd_dump_string_to_log("output", output->s, output->l);

	DBG_RETURN(HND_AGAIN);
}
/* }}} */

/* {{{ xmysqlnd_session_data_set_client_id */
enum_func_status
xmysqlnd_session_data_set_client_id(void * context, const size_t id)
{
	enum_func_status ret{FAIL};
	xmysqlnd_session_data * session = (xmysqlnd_session_data *) context;
	DBG_ENTER("xmysqlnd_session_data_set_client_id");
	DBG_INF_FMT("id=" MYSQLND_LLU_SPEC, id);
	if (context) {
		session->client_id = id;
		ret = PASS;
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ on_muted_auth_warning */
const enum_hnd_func_status
on_muted_auth_warning(
		void* /*context*/,
		const xmysqlnd_stmt_warning_level level,
		const unsigned int code,
		const MYSQLND_CSTRING message)
{
	DBG_ENTER("on_muted_auth_warning");
	DBG_INF_FMT("[%4u] %d %s", code, level, message.s ? message.s : "");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ on_muted_auth_error */
const enum_hnd_func_status
on_muted_auth_error(
		void* /*context*/,
		const unsigned int code,
		const MYSQLND_CSTRING sql_state,
		const MYSQLND_CSTRING message)
{
	DBG_ENTER("on_muted_auth_error");
	DBG_INF_FMT("[%4u][%s] %s", code, sql_state.s ? sql_state.s : "", message.s ? message.s : "");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

// -------------

Auth_scrambler::Auth_scrambler(
		const Authentication_context& context,
		const unsigned int hash_length)
	: context(context)
	, Hash_length(hash_length)
{
}

Auth_scrambler::~Auth_scrambler()
{
}

void Auth_scrambler::run(
		const MYSQLND_CSTRING& salt,
		util::vector<char>& result)
{
	if (drv::is_empty_str(salt)) return;

	if (calc_hash(salt)) {
		hex_hash(result);
	}
}

bool Auth_scrambler::calc_hash(const MYSQLND_CSTRING& salt)
{
	if (context.password.empty()) return false;

	hash.resize(Hash_length, '\0');
	scramble(salt);
	return !hash.empty();
}

void Auth_scrambler::hex_hash(util::vector<char>& hexed_hash)
{
	const char hexconvtab[] = "0123456789abcdef";
	hexed_hash.resize(Hash_length * 2, '\0');
	for (unsigned int i{0}; i < Hash_length; ++i) {
		hexed_hash[ i*2 ] = hexconvtab[ hash[i] >> 4 ];
		hexed_hash[ i*2 + 1 ] = hexconvtab[ hash[i] & 0xF ];
	}
}

// --------------------------

class Mysql41_auth_scrambler : public Auth_scrambler
{
public:
	Mysql41_auth_scrambler(const Authentication_context& context);

protected:
	void scramble(const MYSQLND_CSTRING& salt) override;

};

// -------------

Mysql41_auth_scrambler::Mysql41_auth_scrambler(const Authentication_context& context)
	: Auth_scrambler(context, SHA1_MAX_LENGTH)
{
}

void Mysql41_auth_scrambler::scramble(const MYSQLND_CSTRING& salt)
{
	/*
		CLIENT_HASH=xor(sha256(sha256(sha256(password)), nonce), sha256(password))
	*/
	const util::string& password{ context.password };
	php_mysqlnd_scramble(
				hash.data(),
				reinterpret_cast<const unsigned char*>(salt.s),
				reinterpret_cast<const unsigned char*>(password.c_str()),
				password.length());
}

// --------------------------

const unsigned int SHA256_MAX_LENGTH = 32;

class Sha256_mem_auth_scrambler : public Auth_scrambler
{
public:
	Sha256_mem_auth_scrambler(const Authentication_context& context);

protected:
	void scramble(const MYSQLND_CSTRING& salt) override;

private:
	void hash_data(
		const unsigned char* salt,
		const unsigned char* password,
		const unsigned int password_len,
			unsigned char* buffer);
	void crypt_data(
		const unsigned char* lhs,
		const unsigned char* rhs,
		const size_t length,
		unsigned char* buffer);

};

// -------------

Sha256_mem_auth_scrambler::Sha256_mem_auth_scrambler(const Authentication_context& context)
	: Auth_scrambler(context, SHA256_MAX_LENGTH)
{
}

void Sha256_mem_auth_scrambler::scramble(const MYSQLND_CSTRING& salt)
{
	const util::string& password{ context.password };
	hash_data(
		reinterpret_cast<const unsigned char*>(salt.s),
		reinterpret_cast<const unsigned char*>(password.c_str()),
		static_cast<unsigned int>(password.length()),
		hash.data());
}

void Sha256_mem_auth_scrambler::hash_data(
	const unsigned char* salt,
	const unsigned char* password,
	const unsigned int password_len,
	unsigned char* buffer)
{
	/*
		CLIENT_HASH=SHA256(SHA256(SHA256(PASSWORD)),NONCE) XOR SHA256(PASSWORD)
	*/
	PHP_SHA256_CTX ctx;

	// step 0: hash password - sha0=SHA256(PASSWORD)
	unsigned char sha0[SHA256_MAX_LENGTH];
	PHP_SHA256Init(&ctx);
	PHP_SHA256Update(&ctx, password, password_len);
	PHP_SHA256Final(sha0, &ctx);

	// step 1: hash sha0 - sha1=SHA256(sha0)
	unsigned char sha1[SHA256_MAX_LENGTH];
	PHP_SHA256Init(&ctx);
	PHP_SHA256Update(&ctx, sha0, SHA256_MAX_LENGTH);
	PHP_SHA256Final(sha1, &ctx);

	// step 2: hash sha1 + NONCE = buffer=SHA256(sha1,NONCE)
	PHP_SHA256Init(&ctx);
	PHP_SHA256Update(&ctx, sha1, SHA256_MAX_LENGTH);
	PHP_SHA256Update(&ctx, salt, Scramble_length);
	PHP_SHA256Final(buffer, &ctx);

	// step 3: crypt buffer - buffer XOR sha0
	crypt_data(buffer, sha0, SHA256_MAX_LENGTH, buffer);
}

void Sha256_mem_auth_scrambler::crypt_data(
		const unsigned char* lhs,
		const unsigned char* rhs,
		const size_t length,
		unsigned char* buffer)
{
	for (size_t i{0}; i < length; ++i) {
		buffer[i] = lhs[i] ^ rhs[i];
	}
}

// -----------------------------------------------------------------


Auth_plugin_base::Auth_plugin_base(
		const char* mech_name,
		const Authentication_context& context)
	: mech_name(mech_name)
	, context(context)
{
}

const char* Auth_plugin_base::get_mech_name() const
{
	return mech_name;
}

util::string Auth_plugin_base::prepare_start_auth_data()
{
	// by default do nothing - don't send any auth data
	return util::string();
}

util::string Auth_plugin_base::prepare_continue_auth_data(const MYSQLND_CSTRING& /*salt*/)
{
	assert("call should never happen- method should be overriden!");
	return util::string();
}

void Auth_plugin_base::add_prefix_to_auth_data()
{
	// auth_data = "SCHEMA\0USER\0{custom_scramble_ending}"
	// prefix    = "SCHEMA\0USER\0"
	add_to_auth_data(context.database);
	add_to_auth_data('\0');
	add_to_auth_data(context.username);
	add_to_auth_data('\0');
}

void Auth_plugin_base::add_scramble_to_auth_data(const MYSQLND_CSTRING& salt)
{
	std::unique_ptr<Auth_scrambler> scrambler{ get_scrambler() };
	util::vector<char> scramble;
	scrambler->run(salt, scramble);
	add_to_auth_data(scramble);
}

void Auth_plugin_base::add_to_auth_data(const util::string& str)
{
	auth_data.insert(auth_data.end(), str.begin(), str.end());
}

void Auth_plugin_base::add_to_auth_data(const util::vector<char>& data)
{
	auth_data.insert(auth_data.end(), data.begin(), data.end());
}

void Auth_plugin_base::add_to_auth_data(char chr)
{
	auth_data.push_back(chr);
}

std::unique_ptr<Auth_scrambler> Auth_plugin_base::get_scrambler()
{
	assert(!"shouldn't happen - scrambler unavailable, method should be overriden!");
	return std::unique_ptr<Auth_scrambler>();
}

util::string Auth_plugin_base::auth_data_to_string() const
{
	return util::string(auth_data.begin(), auth_data.end());
}

// --------------------------

class Plain_auth_plugin : public Auth_plugin_base
{
public:
	Plain_auth_plugin(const Authentication_context& context);

	util::string prepare_start_auth_data() override;
};

Plain_auth_plugin::Plain_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_plain, context)
{
}

util::string Plain_auth_plugin::prepare_start_auth_data()
{
	// auth_data='SCHEMA\0USER\0PASSWORD'
	add_prefix_to_auth_data();
	add_to_auth_data(context.password);

	return auth_data_to_string();
}

// --------------------------

class Mysql41_auth_plugin : public Auth_plugin_base
{
public:
	Mysql41_auth_plugin(const Authentication_context& context);

	util::string prepare_continue_auth_data(const MYSQLND_CSTRING& salt) override;

protected:
	std::unique_ptr<Auth_scrambler> get_scrambler() override;

};

Mysql41_auth_plugin::Mysql41_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_mysql41, context)
{
}

util::string Mysql41_auth_plugin::prepare_continue_auth_data(const MYSQLND_CSTRING& salt)
{
	/*
		auth_data = "SCHEMA\0USER\0*SCRAMBLE\0"
	*/
	add_prefix_to_auth_data();
	add_to_auth_data('*');
	add_scramble_to_auth_data(salt);
	add_to_auth_data('\0');
	return auth_data_to_string();
}

std::unique_ptr<Auth_scrambler> Mysql41_auth_plugin::get_scrambler()
{
	return std::unique_ptr<Auth_scrambler>(new Mysql41_auth_scrambler(context));
}

// --------------------------

class Sha256_mem_auth_plugin : public Auth_plugin_base
{
public:
	Sha256_mem_auth_plugin(const Authentication_context& context);

	util::string prepare_continue_auth_data(const MYSQLND_CSTRING& salt) override;

protected:
	std::unique_ptr<Auth_scrambler> get_scrambler() override;

};

Sha256_mem_auth_plugin::Sha256_mem_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_sha256_memory, context)
{
}

util::string Sha256_mem_auth_plugin::prepare_continue_auth_data(const MYSQLND_CSTRING& salt)
{
	/*
		auth_data = "SCHEMA\0USER\0SCRAMBLE"
	*/
	add_prefix_to_auth_data();
	add_scramble_to_auth_data(salt);

	return auth_data_to_string();
}

std::unique_ptr<Auth_scrambler> Sha256_mem_auth_plugin::get_scrambler()
{
	return std::unique_ptr<Auth_scrambler>(new Sha256_mem_auth_scrambler(context));
}

// --------------------------

class External_auth_plugin : public Auth_plugin_base
{
public:
	External_auth_plugin(const Authentication_context& context);

	util::string prepare_continue_auth_data(const MYSQLND_CSTRING& salt) override;

};

External_auth_plugin::External_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_external, context)
{
}

util::string External_auth_plugin::prepare_continue_auth_data(const MYSQLND_CSTRING& /*salt*/)
{
	return auth_data_to_string();
}

// ---------------------------------------

std::unique_ptr<Auth_plugin> create_auth_plugin(
		const Auth_mechanism auth_mechanism,
		const Authentication_context& auth_ctx)
{
	std::unique_ptr<Auth_plugin> auth_plugin;
	switch (auth_mechanism) {
	case Auth_mechanism::plain:
		auth_plugin.reset(new Plain_auth_plugin(auth_ctx));
		break;

	case Auth_mechanism::mysql41:
		auth_plugin.reset(new Mysql41_auth_plugin(auth_ctx));
		break;

	case Auth_mechanism::external:
		auth_plugin.reset(new External_auth_plugin(auth_ctx));
		break;

	case Auth_mechanism::sha256_memory:
		auth_plugin.reset(new Sha256_mem_auth_plugin(auth_ctx));
		break;

	default:
		assert(!"unknown Auth_mechanism!");
	}
	return auth_plugin;
}


// ----------------------------------------------------------------------------

util::string auth_mechanism_to_str(Auth_mechanism auth_mechanism)
{
	using Auth_mechanism_to_label = std::map<Auth_mechanism, std::string>;
	static const Auth_mechanism_to_label auth_mechanism_to_label = {
		{ Auth_mechanism::mysql41, Auth_mechanism_mysql41 },
		{ Auth_mechanism::plain, Auth_mechanism_plain },
		{ Auth_mechanism::external, Auth_mechanism_external },
		{ Auth_mechanism::sha256_memory, Auth_mechanism_sha256_memory },
		{ Auth_mechanism::unspecified, Auth_mechanism_unspecified }
	};

	return util::to_string(auth_mechanism_to_label.at(auth_mechanism));
}

util::strings to_auth_mech_names(const Auth_mechanisms& auth_mechanisms)
{
	util::strings auth_mech_names;
	std::transform(
		auth_mechanisms.begin(),
		auth_mechanisms.end(),
		std::back_inserter(auth_mech_names),
		auth_mechanism_to_str);
	return auth_mech_names;
}

// ----------------------------------------------------------------------------

Gather_auth_mechanisms::Gather_auth_mechanisms(
	const XMYSQLND_SESSION_AUTH_DATA* auth,
	const zval* capabilities,
	Auth_mechanisms* auth_mechanisms)
	: auth(auth)
	, capabilities(capabilities)
	, auth_mechanisms(*auth_mechanisms)
{
}

bool Gather_auth_mechanisms::run()
{
	/*
		WL#11591 DevAPI: Add SHA256_MEMORY support
		if (user picked an auth mechanism in the connection string) {
			try: whatever the user wants or fail with error reported by the server
		} else if (tls_enabled OR unix_domain_socket) {
			try: PLAIN or fail with error reported by the server
		} else {
			first try: MYSQL41
			then try: SHA256_MEMORY
			then fail with: { severity: 1, code: 1045, msg:
				'Authentication failed using MYSQL41 and SHA256_MEMORY, check username
				and password or try a secure connection', sql_state: 'HY000' }
		}
	*/
	const Auth_mechanism user_auth_mechanism{auth->auth_mechanism};
	if (user_auth_mechanism != Auth_mechanism::unspecified) {
		add_auth_mechanism(user_auth_mechanism);
	} else if (is_tls_enabled()) {
		add_auth_mechanism(Auth_mechanism::plain);
	} else {
		add_auth_mechanism_if_supported(Auth_mechanism::mysql41);
		add_auth_mechanism_if_supported(Auth_mechanism::sha256_memory);
	}

	return !auth_mechanisms.empty();
}

Authenticate::Authenticate(
	xmysqlnd_session_data* session,
	const MYSQLND_CSTRING& scheme,
	const MYSQLND_CSTRING& database)
	: session(session)
	, scheme(scheme)
	, database(database)
	, msg_factory(xmysqlnd_get_message_factory(&session->io, session->stats, session->error_info))
	, auth(session->auth)
{
}

Authenticate::~Authenticate()
{
	zval_dtor(&capabilities);
}

bool Authenticate::run()
{
	if (!init_capabilities()) return false;

	if (!init_connection()) return false;

	if (!gather_auth_mechanisms()) return false;

	return authentication_loop();
}

bool Authenticate::init_capabilities()
{
	caps_get = msg_factory.get__capabilities_get(&msg_factory);
	if (caps_get.send_request(&caps_get) != PASS) return false;

	ZVAL_NULL(&capabilities);
	const st_xmysqlnd_on_error_bind on_error{
		xmysqlnd_session_data_handler_on_error,
		session
	};

	caps_get.init_read(&caps_get, on_error);
	return caps_get.read_response(&caps_get, &capabilities) == PASS;
}

bool Authenticate::init_connection()
{
	zend_bool tls_set{FALSE};
	xmysqlnd_get_tls_capability(&capabilities, &tls_set);

	if (auth->ssl_mode == SSL_mode::disabled) return true;

	if (tls_set) {
		return setup_crypto_connection(session, caps_get, msg_factory) == PASS;
	} else {
		php_error_docref(nullptr, E_WARNING, "Cannot connect to MySQL by using SSL, unsupported by the server");
		return false;
	}
}

bool Authenticate::gather_auth_mechanisms()
{
	Gather_auth_mechanisms gather_auth_mechanisms(auth, &capabilities, &auth_mechanisms);
	return gather_auth_mechanisms.run();
}

bool Authenticate::authentication_loop()
{
	Authentication_context auth_ctx{
		session,
		scheme,
		auth->username,
		auth->password,
		util::to_string(database)
	};

	for (Auth_mechanism auth_mechanism : auth_mechanisms) {
		std::unique_ptr<Auth_plugin> auth_plugin{
			create_auth_plugin(auth_mechanism, auth_ctx)
		};

		if (authenticate_with_plugin(auth_plugin)) {
			return true;
		}
	}

	if (is_multiple_auth_mechanisms_algorithm()) {
		raise_multiple_auth_mechanisms_algorithm_error();
	}

	return false;
}

bool Authenticate::authenticate_with_plugin(std::unique_ptr<Auth_plugin>& auth_plugin)
{
	const util::string& auth_mech_name = auth_plugin->get_mech_name();
	const util::string& auth_data{ auth_plugin->prepare_start_auth_data() };
	st_xmysqlnd_msg__auth_start auth_start_msg{ msg_factory.get__auth_start(&msg_factory) };
	auto ret{ auth_start_msg.send_request(
		&auth_start_msg,
		util::to_mysqlnd_cstr(auth_mech_name),
		util::to_mysqlnd_cstr(auth_data))
	};

	if (ret != PASS) return false;

	const st_xmysqlnd_on_auth_continue_bind on_auth_continue{
		xmysqlnd_session_data_handler_on_auth_continue,
		auth_plugin.get()
	};
	const st_xmysqlnd_on_warning_bind on_warning{
		is_multiple_auth_mechanisms_algorithm() ? on_muted_auth_warning : nullptr,
		session
	};
	const st_xmysqlnd_on_error_bind on_error{
		is_multiple_auth_mechanisms_algorithm() ? on_muted_auth_error : xmysqlnd_session_data_handler_on_error,
		session
	};
	const st_xmysqlnd_on_client_id_bind on_client_id{
		xmysqlnd_session_data_set_client_id,
		session
	};
	const st_xmysqlnd_on_session_var_change_bind on_session_var_change{
		nullptr,
		session
	};

	auth_start_msg.init_read(&auth_start_msg,
		on_auth_continue,
		on_warning,
		on_error,
		on_client_id,
		on_session_var_change);
	return auth_start_msg.read_response(&auth_start_msg, nullptr) == PASS;
}

void Authenticate::raise_multiple_auth_mechanisms_algorithm_error()
{
	const util::strings& auth_mech_names{ to_auth_mech_names(auth_mechanisms) };
	util::ostringstream os;
	os << "Authentication failed using "
	   << boost::join(auth_mech_names, ", ")
	   << ". Check username and password or try a secure connection";

	raise_session_error(
		session,
		static_cast<unsigned int>(util::xdevapi_exception::Code::authentication_failure),
		GENERAL_SQL_STATE,
		os.str().c_str());
}

bool Authenticate::is_multiple_auth_mechanisms_algorithm() const
{
	return 1 < auth_mechanisms.size();
}

bool Gather_auth_mechanisms::is_tls_enabled() const
{
	return auth->ssl_mode != SSL_mode::disabled;
}

bool Gather_auth_mechanisms::is_auth_mechanism_supported(Auth_mechanism auth_mechanism) const
{
	zval* entry{nullptr};
	const zval* auth_mechs = zend_hash_str_find(Z_ARRVAL_P(capabilities),
												"authentication.mechanisms", sizeof("authentication.mechanisms") - 1);
	if (!capabilities || Z_TYPE_P(auth_mechs) != IS_ARRAY) {
		return false;
	}

	const util::string& auth_mech_name{ auth_mechanism_to_str(auth_mechanism) };
	MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL_P(auth_mechs), entry) {
		if (!strcasecmp(Z_STRVAL_P(entry), auth_mech_name.c_str())) {
			return true;
		}
	} ZEND_HASH_FOREACH_END();

	return false;
}

void Gather_auth_mechanisms::add_auth_mechanism(Auth_mechanism auth_mechanism)
{
	auth_mechanisms.push_back(auth_mechanism);
}

void Gather_auth_mechanisms::add_auth_mechanism_if_supported(Auth_mechanism auth_mechanism)
{
	if (is_auth_mechanism_supported(auth_mechanism)) {
		add_auth_mechanism(auth_mechanism);
	}
}

// ----------------------------------------------------------------------------

/* {{{ setup_crypto_options */
void setup_crypto_options(
		php_stream_context* stream_context,
		xmysqlnd_session_data* session)
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
	php_stream_context_set_option(stream_context,"ssl","verify_peer",&verify_peer_zval);
	php_stream_context_set_option(stream_context,"ssl","verify_peer_name",&verify_peer_zval);
	DBG_VOID_RETURN;
}

/* }}} */

/* {{{ setup_crypto_connection */
enum_func_status setup_crypto_connection(
		xmysqlnd_session_data* session,
		st_xmysqlnd_msg__capabilities_get& caps_get,
		const st_xmysqlnd_message_factory& msg_factory)
{
	DBG_ENTER("setup_crypto_connection");
	enum_func_status ret{FAIL};
	const struct st_xmysqlnd_on_error_bind on_error =
	{ xmysqlnd_session_data_handler_on_error, (void*) session };
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
		zval zvalue;
		ZVAL_NULL(&zvalue);
		caps_get.init_read(&caps_get, on_error);
		ret = caps_get.read_response(&caps_get,
									 &zvalue);
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
		zval_ptr_dtor(&zvalue);
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


char* build_server_host_info(const util::string& format,
							 const util::string& name,
							 zend_bool session_persistent)
{
	char *hostname{ nullptr }, *host_info{ nullptr };
	mnd_sprintf(&hostname, 0, format.c_str(), name.c_str());
	if (hostname) {
		host_info = mnd_pestrdup(hostname, 0);
		mnd_sprintf_free(hostname);
	}
	return host_info;
}

/* {{{ xmysqlnd_session::xmysqlnd_session */
xmysqlnd_session::xmysqlnd_session(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
										 MYSQLND_STATS * stats,
										 MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_session::xmysqlnd_session");

	session_uuid = new Uuid_generator();
	xmysqlnd_session_data * session_data = factory->get_session_data(factory, persistent, stats, error_info);
	if (session_data) {
		data = std::shared_ptr<xmysqlnd_session_data>(session_data);
	}
}
/* }}} */


/* {{{ xmysqlnd_session::~xmysqlnd_session */
xmysqlnd_session::~xmysqlnd_session()
{
	DBG_ENTER("xmysqlnd_session::~xmysqlnd_session");
	if (server_version_string) {
		mnd_efree(server_version_string);
		server_version_string = nullptr;
	}
	if(session_uuid) {
		delete session_uuid;
	}
}
/* }}} */

XMYSQLND_SESSION_DATA
xmysqlnd_session::get_data()
{
	return data;
}

/* {{{ xmysqlnd_session::connect */
const enum_func_status
xmysqlnd_session::connect(MYSQLND_CSTRING database,
							const unsigned int port,
							const size_t set_capabilities)
{
	enum_func_status ret{FAIL};

	DBG_ENTER("xmysqlnd_session::connect");
	ret = data->connect(database,port, set_capabilities);
#ifdef WANTED_TO_PRECACHE_UUIDS_AT_CONNECT
	if (PASS == ret) {
		ret = precache_uuids();
	}
#endif
	DBG_RETURN(ret);
}
/* }}} */

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

/* {{{ xmysqlnd_schema_operation */
enum_func_status
xmysqlnd_session::xmysqlnd_schema_operation(const MYSQLND_CSTRING operation, const MYSQLND_CSTRING db)
{
	enum_func_status ret{FAIL};
	const MYSQLND_STRING quoted_db = data->quote_name(db);
	DBG_ENTER("xmysqlnd_schema_operation");
	DBG_INF_FMT("db=%s", db);

	if (quoted_db.s && quoted_db.l) {
		util::string query_content(operation.s, operation.l);
		query_content.append(quoted_db.s, quoted_db.l);
		const MYSQLND_CSTRING select_query = { query_content.c_str(), query_content.length() };
		mnd_efree(quoted_db.s);

		ret = query( namespace_sql, select_query, noop__var_binder);
	}
	DBG_RETURN(ret);

}
/* }}} */


/* {{{ xmysqlnd_session::select_db */
const enum_func_status
xmysqlnd_session::select_db(const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "USE ", sizeof("USE ") - 1 };
	DBG_ENTER("xmysqlnd_session::select_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session::create_db */
const enum_func_status
xmysqlnd_session::create_db(const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "CREATE DATABASE ", sizeof("CREATE DATABASE ") - 1 };
	DBG_ENTER("xmysqlnd_session::create_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session::drop_db */
const enum_func_status
xmysqlnd_session::drop_db(const MYSQLND_CSTRING db)
{
	enum_func_status ret;
	static const MYSQLND_CSTRING operation = { "DROP DATABASE ", sizeof("DROP DATABASE ") - 1 };
	DBG_ENTER("xmysqlnd_session::drop_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ st_xmysqlnd_query_cb_ctx */
struct st_xmysqlnd_query_cb_ctx
{
	XMYSQLND_SESSION session;
	struct st_xmysqlnd_session_on_result_start_bind handler_on_result_start;
	struct st_xmysqlnd_session_on_row_bind handler_on_row;
	struct st_xmysqlnd_session_on_warning_bind handler_on_warning;
	struct st_xmysqlnd_session_on_error_bind handler_on_error;
	struct st_xmysqlnd_session_on_result_end_bind handler_on_result_end;
	struct st_xmysqlnd_session_on_statement_ok_bind handler_on_statement_ok;
};
/* }}} */


/* {{{ query_cb_handler_on_result_start */
const enum_hnd_func_status
query_cb_handler_on_result_start(void * context, xmysqlnd_stmt * const stmt)
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
						xmysqlnd_stmt * const stmt,
						const st_xmysqlnd_stmt_result_meta* const meta,
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
							xmysqlnd_stmt * const stmt,
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
						  xmysqlnd_stmt * const stmt,
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
query_cb_handler_on_result_end(void * context, xmysqlnd_stmt * const stmt, const zend_bool has_more)
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
query_cb_handler_on_statement_ok(void * context, xmysqlnd_stmt * const stmt, const st_xmysqlnd_stmt_execution_state* const exec_state)
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


/* {{{ xmysqlnd_session::query_cb */
const enum_func_status
xmysqlnd_session::query_cb(			const MYSQLND_CSTRING namespace_,
											const MYSQLND_CSTRING query,
											const struct st_xmysqlnd_session_query_bind_variable_bind var_binder,
											const struct st_xmysqlnd_session_on_result_start_bind handler_on_result_start,
											const struct st_xmysqlnd_session_on_row_bind handler_on_row,
											const struct st_xmysqlnd_session_on_warning_bind handler_on_warning,
											const struct st_xmysqlnd_session_on_error_bind handler_on_error,
											const struct st_xmysqlnd_session_on_result_end_bind handler_on_result_end,
											const struct st_xmysqlnd_session_on_statement_ok_bind handler_on_statement_ok)
{
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_session::query_cb");
    XMYSQLND_SESSION session_handle(this);
	xmysqlnd_stmt * const stmt = create_statement_object(session_handle);
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
		(PASS == (ret = stmt->send_raw_message(stmt,
        xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute),
        data->stats, data->error_info))))
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
            const struct st_xmysqlnd_stmt_on_row_bind on_row = {
                handler_on_row.handler? query_cb_handler_on_row : nullptr,
                &query_cb_ctx
            };
            const struct st_xmysqlnd_stmt_on_warning_bind on_warning = {
                handler_on_warning.handler? query_cb_handler_on_warning : nullptr,
                &query_cb_ctx
            };
            const struct st_xmysqlnd_stmt_on_error_bind on_error = {
                handler_on_error.handler? query_cb_handler_on_error : nullptr,
                &query_cb_ctx
            };
            const struct st_xmysqlnd_stmt_on_result_start_bind on_result_start = {
                handler_on_result_start.handler? query_cb_handler_on_result_start : nullptr,
                &query_cb_ctx
            };
            const struct st_xmysqlnd_stmt_on_result_end_bind on_result_end = {
                handler_on_result_end.handler? query_cb_handler_on_result_end : nullptr,
                &query_cb_ctx
            };
            const struct st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok = {
                handler_on_statement_ok.handler? query_cb_handler_on_statement_ok : nullptr,
                &query_cb_ctx
            };
			ret = stmt->read_all_results(stmt, on_row, on_warning, on_error, on_result_start, on_result_end, on_statement_ok,
            data->stats, data->error_info);
        }
    }
    /* no else, please */
    if (stmt) {
        xmysqlnd_stmt_free(stmt, data->stats, data->error_info);
    }
    if (stmt_execute) {
        xmysqlnd_stmt_execute__destroy(stmt_execute);
    }

    session_handle.reset();
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_session_on_warning */
const enum_hnd_func_status
xmysqlnd_session_on_warning(
	void* /*context*/,
	xmysqlnd_stmt* const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int /*code*/,
	const MYSQLND_CSTRING /*message*/)
{
	DBG_ENTER("xmysqlnd_session_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.l, message.s);
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_session::query */
const enum_func_status
xmysqlnd_session::query(const MYSQLND_CSTRING namespace_,
										 const MYSQLND_CSTRING query,
										 const struct st_xmysqlnd_session_query_bind_variable_bind var_binder)
{
	enum_func_status ret{FAIL};

	DBG_ENTER("xmysqlnd_session::query");
	XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_, query);
	xmysqlnd_stmt * stmt = create_statement_object(shared_from_this());
	if (stmt && stmt_execute) {
		ret = PASS;
		if (var_binder.handler) {
			zend_bool loop{TRUE};
			do {
				const enum_hnd_func_status var_binder_result = var_binder.handler(var_binder.ctx, shared_from_this(), stmt_execute);
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
				(PASS == (ret = stmt->send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), data->stats, data->error_info))))
		{
			do {
				const struct st_xmysqlnd_stmt_on_warning_bind on_warning = { xmysqlnd_session_on_warning, nullptr };
				const struct st_xmysqlnd_stmt_on_error_bind on_error = { nullptr, nullptr };
				zend_bool has_more{FALSE};
				XMYSQLND_STMT_RESULT * result = stmt->get_buffered_result(stmt, &has_more, on_warning, on_error, data->stats, data->error_info);
				if (result) {
					ret = PASS;
					xmysqlnd_stmt_result_free(result, data->stats, data->error_info);
				} else {
					ret = FAIL;
				}
			} while (stmt->has_more_results(stmt) == TRUE);
		}
	}
	/* no else, please */
	if (stmt) {
		xmysqlnd_stmt_free(stmt, data->stats, data->error_info);
	}
	if (stmt_execute) {
		xmysqlnd_stmt_execute__destroy(stmt_execute);
	}

	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_session::get_server_version */
zend_ulong
xmysqlnd_session::get_server_version()
{
	zend_long major, minor, patch;
	DBG_ENTER("xmysqlnd_session::get_server_version");
	char* p{ server_version_string };
	if (!p) {
		const MYSQLND_CSTRING query = { "SELECT VERSION()", sizeof("SELECT VERSION()") - 1 };
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_sql, query);
        XMYSQLND_SESSION session_handle(this);
		xmysqlnd_stmt * stmt = create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			if (PASS == stmt->send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), data->stats, data->error_info)) {
				const struct st_xmysqlnd_stmt_on_warning_bind on_warning = { nullptr, nullptr };
				const struct st_xmysqlnd_stmt_on_error_bind on_error = { nullptr, nullptr };
				zend_bool has_more{FALSE};
				XMYSQLND_STMT_RESULT * res = stmt->get_buffered_result(stmt, &has_more, on_warning, on_error, data->stats, data->error_info);
				if (res) {
					zval* set{nullptr};
					if (PASS == res->m.fetch_all_c(res, &set, FALSE /* don't duplicate, reference it */, data->stats, data->error_info) &&
							Z_TYPE(set[0 * 0]) == IS_STRING)
					{
						DBG_INF_FMT("Found %*s", Z_STRLEN(set[0 * 0]), Z_STRVAL(set[0 * 0]));
						server_version_string = mnd_pestrndup(Z_STRVAL(set[0 * 0]), Z_STRLEN(set[0 * 0]), 0);
					}
					if (set) {
						mnd_efree(set);
					}
				}
				xmysqlnd_stmt_result_free(res, data->stats, data->error_info);
			}
		}
		/* no else, please */
		if (stmt) {
			xmysqlnd_stmt_free(stmt, data->stats, data->error_info);
		}
		if (stmt_execute) {
			xmysqlnd_stmt_execute__destroy(stmt_execute);
		}
        session_handle.reset();
	} else {
		DBG_INF_FMT("server_version_string=%s", server_version_string);
	}
	if ((p = server_version_string) == nullptr) {
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

/* {{{ xmysqlnd_session::create_statement_object */
xmysqlnd_stmt *
xmysqlnd_session::create_statement_object(XMYSQLND_SESSION session_handle)
{
	xmysqlnd_stmt* stmt{nullptr};
	DBG_ENTER("xmysqlnd_session_data::create_statement_object");
	stmt = xmysqlnd_stmt_create(session_handle, session_handle->persistent, data->object_factory, data->stats, data->error_info);
    DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_session::create_schema_object */
xmysqlnd_schema *
xmysqlnd_session::create_schema_object(const MYSQLND_CSTRING schema_name)
{
	xmysqlnd_schema* schema{nullptr};
	DBG_ENTER("xmysqlnd_session::create_schema_object");
	DBG_INF_FMT("schema_name=%s", schema_name.s);
	schema = xmysqlnd_schema_create(shared_from_this(), schema_name, persistent, data->object_factory, data->stats, data->error_info);

	DBG_RETURN(schema);
}
/* }}} */


/* {{{ xmysqlnd_session::close */
const enum_func_status
xmysqlnd_session::close(const enum_xmysqlnd_session_close_type close_type)
{
	enum_func_status ret{FAIL};

	DBG_ENTER("xmysqlnd_session::close");

	if (data->state.get() >= SESSION_READY) {
		static enum_xmysqlnd_collected_stats close_type_to_stat_map[SESSION_CLOSE_LAST] = {
			XMYSQLND_STAT_CLOSE_EXPLICIT,
			XMYSQLND_STAT_CLOSE_IMPLICIT,
			XMYSQLND_STAT_CLOSE_DISCONNECT
		};
		XMYSQLND_INC_SESSION_STATISTIC(data->stats, close_type_to_stat_map[close_type]);
	}

	/*
		  Close now, free_reference will try,
		  if we are last, but that's not a problem.
		*/
	ret = data->send_close();


	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_session_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION
xmysqlnd_session_create(const size_t client_flags, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_session_create");
	auto session = object_factory->get_session(object_factory, persistent, stats, error_info);
	if (session && session->data) {
		session->data->negotiate_client_api_capabilities( client_flags);
	}
	DBG_RETURN(std::shared_ptr<xmysqlnd_session>(session));
}
/* }}} */

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION
xmysqlnd_session_connect(XMYSQLND_SESSION session,
						 XMYSQLND_SESSION_AUTH_DATA * auth,
						 const MYSQLND_CSTRING database,
						 unsigned int port,
						 const size_t set_capabilities)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory =
			MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	enum_func_status ret{FAIL};

	const size_t client_api_flags{0}; //This is not used at the moment..
	/* may need to pass these from outside */
	MYSQLND_STATS* stats{nullptr};
	MYSQLND_ERROR_INFO* error_info{nullptr};

	DBG_ENTER("xmysqlnd_session_connect");
	DBG_INF_FMT("host=%s user=%s db=%s port=%u flags=%llu",
				auth->hostname.c_str(), auth->username.c_str(), database.s,
				port, static_cast<unsigned long long>(set_capabilities));

	if (!session) {
		if (!(session = xmysqlnd_session_create(client_api_flags,
												FALSE, factory,
												stats, error_info))) {
			/* OOM */
			DBG_RETURN(nullptr);
		}
	}
	session->data->auth = auth;
	ret = session->connect(database,port, set_capabilities);

	if (ret == FAIL) {
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(session);
}
/* }}} */

/* {{{ create_new_session */
mysqlx::devapi::st_mysqlx_session * create_new_session(zval * session_zval)
{
	DBG_ENTER("create_new_session");
	mysqlx::devapi::st_mysqlx_session * object{nullptr};
	if (PASS == mysqlx::devapi::mysqlx_new_session(session_zval)) {
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
enum_func_status establish_connection(mysqlx::devapi::st_mysqlx_session * object,
									  XMYSQLND_SESSION_AUTH_DATA * auth,
									  const util::Url& url,
									  transport_types tr_type)
{
	DBG_ENTER("establish_connection");
	enum_func_status ret{PASS};
	mysqlx::drv::XMYSQLND_SESSION new_session;
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
		new_session = xmysqlnd_session_connect(object->session,
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
		node_url.port = static_cast<unsigned short>(drv::Environment::get_as_int(drv::Environment::Variable::Mysqlx_port));
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


/* {{{ set_auth_mechanism */
enum_func_status set_auth_mechanism(
		XMYSQLND_SESSION_AUTH_DATA* auth,
		Auth_mechanism auth_mechanism)
{
	DBG_ENTER("set_auth_mechanism");
	enum_func_status ret{FAIL};
	if (auth->auth_mechanism == Auth_mechanism::unspecified) {
		DBG_INF_FMT("Selected authentication mechanism: %d", static_cast<int>(auth_mechanism));
		auth->auth_mechanism = auth_mechanism;
		ret = PASS;
	} else if (auth->auth_mechanism != auth_mechanism) {
		DBG_ERR_FMT("Selected two incompatible authentication mechanisms %d vs %d",
					static_cast<int>(auth->auth_mechanism),
					static_cast<int>(auth_mechanism));
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_auth_mechanism);
	}
	DBG_RETURN( ret );
}
/* }}} */

/* {{{ parse_auth_mechanism */
enum_func_status parse_auth_mechanism(
		XMYSQLND_SESSION_AUTH_DATA* auth,
		const util::string& auth_mechanism)
{
	DBG_ENTER("parse_auth_mechanism");
	enum_func_status ret{FAIL};
	using str_to_auth_mechanism = std::map<std::string, Auth_mechanism, util::iless>;
	static const str_to_auth_mechanism str_to_auth_mechanisms = {
		{ Auth_mechanism_mysql41, Auth_mechanism::mysql41 },
		{ Auth_mechanism_plain, Auth_mechanism::plain },
		{ Auth_mechanism_external, Auth_mechanism::external },
		{ Auth_mechanism_sha256_memory , Auth_mechanism::sha256_memory }
	};
	auto it = str_to_auth_mechanisms.find(auth_mechanism.c_str());
	if (it != str_to_auth_mechanisms.end()) {
		ret = set_auth_mechanism(auth, it->second);
	} else {
		devapi::RAISE_EXCEPTION(err_msg_invalid_auth_mechanism);
		ret = FAIL;
		DBG_ERR_FMT("Unknown authentication mechanism: %s", auth_mechanism.c_str());
	}
	DBG_RETURN( ret );
}
/* }}} */

namespace {

using integer_auth_option_to_data_member = std::map<
	std::string,
	boost::optional<int> xmysqlnd_session_auth_data::*,
	util::iless
>;

const auto AUTH_OPTION_CONNECT_TIMEOUT{ "connect-timeout" };

const integer_auth_option_to_data_member int_auth_option_to_data_member{
	{ AUTH_OPTION_CONNECT_TIMEOUT, &xmysqlnd_session_auth_data::connection_timeout },
};

bool is_integer_auth_option(const util::string& auth_option_variable)
{
	return int_auth_option_to_data_member.count(auth_option_variable.c_str()) != 0;
}

bool verify_connection_timeout_option(const int auth_option_value)
{
	if (0 <= auth_option_value) return true;
	throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_timeout);
}

bool verify_integer_auth_option(
	const util::string& auth_option_variable,
	const int auth_option_value)
{
	if (auth_option_variable == AUTH_OPTION_CONNECT_TIMEOUT) {
		return verify_connection_timeout_option(auth_option_value);
	}
	assert(!"unknown integer auth option");
	return false;
}

bool parse_integer_auth_option(
	const util::string& auth_option_variable,
	const util::string& auth_option_value,
	xmysqlnd_session_auth_data* auth_data)
{
	if( auth_option_value.empty() ) {
		util::ostringstream os;
		os << "The argument to " << auth_option_variable << " cannot be empty.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	int value{0};
	if (!util::to_int(auth_option_value, &value)) {
		util::ostringstream os;
		os << "The argument to " << auth_option_variable
			<< " must be an integer, but it is '" << auth_option_value << "'.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	if (!verify_integer_auth_option(auth_option_variable, value)) return false;

	auto int_auth_option_data_member{ int_auth_option_to_data_member.at(auth_option_variable.c_str()) };
	(auth_data->*int_auth_option_data_member) = value;
	return true;
}

} // anonymous namespace


/* {{{ extract_ssl_information */
enum_func_status extract_ssl_information(
		const util::string& auth_option_variable,
		const util::string& auth_option_value,
		XMYSQLND_SESSION_AUTH_DATA* auth)
{
	DBG_ENTER("extract_ssl_information");
	enum_func_status ret{PASS};
	using ssl_option_to_data_member = std::map<std::string,
		util::string xmysqlnd_session_auth_data::*,
		util::iless>;
	// Map the ssl option to the proper member, according to:
	// https://dev.mysql.com/doc/refman/5.7/en/encrypted-connection-options.html
	static const ssl_option_to_data_member ssl_option_to_data_members = {
		{ "ssl-key", &xmysqlnd_session_auth_data::ssl_local_pk },
		{ "ssl-cert",&xmysqlnd_session_auth_data::ssl_local_cert },
		{ "ssl-ca", &xmysqlnd_session_auth_data::ssl_cafile },
		{ "ssl-capath", &xmysqlnd_session_auth_data::ssl_capath },
		{ "ssl-cipher", &xmysqlnd_session_auth_data::ssl_ciphers },
		{ "ssl-crl", &xmysqlnd_session_auth_data::ssl_crl },
		{ "ssl-crlpath", &xmysqlnd_session_auth_data::ssl_crlpath },
		{ "tls-version", &xmysqlnd_session_auth_data::tls_version }
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
			ret = parse_auth_mechanism(auth, auth_option_value);
		} else if (is_integer_auth_option(auth_option_variable)) {
			if (!parse_integer_auth_option(auth_option_variable, auth_option_value, auth)) {
				ret = FAIL;
			}
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
	std::unique_ptr<XMYSQLND_SESSION_AUTH_DATA> auth(new XMYSQLND_SESSION_AUTH_DATA);

	if( nullptr == auth ) {
		util::ostringstream os;
		os << "Couldn't allocate " << sizeof(XMYSQLND_SESSION_AUTH_DATA) << " bytes";
		php_error_docref(nullptr, E_WARNING, "%s", os.str().c_str());
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

			if( FAIL == extract_ssl_information(variable, value, auth.get()) ) {
				ret = FAIL;
				break;
			}
		}
	}

	if( ret != PASS ){
		DBG_RETURN(nullptr);
	}

	/*
	 * If no SSL mode is selected explicitly then
	 * assume 'required'
	 */
	if( auth->ssl_mode == SSL_mode::not_specified ) {
		DBG_INF_FMT("Setting default SSL mode to REQUIRED");
		ret = set_ssl_mode( auth.get(), SSL_mode::required );
	}


	if( node_url.port != 0 ) {
		//If is 0, then we're using win pipe
		//or unix sockets.
		auth->port = node_url.port;
		auth->hostname = node_url.host;
	}
	auth->username = node_url.user;
	auth->password = node_url.pass;

	DBG_RETURN(auth.release());
}
/* }}} */

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

/* {{{ verify_uri_address */
void verify_uri_address(const util::string& uri_address)
{
	php_url* raw_url{ php_url_parse(uri_address.c_str()) };
	const bool uri_valid{ raw_url != nullptr };
	php_url_free(raw_url);

	if (uri_valid) return;

	util::ostringstream os;
	os << "invalid uri '" << uri_address << "'.";
	throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
}
/* }}} */

/* {{{ verify_connection_string */
void verify_connection_string(const util::string& connection_string)
{
	const auto& uri_addresses{ extract_uri_addresses(connection_string) };
	if (uri_addresses.empty()) {
		util::ostringstream os;
		os << "invalid connection_string '" << connection_string << "'.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	for (const auto& uri_address : uri_addresses) {
		verify_uri_address(uri_address.first);
	}
}
/* }}} */

/* {{{ xmysqlnd_new_session_connect */
PHP_MYSQL_XDEVAPI_API
		enum_func_status xmysqlnd_new_session_connect(const char* uri_string,
													  zval * return_value)
{
	DBG_ENTER("xmysqlnd_new_session_connect");
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
	MYSQLND_ERROR_INFO last_error_info{};
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
					if (ret == FAIL) {
						const MYSQLND_ERROR_INFO* session_error_info{
							session->session->get_data()->get_error_info() };
						if (session_error_info) {
							last_error_info = *session_error_info;
						}
					}
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
	/*
	 * Do not worry about the allocated auth if
	 * the connection fails, the dtor of 'session'
	 * will clean it up
	 */
	if( FAIL == ret ) {
		zval_dtor(return_value);
		ZVAL_NULL(return_value);

		if( uris.size() > 1) {
			devapi::RAISE_EXCEPTION( err_msg_all_routers_failed );
		} else if (last_error_info.error_no) {
			throw util::xdevapi_exception(
				last_error_info.error_no,
				last_error_info.sqlstate,
				last_error_info.error);
		}
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
