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
#include "xmysqlnd_compression_setup.h"
#include "xmysqlnd_protocol_dumper.h"
#include "xmysqlnd_stmt_result.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_zval2any.h"
#include "php_mysqlx.h"
#include "xmysqlnd_utils.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "SAPI.h"
#include "php_variables.h"
#include "mysqlx_exception.h"
#include "util/exceptions.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/url_utils.h"
#include "util/value.h"
#include "util/zend_utils.h"
#include <utility>
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>
#include <memory>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#ifndef PHP_WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <resolv.h>
#else
#include <windns.h>
#endif
#include <forward_list>
#include <string>

namespace mysqlx {

namespace drv {

namespace {

#if PHP_VERSION_ID >= 70400
#define TLSv13_IS_SUPPORTED
#endif

inline bool is_tlsv13_supported() {
#ifdef TLSv13_IS_SUPPORTED
	return true;
#else
	return false;
#endif
}


zend_bool
xmysqlnd_is_capability_present(
	const zval* capabilities,
	const std::string& cap_name,
	zend_bool* found
)
{
	zval* zv = zend_hash_str_find(Z_ARRVAL_P(capabilities), cap_name.c_str(), cap_name.length());
	if (!zv || Z_TYPE_P(zv) == IS_UNDEF) {
		*found = FALSE;
		return FALSE;
	}
	*found = TRUE;
	convert_to_boolean(zv);
	return Z_TYPE_P(zv) == IS_TRUE? TRUE:FALSE;
}

} // anonymous namespace

bool set_connection_timeout(
	const std::optional<int>& connection_timeout,
	MYSQLND_VIO* vio)
{
	const int Dont_set_connection_timeout{ 0 };
	int timeout{ Dont_set_connection_timeout };
	if (connection_timeout) {
		timeout = connection_timeout.value();
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

namespace {

bool set_connection_options(
	const Session_auth_data* auth_data,
	MYSQLND_VIO* vio)
{
	auto& connection_timeout = auth_data->connection_timeout;
	return set_connection_timeout(connection_timeout, vio);
}

} // anonymous namespace

Session_auth_data::Session_auth_data() :
	port{ 0 },
	ssl_mode{ SSL_mode::not_specified },
	ssl_no_defaults{ true } {
}

st_xmysqlnd_message_factory xmysqlnd_session_data::create_message_factory()
{
	Message_context msg_ctx{
		io.vio,
		io.pfc,
		stats,
		error_info,
		&compression_executor
	};
	return get_message_factory(msg_ctx);
}

std::string
xmysqlnd_session_data::get_scheme(
	const std::string& hostname,
	unsigned int port)
{
	std::string transport;
	DBG_ENTER("xmysqlnd_session_data::get_scheme");
	/* MY-305: Add support for windows pipe */
	if( transport_type == transport_types::network ) {
		if (!port) {
			port = drv::Environment::get_as_int(drv::Environment::Variable::Mysql_port);
		}
		std::ostringstream os;
		os << "tcp://" << hostname << ':' << port;
		transport = os.str();
	} else if( transport_type == transport_types::unix_domain_socket ) {
		transport = "unix://" + socket_path;
	} else if( transport_type == transport_types::windows_pipe ) {
#ifdef PHP_WIN32
		/* Somewhere here?! (This is old code) */
		if (hostname == ".") {
			/* named pipe in socket */
			socket_path = "\\\\.\\pipe\\MySQL";
		}
		transport = "pipe://" + socket_path;
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
	DBG_INF_FMT("transport=%s", transport.empty() ? "OOM" : transport.c_str());
	DBG_RETURN(transport);
}

Mysqlx::Datatypes::Object*
xmysqlnd_session_data::prepare_client_attr_object()
{
	const std::size_t capa_count{ connection_attribs.size()};
	Mysqlx::Datatypes::Object*        values = new (std::nothrow) Mysqlx::Datatypes::Object;
	if(values){
		std::size_t idx{ 0 };
		for( ; idx < capa_count; ++idx) {
			std::unique_ptr<Mysqlx::Datatypes::Scalar>        scalar{nullptr};
			std::unique_ptr<Mysqlx::Datatypes::Scalar_String> string_value{nullptr};
			std::unique_ptr<Mysqlx::Datatypes::Any>           any{nullptr};

			const auto& connection_attrib = connection_attribs[idx];
			Mysqlx::Datatypes::Object_ObjectField* field = values->add_fld();

			const auto& connection_attrib_key = connection_attrib.first;
			field->set_key(connection_attrib_key.c_str(), connection_attrib_key.length());

			scalar.reset(new Mysqlx::Datatypes::Scalar);
			string_value.reset(new Mysqlx::Datatypes::Scalar_String);
			any.reset(new Mysqlx::Datatypes::Any);

			const auto& connection_attrib_value = connection_attrib.second;
			string_value->set_value(connection_attrib_value.c_str(), connection_attrib_value.length());
			scalar->set_type(Mysqlx::Datatypes::Scalar_Type_V_STRING);
			scalar->set_allocated_v_string(string_value.release());

			any->set_allocated_scalar(scalar.release());
			any->set_type( Mysqlx::Datatypes::Any_Type::Any_Type_SCALAR );
			field->set_allocated_value(any.release());
		}

		if( idx < capa_count ) {
			delete values;
			values = nullptr;
		}
	}
	return values;
}

enum_func_status
xmysqlnd_session_data::send_client_attributes()
{
	DBG_ENTER("send_client_attributes");
	const std::size_t capa_count{ connection_attribs.size()};
	enum_func_status  ret{ PASS };
	if( capa_count > 0 ) {
		ret = FAIL;
		st_xmysqlnd_message_factory msg_factory{ create_message_factory() };

		st_xmysqlnd_msg__capabilities_set caps_set{ msg_factory.get__capabilities_set(&msg_factory) };
		st_xmysqlnd_msg__capabilities_get caps_get{ msg_factory.get__capabilities_get(&msg_factory) };

		zval ** capability_names = static_cast<zval**>( mnd_ecalloc(1 , sizeof(zval*)));
		zval ** capability_values = static_cast<zval**>( mnd_ecalloc(1, sizeof(zval*)));

		if(  capability_names && capability_values ) {
			Mysqlx::Datatypes::Object* values = prepare_client_attr_object();

			if( values ) {
				Mysqlx::Datatypes::Any final_any;
				final_any.set_allocated_obj(values);
				final_any.set_type( Mysqlx::Datatypes::Any_Type::Any_Type_OBJECT );

				zval name;
				ZVAL_NULL(&name);

				zval value;
				ZVAL_NULL(&value);

				ZVAL_STRINGL(&name,
							 "session_connect_attrs",
							 strlen("session_connect_attrs"));
				capability_names[0] = &name;

				any2zval(final_any,&value);
				capability_values[0] = &value;

				const st_xmysqlnd_on_error_bind on_error =
				{ xmysqlnd_session_data_handler_on_error, (void*) this };

				if( PASS == caps_set.send_request(&caps_set,
												  1, //session_connect_attrs
												  capability_names,
												  capability_values) ) {
					DBG_INF_FMT("Successfully submitted the connection attributes to the server.");
					zval zvalue;
					ZVAL_NULL(&zvalue);
					caps_get.init_read(&caps_get, on_error);
					ret = caps_get.read_response(&caps_get,
												 &zvalue);
					if( ret == PASS ) {
						DBG_INF_FMT("Server response OK for the submitted connection attributes.");
					} else {
						DBG_ERR_FMT("Negative response from the server for the submitted connection attributes");
					}

					zval_ptr_dtor(&zvalue);
				}

				zval_ptr_dtor(&name);
				zval_ptr_dtor(&value);
			} else{
				if( values ) {
					delete values;
				}
			}
		}
		else {
			DBG_ERR_FMT("Unable to allocate the memory for the capability objects");
		}

		mnd_efree(capability_names);
		mnd_efree(capability_values);
	}
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_session_data::connect_handshake(
	const util::string_view& scheme_name,
	const util::string& default_schema,
	const size_t set_capabilities)
{
	enum_func_status ret{FAIL};
	DBG_ENTER("xmysqlnd_session_data::connect_handshake");

	if (set_connection_options(auth.get(), io.vio)
		&& (PASS == io.vio->data->m.connect(io.vio,
											util::to_mysqlnd_cstr(scheme_name),
											persistent,
											stats,
											error_info))
		&& (PASS == io.pfc->data->m.reset(io.pfc,
										  stats,
										  error_info))) {
		state.set(SESSION_CONNECTING);
		ret = send_client_attributes();
		if( ret == PASS ) {
			ret = authenticate(scheme_name, default_schema, set_capabilities);
		}
	}
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_session_data::authenticate(
	const util::string_view& scheme_name,
	const util::string& default_schema,
	const size_t /*set_capabilities*/,
	const bool re_auth)
{
	DBG_ENTER("xmysqlnd_session_data::authenticate");
	Authenticate authenticate(this, scheme_name, default_schema);
	enum_func_status ret{FAIL};
	if (authenticate.run(re_auth)) {
		DBG_INF("AUTHENTICATED. YAY!");
		ret = PASS;
	}
	capabilities = authenticate.get_capabilities();
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_session_data::connect(
	const util::string& def_schema,
	unsigned int port,
	size_t set_capabilities)
{
	zend_bool reconnect{FALSE};
	enum_func_status ret{PASS};

	DBG_ENTER("xmysqlnd_session_data::connect");

	SET_EMPTY_ERROR(error_info);

	DBG_INF_FMT(
		"host=%s user=%s db=%s port=%u flags=%u persistent=%u state=%u",
		auth->hostname.c_str(),
		auth->username.c_str(),
		def_schema.c_str(),
		port,
		static_cast<unsigned int>(set_capabilities),
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
	default_schema = util::to_std_string(def_schema);

	std::string transport_name{ get_scheme(auth->hostname, port) };

	if( transport_name.empty()) {
		ret = FAIL;
	} else {
		scheme = transport_name;

		if (scheme.empty()) {
			SET_OOM_ERROR(error_info);
			ret = FAIL;
		}
	}

	/* Attempt to connect */
	if( ret == PASS ) {
		ret = connect_handshake( scheme, def_schema,
								 set_capabilities);
		if( (ret != PASS) && (error_info->error_no == 0)) {
			SET_OOM_ERROR(error_info);
		}
	}

	/* Setup server host information */
	if( ret == PASS ) {
		state.set(SESSION_READY);
		transport_types transport = transport_type;

		switch(transport) {
			case transport_types::network:
				server_host_info = auth->hostname + " via TCP/IP";
				break;

			case transport_types::unix_domain_socket:
				server_host_info = "Localhost via UNIX socket";
				break;

			case transport_types::windows_pipe:
				server_host_info = socket_path + " via named pipe";
				break;

			default:
				assert(!"unknown transport!");
		}

		if ( server_host_info.empty() ) {
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
					scheme.c_str());

		if (!error_info->error_no) {
			SET_CLIENT_ERROR(error_info,
							 CR_CONNECTION_ERROR,
							 UNKNOWN_SQLSTATE,
							 error_info->error[0] ? error_info->error:"Unknown error");
			php_error_docref(nullptr, E_WARNING, "[%u] %.128s (trying to connect via %s)",
							 error_info->error_no, error_info->error, scheme.c_str());
		}
		cleanup();
		XMYSQLND_INC_SESSION_STATISTIC(stats, XMYSQLND_STAT_CONNECT_FAILURE);
	}

	DBG_RETURN(ret);
}

util::string
xmysqlnd_session_data::quote_name(const util::string_view& name)
{
	DBG_ENTER("xmysqlnd_session_data::quote_name");
	DBG_INF_FMT("name=%s", name.data());
	util::string ret;
	if (!name.empty()) {
		ret = '`';
		boost::replace_all_copy(
			std::back_inserter(ret),
			name,
			"`",
			"``");
		ret += '`';
	}
	DBG_RETURN(ret);
}

unsigned int
xmysqlnd_session_data::get_error_no()
{
	return error_info->error_no;
}

const char *
xmysqlnd_session_data::get_error_str()
{
	return error_info->error;
}

const char *
xmysqlnd_session_data::get_sqlstate()
{
	return error_info->sqlstate[0] ? error_info->sqlstate : MYSQLND_SQLSTATE_NULL;
}

const MYSQLND_ERROR_INFO*
xmysqlnd_session_data::get_error_info() const
{
	return error_info;
}

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

enum_func_status
xmysqlnd_session_data::send_reset(bool keep_open)
{
	DBG_ENTER("mysqlnd_send_reset");

	enum_func_status ret{ PASS };
	MYSQLND_VIO* vio{ io.vio };
	php_stream* net_stream{ vio->data->m.get_stream(vio) };
	const xmysqlnd_session_state state_val{ state.get() };

	DBG_INF_FMT("session=%p vio->data->stream->abstract=%p", this, net_stream ? net_stream->abstract : nullptr);
	DBG_INF_FMT("state=%u", state_val);

	switch (state_val) {
		case SESSION_ALLOCATED:
		case SESSION_CONNECTING:
			throw util::xdevapi_exception(
				util::xdevapi_exception::Code::connection_failure,
				"cannot reset, not connected");

		case SESSION_NON_AUTHENTICATED:
		case SESSION_READY:
		case SESSION_CLOSE_SENT: {
			st_xmysqlnd_message_factory msg_factory{ create_message_factory() };
			st_xmysqlnd_msg__session_reset conn_reset_msg{ msg_factory.get__session_reset(&msg_factory) };
			if (keep_open) {
				conn_reset_msg.keep_open.emplace(keep_open);
			}
			DBG_INF("Connection reset, sending SESS_RESET");
			if ((conn_reset_msg.send_request(&conn_reset_msg) != PASS)
				|| (conn_reset_msg.read_response(&conn_reset_msg) != PASS)) {
				throw util::xdevapi_exception(util::xdevapi_exception::Code::session_reset_failure);
			}

			bool session_still_opened{ (state_val == SESSION_READY) && keep_open };
			state.set(session_still_opened ? SESSION_READY : SESSION_NON_AUTHENTICATED);
			break;
		}

		case SESSION_CLOSED:
			throw util::xdevapi_exception(util::xdevapi_exception::Code::session_closed);
	}

	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_session_data::send_close()
{
	DBG_ENTER("mysqlnd_send_close");

	enum_func_status ret{PASS};
	MYSQLND_VIO* vio{ io.vio };
	const xmysqlnd_session_state state_val{ state.get() };

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
		st_xmysqlnd_message_factory msg_factory{ create_message_factory() };
		if ((state_val == SESSION_READY) && is_session_properly_supported()) {
			DBG_INF("Session clean, sending SESS_CLOSE");
			st_xmysqlnd_msg__session_close session_close_msg = msg_factory.get__session_close(&msg_factory);
			session_close_msg.send_request(&session_close_msg);
			session_close_msg.read_response(&session_close_msg);
		}

		DBG_INF("Connection clean, sending CON_CLOSE");
		st_xmysqlnd_msg__connection_close conn_close_msg = msg_factory.get__connection_close(&msg_factory);
		conn_close_msg.send_request(&conn_close_msg);
		conn_close_msg.read_response(&conn_close_msg);

		php_stream* net_stream{ vio->data->m.get_stream(vio) };
		DBG_INF_FMT("session=%p vio->data->stream->abstract=%p", this, net_stream? net_stream->abstract:nullptr);
		if (net_stream) {
			/* HANDLE COM_QUIT here */
			vio->data->m.close_stream(vio, stats, error_info);
		}
		state.set(SESSION_CLOSED);
		break;
	}
	case SESSION_ALLOCATED:
	case SESSION_CONNECTING:
	case SESSION_CLOSE_SENT:
		/* The user has killed its own connection */
		vio->data->m.close_stream(vio, stats, error_info);
		state.set(SESSION_CLOSED);
		break;
	case SESSION_CLOSED:
		// already closed, do nothing
		break;
	}

	DBG_RETURN(ret);
}

size_t
xmysqlnd_session_data::negotiate_client_api_capabilities(const size_t flags)
{
	size_t ret{0};
	DBG_ENTER("xmysqlnd_session_data::negotiate_client_api_capabilities");

	ret = client_api_capabilities;
	client_api_capabilities = flags;

	DBG_RETURN(ret);
}

bool xmysqlnd_session_data::is_session_properly_supported()
{
	if (session_properly_supported) return *session_properly_supported;

	st_xmysqlnd_message_factory msg_factory{ create_message_factory() };
	st_xmysqlnd_msg__expectations_open conn_expectations_open{ msg_factory.get__expectations_open(&msg_factory) };
	conn_expectations_open.condition_key = Mysqlx::Expect::Open_Condition::EXPECT_FIELD_EXIST;
	const char* field_keep_session_open{ "6.1" };
	conn_expectations_open.condition_value = field_keep_session_open;
	conn_expectations_open.condition_op = Mysqlx::Expect::Open_Condition::EXPECT_OP_SET;
	conn_expectations_open.send_request(&conn_expectations_open);
	conn_expectations_open.read_response(&conn_expectations_open);

	st_xmysqlnd_msg__expectations_close conn_expectations_close{ msg_factory.get__expectations_close(&msg_factory) };
	conn_expectations_close.send_request(&conn_expectations_close);
	conn_expectations_close.read_response(&conn_expectations_close);

	session_properly_supported.emplace(conn_expectations_open.result == st_xmysqlnd_msg__expectations_open::Result::ok);
	return *session_properly_supported;
}

size_t
xmysqlnd_session_data::get_client_id()
{
	return client_id;
}

const char* Auth_mechanism_mysql41 = "MYSQL41";
const char* Auth_mechanism_plain = "PLAIN";
const char* Auth_mechanism_external = "EXTERNAL";
const char* Auth_mechanism_sha256_memory = "SHA256_MEMORY";
const char* Auth_mechanism_unspecified = "";

const char* Tls_version_v1 = "TLSv1";
const char* Tls_version_v10 = "TLSv1.0";
const char* Tls_version_v11 = "TLSv1.1";
const char* Tls_version_v12 = "TLSv1.2";
const char* Tls_version_v13 = "TLSv1.3";

const st_xmysqlnd_session_query_bind_variable_bind noop__var_binder = { nullptr, nullptr };
const st_xmysqlnd_session_on_result_start_bind noop__on_result_start = { nullptr, nullptr };
const st_xmysqlnd_session_on_row_bind noop__on_row = { nullptr, nullptr };
const st_xmysqlnd_session_on_warning_bind noop__on_warning = { nullptr, nullptr };
const st_xmysqlnd_session_on_error_bind noop__on_error = { nullptr, nullptr };
const st_xmysqlnd_session_on_result_end_bind noop__on_result_end = { nullptr, nullptr };
const st_xmysqlnd_session_on_statement_ok_bind noop__on_statement_ok = { nullptr, nullptr };

enum xmysqlnd_session_state
st_xmysqlnd_session_state::get() const
{
	DBG_ENTER("xmysqlnd_session_state::get");
	DBG_INF_FMT("State=%u", state);
	DBG_RETURN(state);
}

void
st_xmysqlnd_session_state::set(const enum xmysqlnd_session_state new_state)
{
	DBG_ENTER("xmysqlnd_session_state::set");
	DBG_INF_FMT("New state=%u", state);
	state = new_state;
	DBG_VOID_RETURN;
}

st_xmysqlnd_session_state::st_xmysqlnd_session_state()
{
	DBG_ENTER("st_xmysqlnd_session_state constructor");
	state = SESSION_ALLOCATED;
}

xmysqlnd_session_data::xmysqlnd_session_data(
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
	MYSQLND_STATS* mysqlnd_stats,
	MYSQLND_ERROR_INFO* mysqlnd_error_info)
	: savepoint_name_seed(1)
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

	DBG_VOID_RETURN;
}

xmysqlnd_session_data::xmysqlnd_session_data(xmysqlnd_session_data&& rhs) noexcept
{
	object_factory = rhs.object_factory;
	io = std::move(rhs.io);
	auth = std::move(rhs.auth);
	auth_mechanisms = std::move(rhs.auth_mechanisms);
	scheme = std::move(rhs.scheme);
	default_schema = std::move(rhs.default_schema);
	transport_type = rhs.transport_type;
	socket_path = std::move(rhs.socket_path);
	server_host_info = std::move(rhs.server_host_info);
	compression_executor = std::move(rhs.compression_executor);
	client_id = rhs.client_id;
	rhs.client_id = 0;
	charset = rhs.charset;
	mysqlnd_error_info_init(&error_info_impl, persistent);
	error_info = &error_info_impl;
	state = rhs.state;
	client_api_capabilities = rhs.client_api_capabilities;

	stats = rhs.stats;
	rhs.stats = nullptr;
	own_stats = rhs.own_stats;
	rhs.own_stats = false;

	persistent = rhs.persistent;
	savepoint_name_seed = rhs.savepoint_name_seed;
	rhs.savepoint_name_seed = 0;
}

xmysqlnd_session_data::~xmysqlnd_session_data()
{
	DBG_ENTER("xmysqlnd_session_data::~xmysqlnd_session_data");
	send_close();
	cleanup();
	free_contents();
	DBG_VOID_RETURN;
}

void xmysqlnd_session_data::cleanup()
{
	DBG_ENTER("xmysqlnd_session_data::cleanup");

	if (io.pfc) {
		io.pfc->data->m.free_contents(io.pfc);
	}

	if (io.vio) {
		io.vio->data->m.free_contents(io.vio);
	}

	DBG_INF("Freeing memory of members");

	auth.reset();
	compression_executor.reset();
	default_schema.clear();
	scheme.clear();
	server_host_info.clear();
	util::zend::free_error_info_list(error_info, persistent);
	charset = nullptr;

	DBG_VOID_RETURN;
}

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
	capabilities.reset();
	DBG_VOID_RETURN;
}

// ----------------------------------------------------------------------------

void setup_crypto_options(
	php_stream_context* stream_context,
	xmysqlnd_session_data* session)
{
	// SSL context options: http://php.net/manual/en/context.ssl.php
	DBG_ENTER("setup_crypto_options");
	const Session_auth_data* auth{ session->auth.get() };
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

	// capath
	if (!auth->ssl_capath.empty()) {
		DBG_INF_FMT("Setting CA path %s", auth->ssl_capath.c_str());
		ZVAL_STRING(&string, auth->ssl_capath.c_str());
		php_stream_context_set_option(stream_context, "ssl", "capath", &string);
		zval_ptr_dtor(&string);
	}

	//Provide the list of supported/unsupported ciphers
	if (!auth->ssl_ciphers.empty()) {
		const std::string& cipher_list{ boost::join(auth->ssl_ciphers, ":") };
		ZVAL_STRING(&string, cipher_list.c_str());
		php_stream_context_set_option(stream_context,"ssl","ciphers",&string);
		zval_ptr_dtor(&string);
	}

	// is verification of SSL certificate required
	const SSL_mode ssl_mode{ auth->ssl_mode };
	const bool is_verify_peer_required{
		(ssl_mode == SSL_mode::verify_ca) || (ssl_mode == SSL_mode::verify_identity) };
	zval verify_peer;
	ZVAL_BOOL(&verify_peer, is_verify_peer_required);
	php_stream_context_set_option(stream_context, "ssl", "verify_peer", &verify_peer);
	DBG_INF_FMT("verify SSL certificate %d", static_cast<int>(is_verify_peer_required));

	// is verification of peer name required
	zval verify_peer_name;
	const bool is_verify_peer_name_required{ ssl_mode == SSL_mode::verify_identity };
	ZVAL_BOOL(&verify_peer_name, is_verify_peer_name_required);
	php_stream_context_set_option(stream_context, "ssl", "verify_peer_name", &verify_peer_name);
	DBG_INF_FMT("verify peer name %d", static_cast<int>(is_verify_peer_name_required));

	// allow self-signed certificates
	zval allow_self_signed;
	ZVAL_BOOL(&allow_self_signed, auth->ssl_allow_self_signed_cert);
	php_stream_context_set_option(stream_context, "ssl", "allow_self_signed", &allow_self_signed);
	DBG_INF_FMT("allow self-signed CA allow %d", static_cast<int>(auth->ssl_allow_self_signed_cert));

	DBG_VOID_RETURN;
}

php_stream_xport_crypt_method_t to_stream_crypt_method(Tls_version tls_version)
{
	using Tls_version_to_crypt_method = std::map<Tls_version, php_stream_xport_crypt_method_t>;
	static const Tls_version_to_crypt_method tls_version_to_crypt_method{
		{ Tls_version::unspecified, STREAM_CRYPTO_METHOD_TLS_ANY_CLIENT },
		{ Tls_version::tls_v1_0, STREAM_CRYPTO_METHOD_TLSv1_0_CLIENT },
		{ Tls_version::tls_v1_1, STREAM_CRYPTO_METHOD_TLSv1_1_CLIENT },
		{ Tls_version::tls_v1_2, STREAM_CRYPTO_METHOD_TLSv1_2_CLIENT },
		#ifdef TLSv13_IS_SUPPORTED
		{ Tls_version::tls_v1_3, STREAM_CRYPTO_METHOD_TLSv1_3_CLIENT },
		#endif
	};

	return tls_version_to_crypt_method.at(tls_version);
}

using Crypt_methods = util::vector<php_stream_xport_crypt_method_t>;

Crypt_methods prepare_crypt_methods(const Tls_versions& tls_versions)
{
	int tls_crypt_methods{ 0 };
	for (Tls_version tls_version : tls_versions) {
		php_stream_xport_crypt_method_t tls_crypt_method{ to_stream_crypt_method(tls_version) };
		tls_crypt_methods |= tls_crypt_method;
	}

	return { static_cast<php_stream_xport_crypt_method_t>(tls_crypt_methods) };
}

enum_func_status try_setup_crypto_connection(
	xmysqlnd_session_data* session,
	st_xmysqlnd_msg__capabilities_get& caps_get,
	st_xmysqlnd_message_factory& msg_factory,
	php_stream_xport_crypt_method_t crypt_method)
{
	DBG_ENTER("try_setup_crypto_connection");
	enum_func_status ret{FAIL};
	const st_xmysqlnd_on_error_bind on_error =
	{ xmysqlnd_session_data_handler_on_error, (void*) session };
	//Attempt to set the TLS capa. flag.
	st_xmysqlnd_msg__capabilities_set caps_set{	msg_factory.get__capabilities_set(&msg_factory) };

	zval ** capability_names = (zval **) mnd_ecalloc(2, sizeof(zval*));
	zval ** capability_values = (zval **) mnd_ecalloc(2, sizeof(zval*));
	zval  name, value;

	constexpr util::string_view cstr_name("tls");

	ZVAL_STRINGL(&name,cstr_name.data(),cstr_name.length());

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
			if (php_stream_xport_crypto_setup(net_stream, crypt_method, nullptr) < 0 ||
					php_stream_xport_crypto_enable(net_stream, 1) < 0)
			{
				DBG_ERR_FMT("Cannot connect to MySQL by using SSL");
				util::set_error_info(util::xdevapi_exception::Code::cannot_connect_by_ssl, session->error_info);
				ret = FAIL;
			} else {
				php_stream_context_set( net_stream, nullptr );
			}
		} else {
			DBG_ERR_FMT("Negative response from the server, not able to setup TLS.");
			util::set_error_info(util::xdevapi_exception::Code::cannot_setup_tls, session->error_info);
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

enum_func_status setup_crypto_connection(
	xmysqlnd_session_data* session,
	st_xmysqlnd_msg__capabilities_get& caps_get,
	st_xmysqlnd_message_factory& msg_factory)
{
	DBG_ENTER("setup_crypto_connection");
	Tls_versions tls_versions{ session->auth->tls_versions };
	if (tls_versions.empty()) {
		tls_versions.push_back(Tls_version::unspecified);
	}

	enum_func_status result{ FAIL };
	const Crypt_methods& crypt_methods{ prepare_crypt_methods(tls_versions) };
	for (php_stream_xport_crypt_method_t crypt_method : crypt_methods) {
		DBG_INF_FMT("setup_crypto_connection %d", static_cast<int>(crypt_method));
		result = try_setup_crypto_connection(session, caps_get, msg_factory, crypt_method);
		if (result == PASS) {
			break;
		}
	}

	DBG_RETURN(result);
}

// ----------------------------------------------------------------------------

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

const enum_hnd_func_status
xmysqlnd_session_data_handler_on_error(void * context, const unsigned int code, const util::string_view& sql_state, const util::string_view& message)
{
	xmysqlnd_session_data* session = static_cast<xmysqlnd_session_data*>(context);
	DBG_ENTER("xmysqlnd_stmt::handler_on_error");
	raise_session_error(session, code, sql_state.data(), message.data());
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

const enum_hnd_func_status
xmysqlnd_session_data_handler_on_auth_continue(
		void* context,
		const util::string_view& input,
		util::string* const output)
{
	DBG_ENTER("xmysqlnd_stmt::handler_on_auth_continue");

	const util::string_view salt{ input };
	DBG_INF_FMT("salt[%d]=%s", salt.length(), salt.data());

	Auth_plugin* auth_plugin{ static_cast<Auth_plugin*>(context) };
	*output = auth_plugin->prepare_continue_auth_data(salt);

	xmysqlnd_dump_string_to_log("output", output->c_str(), output->length());

	DBG_RETURN(HND_AGAIN);
}

enum_func_status
xmysqlnd_session_data_set_client_id(void * context, const size_t id)
{
	enum_func_status ret{FAIL};
	xmysqlnd_session_data * session = (xmysqlnd_session_data *) context;
	DBG_ENTER("xmysqlnd_session_data_set_client_id");
	DBG_INF_FMT("id=" MYSQLX_LLU_SPEC, id);
	if (context) {
		session->client_id = id;
		ret = PASS;
	}
	DBG_RETURN(ret);
}

const enum_hnd_func_status
on_muted_auth_warning(
		void* /*context*/,
		const xmysqlnd_stmt_warning_level level,
		const unsigned int code,
		const util::string_view& message)
{
	DBG_ENTER("on_muted_auth_warning");
	DBG_INF_FMT("[%4u] %d %s", code, level, message.data());
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

const enum_hnd_func_status
on_muted_auth_error(
		void* /*context*/,
		const unsigned int code,
		const util::string_view& sql_state,
		const util::string_view& message)
{
	DBG_ENTER("on_muted_auth_error");
	DBG_INF_FMT("[%4u][%s] %s", code, sql_state.data(), message.data());
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

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
		const util::string_view& salt,
		util::vector<char>& result)
{
	if (salt.empty()) return;

	if (calc_hash(salt)) {
		hex_hash(result);
	}
}

bool Auth_scrambler::calc_hash(const util::string_view& salt)
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
	void scramble(const util::string_view& salt) override;

};

// -------------

Mysql41_auth_scrambler::Mysql41_auth_scrambler(const Authentication_context& context)
	: Auth_scrambler(context, SHA1_MAX_LENGTH)
{
}

void Mysql41_auth_scrambler::scramble(const util::string_view& salt)
{
	/*
		CLIENT_HASH=xor(sha256(sha256(sha256(password)), nonce), sha256(password))
	*/
	const util::string& password{ context.password };
	php_mysqlnd_scramble(
				hash.data(),
				reinterpret_cast<const unsigned char*>(salt.data()),
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
	void scramble(const util::string_view& salt) override;

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

void Sha256_mem_auth_scrambler::scramble(const util::string_view& salt)
{
	const util::string& password{ context.password };
	hash_data(
		reinterpret_cast<const unsigned char*>(salt.data()),
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

util::string Auth_plugin_base::prepare_continue_auth_data(const util::string_view& /*salt*/)
{
	assert("call should never happen- method should be overriden!");
	return util::string();
}

void Auth_plugin_base::add_prefix_to_auth_data()
{
	// auth_data = "SCHEMA\0USER\0{custom_scramble_ending}"
	// prefix    = "SCHEMA\0USER\0"
	add_to_auth_data(context.default_schema);
	add_to_auth_data('\0');
	add_to_auth_data(context.username);
	add_to_auth_data('\0');
}

void Auth_plugin_base::add_scramble_to_auth_data(const util::string_view& salt)
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

	util::string prepare_continue_auth_data(const util::string_view& salt) override;

protected:
	std::unique_ptr<Auth_scrambler> get_scrambler() override;

};

Mysql41_auth_plugin::Mysql41_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_mysql41, context)
{
}

util::string Mysql41_auth_plugin::prepare_continue_auth_data(const util::string_view& salt)
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

	util::string prepare_continue_auth_data(const util::string_view& salt) override;

protected:
	std::unique_ptr<Auth_scrambler> get_scrambler() override;

};

Sha256_mem_auth_plugin::Sha256_mem_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_sha256_memory, context)
{
}

util::string Sha256_mem_auth_plugin::prepare_continue_auth_data(const util::string_view& salt)
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

	util::string prepare_continue_auth_data(const util::string_view& salt) override;

};

External_auth_plugin::External_auth_plugin(const Authentication_context& context)
	: Auth_plugin_base(Auth_mechanism_external, context)
{
}

util::string External_auth_plugin::prepare_continue_auth_data(const util::string_view& /*salt*/)
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
	const Session_auth_data* auth,
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
	const util::string_view& scheme,
	const util::string& def_schema)
	: session(session)
	, scheme(scheme)
	, default_schema(def_schema)
	, msg_factory(session->create_message_factory())
	, auth(session->auth.get())
{
	ZVAL_NULL(&capabilities);
}

Authenticate::~Authenticate()
{
}

bool Authenticate::run(bool re_auth)
{
	return re_auth ? run_re_auth() : run_auth();
}

bool Authenticate::run_auth()
{
	if (!init_capabilities()) return false;

	setup_compression();

	if (!init_connection()) return false;

	session->state.set(SESSION_NON_AUTHENTICATED);

	if (!gather_auth_mechanisms()) return false;

	session->auth_mechanisms = auth_mechanisms;

	return authentication_loop();
}

bool Authenticate::run_re_auth()
{
	auth_mechanisms = session->auth_mechanisms;
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

void Authenticate::setup_compression()
{
	const compression::Setup_data setup_data{
		auth->compression_policy,
		auth->compression_algorithms,
		msg_factory,
		capabilities
	};
	const compression::Configuration compression_cfg{
		compression::run_setup(setup_data)
	};
	session->compression_executor.reset(compression_cfg);
}

bool Authenticate::init_connection()
{
	const std::string capability_tls{ "tls" };
	zend_bool tls_set{FALSE};
	xmysqlnd_is_capability_present(&capabilities, capability_tls, &tls_set);

	if (auth->ssl_mode == SSL_mode::disabled) return true;

	if (tls_set) {
		return setup_crypto_connection(session, caps_get, msg_factory) == PASS;
	} else {
		php_error_docref(nullptr, E_WARNING, "Cannot connect to MySQL by using SSL, unsupported by the server");
		return false;
	}
}

zval Authenticate::get_capabilities()
{
	return capabilities;
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
		util::to_string(auth->username),
		util::to_string(auth->password),
		default_schema
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
		auth_mech_name,
		auth_data)
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

xmysqlnd_session::xmysqlnd_session(
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
	MYSQLND_STATS* stats,
	MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_session::xmysqlnd_session");

	session_uuid = std::make_unique<Uuid_generator>();
	xmysqlnd_session_data* session_data{ factory->get_session_data(factory, persistent, stats, error_info) };
	if (session_data) {
		data = std::shared_ptr<xmysqlnd_session_data>(session_data);
	}
}

xmysqlnd_session::xmysqlnd_session(xmysqlnd_session&& rhs) noexcept
{
	data = std::make_shared<xmysqlnd_session_data>(std::move(*rhs.data));
	server_version_string = std::move(rhs.server_version_string);
	session_uuid = std::move(rhs.session_uuid);
	pool_callback = rhs.pool_callback;
	rhs.pool_callback = nullptr;
	persistent = rhs.persistent;
}

xmysqlnd_session::~xmysqlnd_session()
{
	DBG_ENTER("xmysqlnd_session::~xmysqlnd_session");
	DBG_VOID_RETURN;
}

XMYSQLND_SESSION_DATA
xmysqlnd_session::get_data()
{
	return data;
}

const enum_func_status
xmysqlnd_session::connect(
	const util::string& default_schema,
	const unsigned int port,
	const size_t set_capabilities)
{
	enum_func_status ret{FAIL};

	DBG_ENTER("xmysqlnd_session::connect");
	ret = data->connect(default_schema, port, set_capabilities);
#ifdef WANTED_TO_PRECACHE_UUIDS_AT_CONNECT
	if (PASS == ret) {
		ret = precache_uuids();
	}
#endif
	DBG_RETURN(ret);
}

const enum_func_status
xmysqlnd_session::reset()
{
	DBG_ENTER("xmysqlnd_session::reset");
	bool keep_session_open{ get_data()->is_session_properly_supported() };
	enum_func_status ret{ get_data()->send_reset(keep_session_open) };
	bool need_reauth_after_reset{ !keep_session_open };
	if ((ret == PASS) && need_reauth_after_reset) {
		const util::string default_schema{ util::to_string(data->default_schema) };
		ret = data->authenticate(data->scheme, default_schema, 0, true);
	}
	DBG_RETURN(ret);
}

Uuid_format::Uuid_format() :
	clock_seq{ 0 },
	time_hi_and_version{ 0 },
	time_mid{ 0 },
	time_low{ 0 }
{
	node_id.fill( 0 );
}

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

Uuid_generator::Uuid_generator() :
	last_timestamp{ 0 }
{
	generate_session_node_info();
}

Uuid_format::uuid_t Uuid_generator::generate()
{
	Uuid_format uuid;
	assign_node_id( uuid );
	assign_timestamp( uuid );
	return uuid.get_uuid();
}

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

void Uuid_generator::assign_node_id( Uuid_format &uuid )
{
	uuid.node_id = session_node_id;
}

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

enum_func_status
xmysqlnd_session::xmysqlnd_schema_operation(const util::string_view& operation, const util::string_view& db)
{
	DBG_ENTER("xmysqlnd_schema_operation");
	DBG_INF_FMT("db=%s", db);
	enum_func_status ret{FAIL};
	if (!db.empty()) {
		const util::string& quoted_db = data->quote_name(db);
		const util::string schema_query = operation.data() + quoted_db;
		ret = query(namespace_sql, schema_query, noop__var_binder);
	}
	DBG_RETURN(ret);
}

const enum_func_status
xmysqlnd_session::select_db(const util::string_view& db)
{
	enum_func_status ret;
	constexpr std::string_view operation("USE ");
	DBG_ENTER("xmysqlnd_session::select_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}

const enum_func_status
xmysqlnd_session::create_db(const util::string_view& db)
{
	enum_func_status ret;
	constexpr std::string_view operation("CREATE DATABASE ");
	DBG_ENTER("xmysqlnd_session::create_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}

const enum_func_status
xmysqlnd_session::drop_db(const util::string_view& db)
{
	enum_func_status ret;
	constexpr util::string_view operation("DROP DATABASE ");
	DBG_ENTER("xmysqlnd_session::drop_db");
	ret = xmysqlnd_schema_operation( operation, db);
	DBG_RETURN(ret);
}

struct st_xmysqlnd_query_cb_ctx
{
	XMYSQLND_SESSION session;
	st_xmysqlnd_session_on_result_start_bind handler_on_result_start;
	st_xmysqlnd_session_on_row_bind handler_on_row;
	st_xmysqlnd_session_on_warning_bind handler_on_warning;
	st_xmysqlnd_session_on_error_bind handler_on_error;
	st_xmysqlnd_session_on_result_end_bind handler_on_result_end;
	st_xmysqlnd_session_on_statement_ok_bind handler_on_statement_ok;
};

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

const enum_hnd_func_status
query_cb_handler_on_warning(void * context,
							xmysqlnd_stmt * const stmt,
							const enum xmysqlnd_stmt_warning_level level,
							const unsigned int code,
							const util::string_view& message)
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

const enum_hnd_func_status
query_cb_handler_on_error(void * context,
						  xmysqlnd_stmt * const stmt,
						  const unsigned int code,
						  const util::string_view& sql_state,
						  const util::string_view& message)
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

const enum_func_status
xmysqlnd_session::query_cb(			const std::string_view& namespace_,
											const util::string_view& query,
											const st_xmysqlnd_session_query_bind_variable_bind var_binder,
											const st_xmysqlnd_session_on_result_start_bind handler_on_result_start,
											const st_xmysqlnd_session_on_row_bind handler_on_row,
											const st_xmysqlnd_session_on_warning_bind handler_on_warning,
											const st_xmysqlnd_session_on_error_bind handler_on_error,
											const st_xmysqlnd_session_on_result_end_bind handler_on_result_end,
											const st_xmysqlnd_session_on_statement_ok_bind handler_on_statement_ok)
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
			st_xmysqlnd_query_cb_ctx query_cb_ctx{
				session_handle,
				handler_on_result_start,
				handler_on_row,
				handler_on_warning,
				handler_on_error,
				handler_on_result_end,
				handler_on_statement_ok
			};
			const st_xmysqlnd_stmt_on_row_bind on_row{
				handler_on_row.handler? query_cb_handler_on_row : nullptr,
				&query_cb_ctx
			};
			const st_xmysqlnd_stmt_on_warning_bind on_warning{
				handler_on_warning.handler? query_cb_handler_on_warning : nullptr,
				&query_cb_ctx
			};
			const st_xmysqlnd_stmt_on_error_bind on_error{
				handler_on_error.handler? query_cb_handler_on_error : nullptr,
				&query_cb_ctx
			};
			const st_xmysqlnd_stmt_on_result_start_bind on_result_start{
				handler_on_result_start.handler? query_cb_handler_on_result_start : nullptr,
				&query_cb_ctx
			};
			const st_xmysqlnd_stmt_on_result_end_bind on_result_end{
				handler_on_result_end.handler? query_cb_handler_on_result_end : nullptr,
				&query_cb_ctx
			};
			const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok{
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

const enum_hnd_func_status
xmysqlnd_session_on_warning(
	void* /*context*/,
	xmysqlnd_stmt* const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int /*code*/,
	const util::string_view& /*message*/)
{
	DBG_ENTER("xmysqlnd_session_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.length(), message.data());
	DBG_RETURN(HND_AGAIN);
}

const enum_func_status
xmysqlnd_session::query(const std::string_view& namespace_,
										 const util::string_view& query,
										 const st_xmysqlnd_session_query_bind_variable_bind var_binder)
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
				const st_xmysqlnd_stmt_on_warning_bind on_warning = { xmysqlnd_session_on_warning, nullptr };
				const st_xmysqlnd_stmt_on_error_bind on_error = { nullptr, nullptr };
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

zend_ulong
xmysqlnd_session::get_server_version()
{
	DBG_ENTER("xmysqlnd_session::get_server_version");
	if (server_version_string.empty()) {
		constexpr util::string_view query("SELECT VERSION()");
		XMYSQLND_STMT_OP__EXECUTE * stmt_execute = xmysqlnd_stmt_execute__create(namespace_sql, query);
		XMYSQLND_SESSION session_handle(this);
		xmysqlnd_stmt * stmt = create_statement_object(session_handle);
		if (stmt && stmt_execute) {
			if (PASS == stmt->send_raw_message(stmt, xmysqlnd_stmt_execute__get_protobuf_message(stmt_execute), data->stats, data->error_info)) {
				const st_xmysqlnd_stmt_on_warning_bind on_warning = { nullptr, nullptr };
				const st_xmysqlnd_stmt_on_error_bind on_error = { nullptr, nullptr };
				zend_bool has_more{FALSE};
				XMYSQLND_STMT_RESULT * res = stmt->get_buffered_result(stmt, &has_more, on_warning, on_error, data->stats, data->error_info);
				if (res) {
					zval* set{nullptr};
					if (PASS == res->m.fetch_all_c(res, &set, FALSE /* don't duplicate, reference it */, data->stats, data->error_info) &&
							Z_TYPE(set[0 * 0]) == IS_STRING)
					{
						DBG_INF_FMT("Found %*s", Z_STRLEN(set[0 * 0]), Z_STRVAL(set[0 * 0]));
						server_version_string.assign(Z_STRVAL(set[0 * 0]), Z_STRLEN(set[0 * 0]));
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
		DBG_INF_FMT("server_version_string=%s", server_version_string.c_str());
	}
	if (server_version_string.empty()) {
		return 0;
	}

	std::vector<std::string> server_version_fragments;
	const char* Version_separator{ "." };
	boost::split(server_version_fragments, server_version_string, boost::is_any_of(Version_separator));

	if (server_version_fragments.size() != 3) {
		return 0;
	}

	zend_long major{ std::stol(server_version_fragments[0])};
	zend_long minor{ std::stol(server_version_fragments[1])};
	zend_long patch{ std::stol(server_version_fragments[2])};

	DBG_RETURN( (zend_ulong)(major * Z_L(10000) + (zend_ulong)(minor * Z_L(100) + patch)) );
}

xmysqlnd_stmt *
xmysqlnd_session::create_statement_object(XMYSQLND_SESSION session_handle)
{
	xmysqlnd_stmt* stmt{nullptr};
	DBG_ENTER("xmysqlnd_session::create_statement_object");
	stmt = xmysqlnd_stmt_create(session_handle, data->object_factory, data->stats, data->error_info);
	DBG_RETURN(stmt);
}

xmysqlnd_schema *
xmysqlnd_session::create_schema_object(const util::string_view& schema_name)
{
	xmysqlnd_schema* schema{nullptr};
	DBG_ENTER("xmysqlnd_session::create_schema_object");
	DBG_INF_FMT("schema_name=%s", schema_name.data());
	schema = xmysqlnd_schema_create(shared_from_this(), schema_name, data->object_factory, data->stats, data->error_info);

	DBG_RETURN(schema);
}

const enum_func_status
xmysqlnd_session::close(const enum_xmysqlnd_session_close_type close_type)
{
	DBG_ENTER("xmysqlnd_session::close");

	enum_func_status ret{FAIL};

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

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION
xmysqlnd_session_create(const size_t client_flags, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_session_create");
	auto session = object_factory->get_session(object_factory, persistent, stats, error_info);
	if (session && session->data) {
		session->data->negotiate_client_api_capabilities( client_flags);
	}
	XMYSQLND_SESSION session_ptr = std::shared_ptr<xmysqlnd_session>(session);
	session->data->ps_data.assign_session(session_ptr);
	DBG_RETURN(session_ptr);
}

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION
create_session(const bool persistent)
{
	DBG_ENTER("drv::create_session");
	const size_t client_flags{ 0 };
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory{
		MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory)
	};
	MYSQLND_STATS* stats{ nullptr };
	MYSQLND_ERROR_INFO* error_info{ nullptr };
	DBG_RETURN(xmysqlnd_session_create(client_flags, persistent, factory, stats, error_info));
}

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION
xmysqlnd_session_connect(XMYSQLND_SESSION session,
						 Session_auth_data * auth,
						 const util::string& default_schema,
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
				auth->hostname.c_str(), auth->username.c_str(), default_schema.c_str(),
				port, static_cast<unsigned long long>(set_capabilities));

	if (!session) {
		if (!(session = xmysqlnd_session_create(client_api_flags,
												TRUE, factory,
												stats, error_info))) {
			/* OOM */
			DBG_RETURN(nullptr);
		}
	}
	session->data->auth.reset(auth);
	ret = session->connect(default_schema, port, set_capabilities);

	if (ret == FAIL) {
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(session);
}

mysqlx::devapi::Session_data * create_new_session(zval * session_zval)
{
	DBG_ENTER("create_new_session");
	devapi::mysqlx_new_session(session_zval);
	auto& data_object{ util::fetch_data_object<devapi::Session_data>(session_zval) };
	DBG_RETURN(&data_object);
}

enum_func_status establish_connection(XMYSQLND_SESSION& session,
									  Session_auth_data * auth,
									  const util::Url& url,
									  transport_types tr_type)
{
	DBG_ENTER("establish_connection");
	enum_func_status ret{PASS};
	XMYSQLND_SESSION new_session;
	size_t set_capabilities{0};
	if( tr_type != transport_types::network ) {
		DBG_INF_FMT("Connecting with the provided socket/pipe: %s",
					url.host.c_str());
		if( url.host.empty() ) {
			//This should never happen!
			DBG_ERR_FMT("Expecting socket/pipe location, found nothing!");
			ret = FAIL;
		} else {
			session->data->socket_path = util::to_std_string(url.host);
		}
	}

	if( ret != FAIL ) {
		const util::string& default_schema{ url.path };
		session->data->transport_type = tr_type;
		new_session = xmysqlnd_session_connect(session,
											   auth,
											   default_schema,
											   url.port,
											   set_capabilities);
		if(new_session == nullptr) {
			ret = FAIL;
		}

		if (ret == PASS && session != new_session) {
			if (new_session) {
				php_error_docref(nullptr, E_WARNING, "Different object returned");
			}
			session = new_session;
		}
	}
	DBG_RETURN(ret);
}

/*
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

// ------------------------------------------------------------------------------

namespace {

class Extract_client_option
{
public:
	Extract_client_option(
		const util::string& option_name,
		const util::string& option_value,
		Session_auth_data* auth);

	void run();

	using Setter = void (Extract_client_option::*)(const std::string&);

public:
	static enum_func_status assign_ssl_mode(Session_auth_data& auth, SSL_mode ssl_mode);

private:
	void ensure_ssl_mode();
	void set_ssl_mode(const std::string& ssl_mode_str);
	SSL_mode parse_ssl_mode(const std::string& mode_str) const;
	void assign_ssl_mode(SSL_mode ssl_mode);
	void set_ssl_no_defaults(const std::string&);
	void set_auth_mechanism(const std::string& auth_mechanism_str);
	Auth_mechanism parse_auth_mechanism(const std::string& auth_mechanism_str) const;
	void assign_auth_mechanism(const Auth_mechanism auth_mechanism);
	void set_ssl_client_key(const std::string& ssl_client_key);
	void set_ssl_client_cert(const std::string& ssl_client_cert);
	void set_ssl_cafile(const std::string& ssl_cafile);
	void set_ssl_capath(const std::string& ssl_capath);
	void set_tls_versions(const std::string& raw_tls_versions);
	Tls_version parse_tls_version(const std::string& tls_version_str) const;
	void set_tls_ciphersuites(const std::string& raw_tls_ciphersuites);
	void set_ssl_ciphers(const std::string& ssl_ciphers);
	void set_connect_timeout(const std::string& timeout_str);
	void set_compression(const std::string& compression_str);
	void set_compression_algorithms(const std::string& compression_algorithms_str);

	util::std_strings parse_single_or_array(const std::string& value) const;
	bool parse_boolean(const std::string& value_str) const;
	int parse_int(const std::string& value_str) const;
	compression::Policy parse_compression_policy(const std::string& compression_policy_str) const;
	void verify_compression_algorithm(const std::string& algorithm_name) const;

private:
	const util::string& option_name;
	const util::string& option_value;
	Session_auth_data& auth;
};

// ------------------------------------------------------------------------------

struct Client_option_info
{
	enum Trait {
		None = 0x0,
		Requires_value = 0x1,
		Is_secure = 0x2
	};
	int traits;
	bool has_trait(Trait trait) const { return (traits & trait) == trait; }
	bool requires_value() const { return has_trait(Trait::Requires_value); }
	bool is_secure() const { return has_trait(Trait::Is_secure); }

	Extract_client_option::Setter setter;
};

// -----------------

Extract_client_option::Extract_client_option(
	const util::string& option_name,
	const util::string& option_value,
	Session_auth_data* auth)
	: option_name(option_name)
	, option_value(option_value)
	, auth(*auth)
{
}

void Extract_client_option::run()
{
	// https://dev.mysql.com/doc/refman/8.0/en/encrypted-connection-options.html
	using Option_to_info = std::map<const char*, Client_option_info, util::iless>;
	using Option_trait = Client_option_info::Trait;
	static const Option_to_info option_to_info{
		{ "ssl-mode", {Option_trait::Requires_value, &Extract_client_option::set_ssl_mode }},
		{ "ssl-no-defaults", {Option_trait::None, &Extract_client_option::set_ssl_no_defaults }},
		{ "auth", {Option_trait::Requires_value, &Extract_client_option::set_auth_mechanism }},
		{ "ssl-key", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_client_key }},
		{ "ssl-cert", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_client_cert }},
		{ "ssl-ca", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_cafile }},
		{ "ssl-capath", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_capath }},
		{ "ssl-cipher", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_ciphers }},
		{ "ssl-ciphers", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_ssl_ciphers }},
		{ "tls-version", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_tls_versions }},
		{ "tls-versions", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_tls_versions }},
		{ "tls-ciphersuite", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_tls_ciphersuites }},
		{ "tls-ciphersuites", {Option_trait::Is_secure | Option_trait::Requires_value, &Extract_client_option::set_tls_ciphersuites }},
		{ "connect-timeout", {Option_trait::Requires_value, &Extract_client_option::set_connect_timeout }},
		{ "compression", {Option_trait::Requires_value, &Extract_client_option::set_compression }},
		{ "compression-algorithms", {Option_trait::Requires_value, &Extract_client_option::set_compression_algorithms }},
	};

	auto it{ option_to_info.find(option_name.c_str()) };
	if (it == option_to_info.end()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::unknown_client_conn_option,
			option_name);
	}

	const Client_option_info& coi{it->second};
	if ((coi.requires_value()) && option_value.empty()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument,
			"The argument to " + option_name + " cannot be empty.");
	}

	auto setter{coi.setter};
	(this->*setter)(util::to_std_string(option_value));

	if (coi.is_secure()) {
		ensure_ssl_mode();
	}
}

// -----------------

enum_func_status Extract_client_option::assign_ssl_mode(Session_auth_data& auth, SSL_mode ssl_mode)
{
	DBG_ENTER("Extract_client_option::assign_ssl_mode");
	if (auth.ssl_mode == ssl_mode) {
		DBG_RETURN(PASS);
	}

	if (auth.ssl_mode == SSL_mode::not_specified) {
		DBG_INF_FMT("Selected mode: %d", static_cast<int>(ssl_mode));
		auth.ssl_mode = ssl_mode;
		DBG_RETURN(PASS);
	}

	if ((auth.ssl_mode == SSL_mode::any_secure) && (ssl_mode != SSL_mode::disabled)) {
		DBG_INF_FMT("Selected secure mode: %d", static_cast<int>(ssl_mode));
		auth.ssl_mode = ssl_mode;
		DBG_RETURN(PASS);
	}

	if ((auth.ssl_mode == SSL_mode::any_secure) && (ssl_mode == SSL_mode::disabled)) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::inconsistent_ssl_options,
			"cannot disable SSL connections when secure options are used");
		DBG_RETURN(FAIL);
	}

	const char* error_reason{ "Only one ssl mode is allowed." };
	DBG_ERR_FMT(error_reason);
	throw util::xdevapi_exception(
		util::xdevapi_exception::Code::inconsistent_ssl_options,
		error_reason);
	DBG_RETURN(FAIL);
}

void Extract_client_option::ensure_ssl_mode()
{
	// some SSL options provided, assuming 'required' mode if not specified yet
	if (auth.ssl_mode == SSL_mode::not_specified) {
		assign_ssl_mode(SSL_mode::any_secure);
	} else if (auth.ssl_mode == SSL_mode::disabled) {
		/*
			WL#10400 DevAPI: Ensure all Session connections are secure by default
			- if ssl-mode=disabled is used appearance of any ssl option
			such as ssl-ca would result in an error
			- inconsistent options such as  ssl-mode=disabled&ssl-ca=xxxx
			would result in an error returned to the user
		*/
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::inconsistent_ssl_options,
			"secure option '" + option_name + "' can not be specified when SSL connections are disabled");
	}
}

void Extract_client_option::set_ssl_mode(const std::string& ssl_mode_str)
{
	SSL_mode ssl_mode{ parse_ssl_mode(ssl_mode_str) };
	assign_ssl_mode(ssl_mode);
}

SSL_mode Extract_client_option::parse_ssl_mode(const std::string& mode_str) const
{
	using modestr_to_enum = std::map<std::string, SSL_mode, util::iless>;
	static const modestr_to_enum mode_mapping = {
		{ "required", SSL_mode::required },
		{ "disabled", SSL_mode::disabled },
		{ "verify_ca", SSL_mode::verify_ca },
		{ "verify_identity", SSL_mode::verify_identity }
	};
	auto it{ mode_mapping.find(mode_str) };
	if (it == mode_mapping.end()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::unknown_ssl_mode, mode_str);
	}
	return it->second;
}

void Extract_client_option::assign_ssl_mode(SSL_mode ssl_mode)
{
	assign_ssl_mode(auth, ssl_mode);
}

void Extract_client_option::set_ssl_no_defaults(const std::string&)
{
	auth.ssl_no_defaults = true;
}

void Extract_client_option::set_auth_mechanism(const std::string& auth_mechanism_str)
{
	const Auth_mechanism auth_mechanism{ parse_auth_mechanism(auth_mechanism_str) };
	assign_auth_mechanism(auth_mechanism);
}

Auth_mechanism Extract_client_option::parse_auth_mechanism(const std::string& auth_mechanism_str) const
{
	using str_to_auth_mechanism = std::map<std::string, Auth_mechanism, util::iless>;
	static const str_to_auth_mechanism str_to_auth_mechanisms{
		{ Auth_mechanism_mysql41, Auth_mechanism::mysql41 },
		{ Auth_mechanism_plain, Auth_mechanism::plain },
		{ Auth_mechanism_external, Auth_mechanism::external },
		{ Auth_mechanism_sha256_memory , Auth_mechanism::sha256_memory }
	};
	auto it{ str_to_auth_mechanisms.find(auth_mechanism_str) };
	if (it == str_to_auth_mechanisms.end()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::invalid_auth_mechanism, auth_mechanism_str);
	}
	return it->second;
}

void Extract_client_option::assign_auth_mechanism(const Auth_mechanism auth_mechanism)
{
	DBG_ENTER("set_auth_mechanism");
	if (auth.auth_mechanism == Auth_mechanism::unspecified) {
		DBG_INF_FMT("Selected authentication mechanism: %d", static_cast<int>(auth_mechanism));
		auth.auth_mechanism = auth_mechanism;
	} else if (auth.auth_mechanism != auth_mechanism) {
		const char* error_reason{ "only one authentication mechanism is allowed" };
		DBG_ERR_FMT("%s", error_reason);
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::invalid_auth_mechanism,
			error_reason);
	}
	DBG_VOID_RETURN;
}

void Extract_client_option::set_ssl_client_key(const std::string& ssl_client_key)
{
	auth.ssl_local_pk = ssl_client_key;
}

void Extract_client_option::set_ssl_client_cert(const std::string& ssl_client_cert)
{
	auth.ssl_local_cert = ssl_client_cert;
}

void Extract_client_option::set_ssl_cafile(const std::string& ssl_cafile)
{
	auth.ssl_cafile = ssl_cafile;
}

void Extract_client_option::set_ssl_capath(const std::string& ssl_capath)
{
	auth.ssl_capath = ssl_capath;
}

void Extract_client_option::set_tls_versions(const std::string& raw_tls_versions)
{
	const util::std_strings& tls_versions{ parse_single_or_array(raw_tls_versions) };
	if (tls_versions.empty()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::empty_tls_versions);
	}
	for (const auto& tls_version_str : tls_versions) {
		const Tls_version tls_version{ parse_tls_version(tls_version_str) };
		auth.tls_versions.push_back(tls_version);
	}
}

Tls_version Extract_client_option::parse_tls_version(const std::string& tls_version_str) const
{
	using name_to_protocol = std::map<std::string, Tls_version, util::iless>;
	static const name_to_protocol name_to_protocols{
		{ Tls_version_v1, Tls_version::tls_v1_0 },
		{ Tls_version_v10, Tls_version::tls_v1_0 },
		{ Tls_version_v11, Tls_version::tls_v1_1 },
		{ Tls_version_v12, Tls_version::tls_v1_2 },
		#ifdef TLSv13_IS_SUPPORTED
		{ Tls_version_v13, Tls_version::tls_v1_3 },
		#endif
	};
	auto it{ name_to_protocols.find(tls_version_str) };
	if (it != name_to_protocols.end()) return it->second;

	util::strings supported_protocols;
	std::transform(
		name_to_protocols.begin(),
		name_to_protocols.end(),
		std::back_inserter(supported_protocols),
		[](const auto& name_protocol) { return util::to_string(name_protocol.first); }
	);

	util::ostringstream os;
	os << util::quotation_if_blank(tls_version_str)
		<< " not recognized as a valid TLS protocol version (should be one of "
		<< boost::join(supported_protocols, ", ")
		<< ')';
	throw util::xdevapi_exception(
		util::xdevapi_exception::Code::unknown_tls_version,
		os.str());
}

void Extract_client_option::set_tls_ciphersuites(const std::string& raw_tls_ciphersuites)
{
	auth.tls_ciphersuites = parse_single_or_array(raw_tls_ciphersuites);
}

void Extract_client_option::set_ssl_ciphers(const std::string& raw_ssl_ciphers)
{
	auth.ssl_ciphers = parse_single_or_array(raw_ssl_ciphers);
}

void Extract_client_option::set_connect_timeout(const std::string& timeout_str)
{
	int timeout{ parse_int(timeout_str) };
	if (timeout < 0) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_timeout);
	}

	auth.connection_timeout = timeout;
}

void Extract_client_option::set_compression(const std::string& compression_policy_str)
{
	auth.compression_policy = parse_compression_policy(compression_policy_str);
}

void Extract_client_option::set_compression_algorithms(const std::string& compression_algorithms_str)
{
	util::std_strings compression_algorithms = parse_single_or_array(compression_algorithms_str);
	for (const auto& algorithm_name : compression_algorithms) {
		verify_compression_algorithm(algorithm_name);
	}
	auth.compression_algorithms = std::move(compression_algorithms);
}

// -----------------

util::std_strings Extract_client_option::parse_single_or_array(const std::string& value) const
{
	/*
		parse value of given option which can be single item or an array of items
		...?tls-versions=TLSv1.3&...
		...?tls-versions=[TLSv1.2,TLSv1.3]&...

		...?tls-ciphersuites=[
			TLS_DHE_PSK_WITH_AES_128_GCM_SHA256,
			TLS_CHACHA20_POLY1305_SHA256
			]&...
	*/
	assert(!value.empty());
	util::std_strings items;
	if ((value.front() == '[') && (value.back() == ']')) {
		const std::string contents(value.begin() + 1, value.end() - 1);
		if (!contents.empty()) {
			const char* Items_separator{ "," };
			boost::split(items, contents, boost::is_any_of(Items_separator));
		}
	} else {
		items.push_back(value);
	}
	std::for_each(
		items.begin(),
		items.end(),
		[](std::string& str){ boost::trim<std::string>(str); }
	);
	return items;
}

bool Extract_client_option::parse_boolean(const std::string& value_str) const
{
	static const std::map<std::string, bool, util::iless> valid_options{
		{"true", true},
		{"false", false},
		{"on", true},
		{"off", false},
		{"yes", true},
		{"no", false}
	};

	auto it{ valid_options.find(value_str) };
	if (it == valid_options.end()) {
		util::ostringstream os;
		os << "The argument to " << option_name
			<< " must be boolean, but it is '" << value_str.c_str() << "'.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	return it->second;
}

int Extract_client_option::parse_int(const std::string& value_str) const
{
	int value;
	if (!util::to_int(value_str, &value)) {
		util::ostringstream os;
		os << "The argument to " << option_name
			<< " must be an integer, but it is '" << value_str.c_str() << "'.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}
	return value;
}

compression::Policy Extract_client_option::parse_compression_policy(
	const std::string& compression_policy_str) const
{
	static const std::map<std::string, compression::Policy, util::iless> valid_policies{
		{"required", compression::Policy::required},
		{"preferred", compression::Policy::preferred},
		{"disabled", compression::Policy::disabled},
	};

	auto it{ valid_policies.find(compression_policy_str) };
	if (it == valid_policies.end()) {
		util::ostringstream os;
		os << "The connection property '"
			<< option_name
			<< "' acceptable values are: 'preferred', 'required', or 'disabled'. The value '"
			<< compression_policy_str.c_str()
			<< "' is not acceptable.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	return it->second;
}

void Extract_client_option::verify_compression_algorithm(const std::string& algorithm_name) const
{
	bool is_valid_name = std::all_of(
		algorithm_name.begin(),
		algorithm_name.end(),
		[](const char chr){ return std::isalnum(chr) || (chr == '_'); });

	if (!is_valid_name) {
		util::ostringstream os;
		os << util::quotation_if_blank(algorithm_name)
			<< " not recognized as a correct name of compression algorithm";
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::compression_invalid_algorithm_name,
			os.str());
	}
}

// -----------------

enum_func_status extract_client_option(
	const util::string& option_name,
	const util::string& option_value,
	Session_auth_data* auth)
{
	Extract_client_option extract_client_option(option_name, option_value, auth);
	extract_client_option.run();
	return PASS;
}

// ------------------------------------------------------------------------------

using Ciphersuites_to_ciphers = std::map<std::string, std::string>;

class Map_ciphersuites_to_ciphers
{
public:
	Map_ciphersuites_to_ciphers(Session_auth_data* auth_data);
	void run();

private:
	bool need_mapping() const;

private:
	static const Ciphersuites_to_ciphers ciphersuites_to_ciphers;
	const Tls_versions& tls_versions;
	const util::std_strings& tls_ciphersuites;
	util::std_strings& ssl_ciphers;
};

// source: https://www.openssl.org/docs/man1.0.2/man1/ciphers.html
// section: CIPHER SUITE NAMES
const Ciphersuites_to_ciphers Map_ciphersuites_to_ciphers::ciphersuites_to_ciphers{
	// SSL v3.0 cipher suites.
	{ "SSL_RSA_WITH_NULL_MD5", "NULL-MD5" },
	{ "SSL_RSA_WITH_NULL_SHA", "NULL-SHA" },
	{ "SSL_RSA_EXPORT_WITH_RC4_40_MD5", "EXP-RC4-MD5" },
	{ "SSL_RSA_WITH_RC4_128_MD5", "RC4-MD5" },
	{ "SSL_RSA_WITH_RC4_128_SHA", "RC4-SHA" },
	{ "SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5", "EXP-RC2-CBC-MD5" },
	{ "SSL_RSA_WITH_IDEA_CBC_SHA", "IDEA-CBC-SHA" },
	{ "SSL_RSA_EXPORT_WITH_DES40_CBC_SHA", "EXP-DES-CBC-SHA" },
	{ "SSL_RSA_WITH_DES_CBC_SHA", "DES-CBC-SHA" },
	{ "SSL_RSA_WITH_3DES_EDE_CBC_SHA", "DES-CBC3-SHA" },
	{ "SSL_DH_DSS_WITH_DES_CBC_SHA", "DH-DSS-DES-CBC-SHA" },
	{ "SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA", "DH-DSS-DES-CBC3-SHA" },
	{ "SSL_DH_RSA_WITH_DES_CBC_SHA", "DH-RSA-DES-CBC-SHA" },
	{ "SSL_DH_RSA_WITH_3DES_EDE_CBC_SHA", "DH-RSA-DES-CBC3-SHA" },
	{ "SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA", "EXP-EDH-DSS-DES-CBC-SHA" },
	{ "SSL_DHE_DSS_WITH_DES_CBC_SHA", "EDH-DSS-CBC-SHA" },
	{ "SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA", "EDH-DSS-DES-CBC3-SHA" },
	{ "SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA", "EXP-EDH-RSA-DES-CBC-SHA" },
	{ "SSL_DHE_RSA_WITH_DES_CBC_SHA", "EDH-RSA-DES-CBC-SHA" },
	{ "SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA", "EDH-RSA-DES-CBC3-SHA" },
	{ "SSL_DH_anon_EXPORT_WITH_RC4_40_MD5", "EXP-ADH-RC4-MD5" },
	{ "SSL_DH_anon_WITH_RC4_128_MD5", "ADH-RC4-MD5" },
	{ "SSL_DH_anon_EXPORT_WITH_DES40_CBC_SHA", "EXP-ADH-DES-CBC-SHA" },
	{ "SSL_DH_anon_WITH_DES_CBC_SHA", "ADH-DES-CBC-SHA" },
	{ "SSL_DH_anon_WITH_3DES_EDE_CBC_SHA", "ADH-DES-CBC3-SHA" },
	//SSL_FORTEZZA_KEA_WITH_NULL_SHA          Not implemented.
	//SSL_FORTEZZA_KEA_WITH_FORTEZZA_CBC_SHA  Not implemented.
	//SSL_FORTEZZA_KEA_WITH_RC4_128_SHA       Not implemented.

	// TLS v1.0 cipher suites.
	{ "TLS_RSA_WITH_NULL_MD5", "NULL-MD5" },
	{ "TLS_RSA_WITH_NULL_SHA", "NULL-SHA" },
	{ "TLS_RSA_EXPORT_WITH_RC4_40_MD5", "EXP-RC4-MD5" },
	{ "TLS_RSA_WITH_RC4_128_MD5", "RC4-MD5" },
	{ "TLS_RSA_WITH_RC4_128_SHA", "RC4-SHA" },
	{ "TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5", "EXP-RC2-CBC-MD5" },
	{ "TLS_RSA_WITH_IDEA_CBC_SHA", "IDEA-CBC-SHA" },
	{ "TLS_RSA_EXPORT_WITH_DES40_CBC_SHA", "EXP-DES-CBC-SHA" },
	{ "TLS_RSA_WITH_DES_CBC_SHA", "DES-CBC-SHA" },
	{ "TLS_RSA_WITH_3DES_EDE_CBC_SHA", "DES-CBC3-SHA" },
	//TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA    Not implemented.
	//TLS_DH_DSS_WITH_DES_CBC_SHA             Not implemented.
	//TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA        Not implemented.
	//TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA    Not implemented.
	//TLS_DH_RSA_WITH_DES_CBC_SHA             Not implemented.
	//TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA        Not implemented.
	{ "TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA", "EXP-EDH-DSS-DES-CBC-SHA" },
	{ "TLS_DHE_DSS_WITH_DES_CBC_SHA", "EDH-DSS-CBC-SHA" },
	{ "TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA", "EDH-DSS-DES-CBC3-SHA" },
	{ "TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA", "EXP-EDH-RSA-DES-CBC-SHA" },
	{ "TLS_DHE_RSA_WITH_DES_CBC_SHA", "EDH-RSA-DES-CBC-SHA" },
	{ "TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA", "EDH-RSA-DES-CBC3-SHA" },
	{ "TLS_DH_anon_EXPORT_WITH_RC4_40_MD5", "EXP-ADH-RC4-MD5" },
	{ "TLS_DH_anon_WITH_RC4_128_MD5", "ADH-RC4-MD5" },
	{ "TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA", "EXP-ADH-DES-CBC-SHA" },
	{ "TLS_DH_anon_WITH_DES_CBC_SHA", "ADH-DES-CBC-SHA" },
	{ "TLS_DH_anon_WITH_3DES_EDE_CBC_SHA", "ADH-DES-CBC3-SHA" },

	// AES ciphersuites from RFC3268, extending TLS v1.0
	{ "TLS_RSA_WITH_AES_128_CBC_SHA", "AES128-SHA" },
	{ "TLS_RSA_WITH_AES_256_CBC_SHA", "AES256-SHA" },
	{ "TLS_DH_DSS_WITH_AES_128_CBC_SHA", "DH-DSS-AES128-SHA" },
	{ "TLS_DH_DSS_WITH_AES_256_CBC_SHA", "DH-DSS-AES256-SHA" },
	{ "TLS_DH_RSA_WITH_AES_128_CBC_SHA", "DH-RSA-AES128-SHA" },
	{ "TLS_DH_RSA_WITH_AES_256_CBC_SHA", "DH-RSA-AES256-SHA" },
	{ "TLS_DHE_DSS_WITH_AES_128_CBC_SHA", "DHE-DSS-AES128-SHA" },
	{ "TLS_DHE_DSS_WITH_AES_256_CBC_SHA", "DHE-DSS-AES256-SHA" },
	{ "TLS_DHE_RSA_WITH_AES_128_CBC_SHA", "DHE-RSA-AES128-SHA" },
	{ "TLS_DHE_RSA_WITH_AES_256_CBC_SHA", "DHE-RSA-AES256-SHA" },
	{ "TLS_DH_anon_WITH_AES_128_CBC_SHA", "ADH-AES128-SHA" },
	{ "TLS_DH_anon_WITH_AES_256_CBC_SHA", "ADH-AES256-SHA" },

	// Camellia ciphersuites from RFC4132, extending TLS v1.0
	{ "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA", "CAMELLIA128-SHA" },
	{ "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA", "CAMELLIA256-SHA" },
	{ "TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA", "DH-DSS-CAMELLIA128-SHA" },
	{ "TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA", "DH-DSS-CAMELLIA256-SHA" },
	{ "TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA", "DH-RSA-CAMELLIA128-SHA" },
	{ "TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA", "DH-RSA-CAMELLIA256-SHA" },
	{ "TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA", "DHE-DSS-CAMELLIA128-SHA" },
	{ "TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA", "DHE-DSS-CAMELLIA256-SHA" },
	{ "TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA", "DHE-RSA-CAMELLIA128-SHA" },
	{ "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA", "DHE-RSA-CAMELLIA256-SHA" },
	{ "TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA", "ADH-CAMELLIA128-SHA" },
	{ "TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA", "ADH-CAMELLIA256-SHA" },

	// SEED ciphersuites from RFC4162, extending TLS v1.0
	{ "TLS_RSA_WITH_SEED_CBC_SHA", "SEED-SHA" },
	{ "TLS_DH_DSS_WITH_SEED_CBC_SHA", "DH-DSS-SEED-SHA" },
	{ "TLS_DH_RSA_WITH_SEED_CBC_SHA", "DH-RSA-SEED-SHA" },
	{ "TLS_DHE_DSS_WITH_SEED_CBC_SHA", "DHE-DSS-SEED-SHA" },
	{ "TLS_DHE_RSA_WITH_SEED_CBC_SHA", "DHE-RSA-SEED-SHA" },
	{ "TLS_DH_anon_WITH_SEED_CBC_SHA", "ADH-SEED-SHA" },

	// GOST ciphersuites from draft-chudov-cryptopro-cptls, extending TLS v1.0
	// Note: these ciphers require an engine which including GOST cryptographic algorithms, such as the ccgost engine, included in the OpenSSL distribution.
	{ "TLS_GOSTR341094_WITH_28147_CNT_IMIT", "GOST94-GOST89-GOST89" },
	{ "TLS_GOSTR341001_WITH_28147_CNT_IMIT", "GOST2001-GOST89-GOST89" },
	{ "TLS_GOSTR341094_WITH_NULL_GOSTR3411", "GOST94-NULL-GOST94" },
	{ "TLS_GOSTR341001_WITH_NULL_GOSTR3411", "GOST2001-NULL-GOST94" },

	// Additional Export 1024 and other cipher suites
	// Note: these ciphers can also be used in SSL v3.
	{ "TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA", "EXP1024-DES-CBC-SHA" },
	{ "TLS_RSA_EXPORT1024_WITH_RC4_56_SHA", "EXP1024-RC4-SHA" },
	{ "TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA", "EXP1024-DHE-DSS-DES-CBC-SHA" },
	{ "TLS_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA", "EXP1024-DHE-DSS-RC4-SHA" },
	{ "TLS_DHE_DSS_WITH_RC4_128_SHA", "DHE-DSS-RC4-SHA" },

	// Elliptic curve cipher suites.
	{ "TLS_ECDH_RSA_WITH_NULL_SHA", "ECDH-RSA-NULL-SHA" },
	{ "TLS_ECDH_RSA_WITH_RC4_128_SHA", "ECDH-RSA-RC4-SHA" },
	{ "TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA", "ECDH-RSA-DES-CBC3-SHA" },
	{ "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA", "ECDH-RSA-AES128-SHA" },
	{ "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA", "ECDH-RSA-AES256-SHA" },
	{ "TLS_ECDH_ECDSA_WITH_NULL_SHA", "ECDH-ECDSA-NULL-SHA" },
	{ "TLS_ECDH_ECDSA_WITH_RC4_128_SHA", "ECDH-ECDSA-RC4-SHA" },
	{ "TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA", "ECDH-ECDSA-DES-CBC3-SHA" },
	{ "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA", "ECDH-ECDSA-AES128-SHA" },
	{ "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA", "ECDH-ECDSA-AES256-SHA" },
	{ "TLS_ECDHE_RSA_WITH_NULL_SHA", "ECDHE-RSA-NULL-SHA" },
	{ "TLS_ECDHE_RSA_WITH_RC4_128_SHA", "ECDHE-RSA-RC4-SHA" },
	{ "TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA", "ECDHE-RSA-DES-CBC3-SHA" },
	{ "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA", "ECDHE-RSA-AES128-SHA" },
	{ "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA", "ECDHE-RSA-AES256-SHA" },
	{ "TLS_ECDHE_ECDSA_WITH_NULL_SHA", "ECDHE-ECDSA-NULL-SHA" },
	{ "TLS_ECDHE_ECDSA_WITH_RC4_128_SHA", "ECDHE-ECDSA-RC4-SHA" },
	{ "TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA", "ECDHE-ECDSA-DES-CBC3-SHA" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA", "ECDHE-ECDSA-AES128-SHA" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA", "ECDHE-ECDSA-AES256-SHA" },
	{ "TLS_ECDH_anon_WITH_NULL_SHA", "AECDH-NULL-SHA" },
	{ "TLS_ECDH_anon_WITH_RC4_128_SHA", "AECDH-RC4-SHA" },
	{ "TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA", "AECDH-DES-CBC3-SHA" },
	{ "TLS_ECDH_anon_WITH_AES_128_CBC_SHA", "AECDH-AES128-SHA" },
	{ "TLS_ECDH_anon_WITH_AES_256_CBC_SHA", "AECDH-AES256-SHA" },

	//TLS v1.2 cipher suites
	{ "TLS_RSA_WITH_NULL_SHA256", "NULL-SHA256" },
	{ "TLS_RSA_WITH_AES_128_CBC_SHA256", "AES128-SHA256" },
	{ "TLS_RSA_WITH_AES_256_CBC_SHA256", "AES256-SHA256" },
	{ "TLS_RSA_WITH_AES_128_GCM_SHA256", "AES128-GCM-SHA256" },
	{ "TLS_RSA_WITH_AES_256_GCM_SHA384", "AES256-GCM-SHA384" },
	{ "TLS_DH_RSA_WITH_AES_128_CBC_SHA256", "DH-RSA-AES128-SHA256" },
	{ "TLS_DH_RSA_WITH_AES_256_CBC_SHA256", "DH-RSA-AES256-SHA256" },
	{ "TLS_DH_RSA_WITH_AES_128_GCM_SHA256", "DH-RSA-AES128-GCM-SHA256" },
	{ "TLS_DH_RSA_WITH_AES_256_GCM_SHA384", "DH-RSA-AES256-GCM-SHA384" },
	{ "TLS_DH_DSS_WITH_AES_128_CBC_SHA256", "DH-DSS-AES128-SHA256" },
	{ "TLS_DH_DSS_WITH_AES_256_CBC_SHA256", "DH-DSS-AES256-SHA256" },
	{ "TLS_DH_DSS_WITH_AES_128_GCM_SHA256", "DH-DSS-AES128-GCM-SHA256" },
	{ "TLS_DH_DSS_WITH_AES_256_GCM_SHA384", "DH-DSS-AES256-GCM-SHA384" },
	{ "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256", "DHE-RSA-AES128-SHA256" },
	{ "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256", "DHE-RSA-AES256-SHA256" },
	{ "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256", "DHE-RSA-AES128-GCM-SHA256" },
	{ "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384", "DHE-RSA-AES256-GCM-SHA384" },
	{ "TLS_DHE_DSS_WITH_AES_128_CBC_SHA256", "DHE-DSS-AES128-SHA256" },
	{ "TLS_DHE_DSS_WITH_AES_256_CBC_SHA256", "DHE-DSS-AES256-SHA256" },
	{ "TLS_DHE_DSS_WITH_AES_128_GCM_SHA256", "DHE-DSS-AES128-GCM-SHA256" },
	{ "TLS_DHE_DSS_WITH_AES_256_GCM_SHA384", "DHE-DSS-AES256-GCM-SHA384" },
	{ "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256", "ECDH-RSA-AES128-SHA256" },
	{ "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384", "ECDH-RSA-AES256-SHA384" },
	{ "TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256", "ECDH-RSA-AES128-GCM-SHA256" },
	{ "TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384", "ECDH-RSA-AES256-GCM-SHA384" },
	{ "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256", "ECDH-ECDSA-AES128-SHA256" },
	{ "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384", "ECDH-ECDSA-AES256-SHA384" },
	{ "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256", "ECDH-ECDSA-AES128-GCM-SHA256" },
	{ "TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384", "ECDH-ECDSA-AES256-GCM-SHA384" },
	{ "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256", "ECDHE-RSA-AES128-SHA256" },
	{ "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384", "ECDHE-RSA-AES256-SHA384" },
	{ "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256", "ECDHE-RSA-AES128-GCM-SHA256" },
	{ "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384", "ECDHE-RSA-AES256-GCM-SHA384" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256", "ECDHE-ECDSA-AES128-SHA256" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384", "ECDHE-ECDSA-AES256-SHA384" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256", "ECDHE-ECDSA-AES128-GCM-SHA256" },
	{ "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384", "ECDHE-ECDSA-AES256-GCM-SHA384" },
	{ "TLS_DH_anon_WITH_AES_128_CBC_SHA256", "ADH-AES128-SHA256" },
	{ "TLS_DH_anon_WITH_AES_256_CBC_SHA256", "ADH-AES256-SHA256" },
	{ "TLS_DH_anon_WITH_AES_128_GCM_SHA256", "ADH-AES128-GCM-SHA256" },
	{ "TLS_DH_anon_WITH_AES_256_GCM_SHA384", "ADH-AES256-GCM-SHA384" },

	// Pre shared keying (PSK) cipheruites
	{ "TLS_PSK_WITH_RC4_128_SHA", "PSK-RC4-SHA" },
	{ "TLS_PSK_WITH_3DES_EDE_CBC_SHA", "PSK-3DES-EDE-CBC-SHA" },
	{ "TLS_PSK_WITH_AES_128_CBC_SHA", "PSK-AES128-CBC-SHA" },
	{ "TLS_PSK_WITH_AES_256_CBC_SHA", "PSK-AES256-CBC-SHA" },

	// Deprecated SSL v2.0 cipher suites.
	{ "SSL_CK_RC4_128_WITH_MD5", "RC4-MD5" },
	//SSL_CK_RC4_128_EXPORT40_WITH_MD5        Not implemented.
	{ "SSL_CK_RC2_128_CBC_WITH_MD5", "RC2-CBC-MD5" },
	//SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5    Not implemented.
	{ "SSL_CK_IDEA_128_CBC_WITH_MD5", "IDEA-CBC-MD5" },
	//SSL_CK_DES_64_CBC_WITH_MD5              Not implemented.
	{ "SSL_CK_DES_192_EDE3_CBC_WITH_MD5", "DES-CBC3-MD5" },
};

// -------------------------------

Map_ciphersuites_to_ciphers::Map_ciphersuites_to_ciphers(Session_auth_data* auth_data)
	: tls_versions(auth_data->tls_versions)
	, tls_ciphersuites(auth_data->tls_ciphersuites)
	, ssl_ciphers(auth_data->ssl_ciphers)
{
}

void Map_ciphersuites_to_ciphers::run()
{
	if (!need_mapping()) return;

	for (const std::string& tls_ciphersuite : tls_ciphersuites) {
		auto it{ciphersuites_to_ciphers.find(tls_ciphersuite)};
		if (it == ciphersuites_to_ciphers.end()) continue;
		const std::string& aligned_cipher{ it->second };
		ssl_ciphers.push_back(aligned_cipher);
	}
}

bool Map_ciphersuites_to_ciphers::need_mapping() const
{
	if (tls_ciphersuites.empty()) return false;

	// if user passes ciphers explicitly, then don't add/overwrite anything
	if (!ssl_ciphers.empty()) return false;

	/*
		OpenSSL for TLS earlier than v1.3 doesn't support ciphersuites, so in
		case user allows also older TLS versions (including v1.2 or earlier) the
		ciphersuites have to be mapped to ciphers
	*/
	return std::any_of(
		tls_versions.begin(),
		tls_versions.end(),
		[](const Tls_version& tls_ver){
			return (tls_ver == Tls_version::unspecified)
				|| (tls_ver == Tls_version::tls_v1_0)
				|| (tls_ver == Tls_version::tls_v1_1)
				|| (tls_ver == Tls_version::tls_v1_2);
		}
	);
}

// -------------------------------

void map_ciphersuites_to_ciphers(Session_auth_data* auth_data)
{
	Map_ciphersuites_to_ciphers map_ciphers(auth_data);
	map_ciphers.run();
}

// ------------------------------------------------------------------------------

class Filter_ciphers
{
public:
	Filter_ciphers(Session_auth_data* auth_data);
	void run();

private:
	void process_ciphersuites();
	void filter_ciphersuites();
	bool is_ciphersuite_forbidden(const std::string& ciphersuite) const;
	bool need_default_ciphersuites() const;

	void process_ciphers();
	void filter_ciphers();
	bool is_cipher_forbidden(const std::string& cipher) const;
	bool need_default_ciphers() const;

private:
	static const util::std_strings allowed_ciphersuites;
	static const util::std_strings allowed_ciphers;

	const Tls_versions& tls_versions;
	util::std_strings& tls_ciphersuites;
	util::std_strings& ssl_ciphers;
};

// -------------------------------

/*
	list according to specification, section "Acceptable ciphers":
	"The only acceptable cipher suites are these listed in the OSSA document [*] as
	either "mandatory", "approved" or "deprecated". The document also contains a
	list of "unacceptable" cipher suites, but this is there only for the reference -
	the OSSA group clarified that checking of acceptable cipher suites should be
	done using white-list approach, not black-list."

	link:
	https://confluence.oraclecorp.com/confluence/display/GPS/Approved+Security+Technologies%3A+Standards+-+TLS+Ciphers+and+Versions
*/
const util::std_strings Filter_ciphers::allowed_ciphers
{
	// ---------------------
	// Mandatory
	// ---------------------
	// P1 TLSv1.2
	"ECDHE-ECDSA-AES128-GCM-SHA256",

	// P1 TLSv1.2
	"ECDHE-ECDSA-AES256-GCM-SHA384",

	// P1 TLSv1.2
	"ECDHE-RSA-AES128-GCM-SHA256",

	// P1 TLSv1.2
	"ECDHE-ECDSA-AES128-SHA256",

	// P1 TLSv1.2
	"ECDHE-RSA-AES128-SHA256",


	// ---------------------
	// Approved
	// ---------------------
	// A1 TLSv1.3
	"TLS_AES_128_GCM_SHA256",

	// A1 TLSv1.3
	"TLS_AES_256_GCM_SHA384",

	// A1 TLSv1.3
	"TLS_CHACHA20_POLY1305_SHA256",

	// A1 TLSv1.3
	"TLS_AES_128_CCM_SHA256",

	// A1 TLSv1.3
	"TLS_AES_128_CCM_8_SHA256",

	// A1 TLSv1.2
	"ECDHE-RSA-AES256-GCM-SHA384",

	// A1 TLSv1.2
	"ECDHE-ECDSA-AES256-SHA384",

	// A1 TLSv1.2
	"ECDHE-RSA-AES256-SHA384",

	// A1 TLSv1.2
	"DHE-RSA-AES128-GCM-SHA256",

	// A1 TLSv1.2
	"DHE-DSS-AES128-GCM-SHA256",

	// A1 TLSv1.2
	"DHE-RSA-AES128-SHA256",

	// A1 TLSv1.2
	"DHE-DSS-AES128-SHA256",

	// A1 TLSv1.2
	"DHE-DSS-AES256-GCM-SHA384",

	// A1 TLSv1.2
	"DHE-RSA-AES256-SHA256",

	// A1 TLSv1.2
	"DHE-DSS-AES256-SHA256",

	// A1 TLSv1.2
	"DHE-RSA-AES256-GCM-SHA384",

	// A1 TLSv1.2
	"ECDHE-ECDSA-CHACHA20-POLY1305",

	// A1 TLSv1.2
	"ECDHE-RSA-CHACHA20-POLY1305",

	// A2 TLSv1.2
	"DH-DSS-AES128-GCM-SHA256",

	// A2 TLSv1.2
	"ECDH-ECDSA-AES128-GCM-SHA256",

	// A2 TLSv1.2
	"DH-DSS-AES256-GCM-SHA384",

	// A2 TLSv1.2
	"ECDH-ECDSA-AES256-GCM-SHA384",

	// A2 TLSv1.2
	"DH-DSS-AES128-SHA256",

	// A2 TLSv1.2
	"ECDH-ECDSA-AES128-SHA256",

	// A2 TLSv1.2
	"DH-DSS-AES256-SHA256",

	// A2 TLSv1.2
	"ECDH-ECDSA-AES256-SHA384",

	// A2 TLSv1.2
	"DH-RSA-AES128-GCM-SHA256",

	// A2 TLSv1.2
	"ECDH-RSA-AES128-GCM-SHA256",

	// A2 TLSv1.2
	"DH-RSA-AES256-GCM-SHA384",

	// A2 TLSv1.2
	"ECDH-RSA-AES256-GCM-SHA384",

	// A2 TLSv1.2
	"DH-RSA-AES128-SHA256",

	// A2 TLSv1.2
	"ECDH-RSA-AES128-SHA256",

	// A2 TLSv1.2
	"DH-RSA-AES256-SHA256",

	// A2 TLSv1.2
	"ECDH-RSA-AES256-SHA384",


	// ---------------------
	// Deprecated
	// ---------------------
	// D1 TLSv1.2, TLSv1.1
	"ECDHE-RSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDHE-ECDSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDHE-RSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDHE-ECDSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DHE-DSS-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DHE-RSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DHE-DSS-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DHE-RSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DH-DSS-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDH-ECDSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DH-DSS-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDH-ECDSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DH-RSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDH-RSA-AES128-SHA",

	// D1 TLSv1.2, TLSv1.1
	"DH-RSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"ECDH-RSA-AES256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"CAMELLIA256-SHA",

	// D1 TLSv1.2, TLSv1.1
	"CAMELLIA128-SHA",

	// D1 TLSv1.2
	"AES128-GCM-SHA256",

	// D1 TLSv1.2
	"AES256-GCM-SHA384",

	// D1 TLSv1.2
	"AES128-SHA256",

	// D1 TLSv1.2
	"AES256-SHA256",

	// D1 TLSv1.2
	"AES128-SHA",


	// D2 TLSv1.2, TLSv1.1
	// "N/A", unavailable - only ciphersuite counterpart available

	// D2 TLSv1.2, TLSv1.1
	// "N/A", unavailable - only ciphersuite counterpart available

	// D2 TLSv1.2, TLSv1.1
	"DHE-DSS-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"DHE-RSA-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"ECDH-RSA-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"ECDH-ECDSA-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"ECDHE-RSA-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"ECDHE-ECDSA-DES-CBC3-SHA",

	// D2 TLSv1.2, TLSv1.1
	"DES-CBC3-SHA",

	// D3 TLSv1.0
	// All approved ciphers - this includes all ciphers from the Mandatory and
	// Approved categories when used with TLSv1.0
}; // allowed_ciphers

// -------------------------------

// more details in comment to above Filter_ciphers::allowed_ciphers
const util::std_strings Filter_ciphers::allowed_ciphersuites
{
	// ---------------------
	// Mandatory
	// ---------------------
	// P1 TLSv1.2
	"TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",

	// P1 TLSv1.2
	"TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384",

	// P1 TLSv1.2
	"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",

	// P1 TLSv1.2
	"TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256",

	// P1 TLSv1.2
	"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256",


	// ---------------------
	// Approved
	// ---------------------
	// A1 TLSv1.3
	"TLS_AES_128_GCM_SHA256",

	// A1 TLSv1.3
	"TLS_AES_256_GCM_SHA384",

	// A1 TLSv1.3
	"TLS_CHACHA20_POLY1305_SHA256",

	// A1 TLSv1.3
	"TLS_AES_128_CCM_SHA256",

	// A1 TLSv1.3
	"TLS_AES_128_CCM_8_SHA256",

	// A1 TLSv1.2
	"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",

	// A1 TLSv1.2
	"TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384",

	// A1 TLSv1.2
	"TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384",

	// A1 TLSv1.2
	"TLS_DHE_RSA_WITH_AES_128_GCM_SHA256",

	// A1 TLSv1.2
	"TLS_DHE_DSS_WITH_AES_128_GCM_SHA256",


	// A1 TLSv1.2
	"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256",

	// A1 TLSv1.2
	"TLS_DHE_DSS_WITH_AES_128_CBC_SHA256",

	// A1 TLSv1.2
	"TLS_DHE_DSS_WITH_AES_256_GCM_SHA384",

	// A1 TLSv1.2
	"TLS_DHE_RSA_WITH_AES_256_CBC_SHA256",

	// A1 TLSv1.2
	"TLS_DHE_DSS_WITH_AES_256_CBC_SHA256",

	// A1 TLSv1.2
	"TLS_DHE_RSA_WITH_AES_256_GCM_SHA384",

	// A1 TLSv1.2
	"TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256",

	// A1 TLSv1.2
	"TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256",

	// A2 TLSv1.2
	"TLS_DH_DSS_WITH_AES_128_GCM_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256",

	// A2 TLSv1.2
	"TLS_DH_DSS_WITH_AES_256_GCM_SHA384",

	// A2 TLSv1.2
	"TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384",

	// A2 TLSv1.2
	"TLS_DH_DSS_WITH_AES_128_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_DH_DSS_WITH_AES_256_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384",

	// A2 TLSv1.2
	"TLS_DH_RSA_WITH_AES_128_GCM_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256",

	// A2 TLSv1.2
	"TLS_DH_RSA_WITH_AES_256_GCM_SHA384",

	// A2 TLSv1.2
	"TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384",

	// A2 TLSv1.2
	"TLS_DH_RSA_WITH_AES_128_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_DH_RSA_WITH_AES_256_CBC_SHA256",

	// A2 TLSv1.2
	"TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384",


	// ---------------------
	// Deprecated
	// ---------------------
	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DHE_DSS_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DHE_RSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DHE_DSS_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DHE_RSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DH_DSS_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_RSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DH_DSS_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DH_RSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDH_RSA_WITH_AES_128_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_DH_RSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_ECDH_RSA_WITH_AES_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_RSA_WITH_CAMELLIA_256_CBC_SHA",

	// D1 TLSv1.2, TLSv1.1
	"TLS_RSA_WITH_CAMELLIA_128_CBC_SHA",

	// D1 TLSv1.2
	"TLS_RSA_WITH_AES_128_GCM_SHA256",

	// D1 TLSv1.2
	"TLS_RSA_WITH_AES_256_GCM_SHA384",

	// D1 TLSv1.2
	"TLS_RSA_WITH_AES_128_CBC_SHA256",

	// D1 TLSv1.2
	"TLS_RSA_WITH_AES_256_CBC_SHA256",

	// D1 TLSv1.2
	"TLS_RSA_WITH_AES_128_CBC_SHA",


	// D2 TLSv1.2, TLSv1.1
	"TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA",

	// D2 TLSv1.2, TLSv1.1
	"TLS_RSA_WITH_3DES_EDE_CBC_SHA",

	// D3 TLSv1.0
	// All approved ciphers - this includes all ciphers from the Mandatory and
	// Approved categories when used with TLSv1.0
}; // allowed_ciphersuites

// -------------------------------

Filter_ciphers::Filter_ciphers(Session_auth_data* auth_data)
	: tls_versions(auth_data->tls_versions)
	, tls_ciphersuites(auth_data->tls_ciphersuites)
	, ssl_ciphers(auth_data->ssl_ciphers)
{
}

void Filter_ciphers::run()
{
	process_ciphersuites();
	process_ciphers();
}

// -------------

void Filter_ciphers::process_ciphersuites()
{
	if (!tls_ciphersuites.empty()) {
		filter_ciphersuites();
		return;
	}

	if (need_default_ciphersuites()) {
		tls_ciphersuites = allowed_ciphersuites;
	}
}

void Filter_ciphers::filter_ciphersuites()
{
	auto it{ std::remove_if(
		tls_ciphersuites.begin(),
		tls_ciphersuites.end(),
		[this](const std::string& ciphersuite) { return is_ciphersuite_forbidden(ciphersuite); })
	};
	tls_ciphersuites.erase(it, tls_ciphersuites.end());
	if (tls_ciphersuites.empty()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::no_valid_ciphersuite_in_list);
	}
}

bool Filter_ciphers::is_ciphersuite_forbidden(const std::string& ciphersuite) const
{
	static const util::std_stringset allowed_ciphersuite_set(
		allowed_ciphersuites.begin(),
		allowed_ciphersuites.end());
	return allowed_ciphersuite_set.find(ciphersuite) == allowed_ciphersuite_set.end();
}

/*
	OpenSSL for TLS earlier than v1.3 doesn't support ciphersuites, so default
	ciphpersuites are needed if below conditions are fulfilled:
	- user doesn't provide list of ciphpersuites explicitly, and
	- TLSv1.3 or newer is supported
	- user doesn't provide list of TLS versions explicitly, or...
	- ...among them is TLSv1.3 (or newer)
*/
bool Filter_ciphers::need_default_ciphersuites() const
{
	assert(tls_ciphersuites.empty());

	if (tls_versions.empty()) {
		return is_tlsv13_supported();
	}

	if (std::find(
			tls_versions.begin(),
			tls_versions.end(),
			Tls_version::tls_v1_3) != tls_versions.end()) {
		return true;
	}

	return is_tlsv13_supported()
		&& (std::find(
			tls_versions.begin(),
			tls_versions.end(),
			Tls_version::unspecified) != tls_versions.end());
}

// -------------

void Filter_ciphers::process_ciphers()
{
	if (!ssl_ciphers.empty()) {
		filter_ciphers();
		return;
	}

	if (need_default_ciphers()) {
		ssl_ciphers = allowed_ciphers;
	}
}

void Filter_ciphers::filter_ciphers()
{
	auto it{ std::remove_if(
		ssl_ciphers.begin(),
		ssl_ciphers.end(),
		[this](const std::string& cipher) { return is_cipher_forbidden(cipher); })
	};
	ssl_ciphers.erase(it, ssl_ciphers.end());
	if (ssl_ciphers.empty()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::no_valid_cipher_in_list);
	}
}

bool Filter_ciphers::is_cipher_forbidden(const std::string& cipher) const
{
	static const util::std_stringset allowed_cipher_set(
		allowed_ciphers.begin(),
		allowed_ciphers.end());
	return allowed_cipher_set.find(cipher) == allowed_cipher_set.end();
}

/*
	default ciphpers are needed if below conditions are fulfilled:
	- user doesn't provide list of ciphpers explicitly, and...
	- user doesn't provide tls versions explicitly, or...
	- ...among them is at least one TLS version earlier than TLSv1.3 (so operates on ciphers)
*/
bool Filter_ciphers::need_default_ciphers() const
{
	assert(ssl_ciphers.empty());

	if (tls_versions.empty()) return true;

	return std::any_of(
		tls_versions.begin(),
		tls_versions.end(),
		[](const Tls_version& tls_ver){
			return (tls_ver == Tls_version::unspecified)
				|| (tls_ver == Tls_version::tls_v1_0)
				|| (tls_ver == Tls_version::tls_v1_1)
				|| (tls_ver == Tls_version::tls_v1_2);
		}
	);
}

// -------------------------------

void filter_ciphers(Session_auth_data* auth_data)
{
	Filter_ciphers filter_ciphers(auth_data);
	filter_ciphers.run();
}

void prepare_ciphers(Session_auth_data* auth_data)
{
	if (auth_data->ssl_mode == SSL_mode::disabled) return;

	map_ciphersuites_to_ciphers(auth_data);
	filter_ciphers(auth_data);
}

} // anonymous namespace

// ------------------------------------------------------------------------------

Session_auth_data* extract_auth_information(const util::Url& node_url)
{
	DBG_ENTER("extract_auth_information");
	enum_func_status ret{PASS};
	std::unique_ptr<Session_auth_data> auth(new Session_auth_data);

	if( nullptr == auth ) {
		util::ostringstream os;
		os << "Couldn't allocate " << sizeof(Session_auth_data) << " bytes";
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

			/*
			 * Connection attributes
			 * are handled separately, in this function we're
			 * focusing on authentication stuff.
			 */
			if( boost::iequals(variable, "connection-attributes" )) {
					continue;
			}

			const util::string value{
				separator_pos == util::string::npos
						? util::string() : auth_option.substr(separator_pos + 1)
			};


			DBG_INF_FMT("URI query option: %s=%s",
						variable.c_str(),
						value.c_str()
						);

			if( FAIL == extract_client_option(variable, value, auth.get()) ) {
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
	if ((auth->ssl_mode == SSL_mode::not_specified) || (auth->ssl_mode == SSL_mode::any_secure)) {
		DBG_INF_FMT("Setting default SSL mode to REQUIRED");
		ret = Extract_client_option::assign_ssl_mode( *auth.get(), SSL_mode::required );
	}


	if( node_url.port != 0 ) {
		//If is 0, then we're using win pipe
		//or unix sockets.
		auth->port = node_url.port;
		auth->hostname = util::to_std_string(node_url.host);
	}
	auth->username = util::to_std_string(node_url.user);
	auth->password = util::to_std_string(node_url.pass);

	prepare_ciphers(auth.get());

	DBG_RETURN(auth.release());
}


int contains_list_of_url(
		const util::string& uri
)
{
	/*
	 * The function returns:
	 *  end = the URI contains a valid list
	 *  0   = the URI does not contain a valid list
	 * -1   = the URI is ill formatted.
	 */
	auto beg = uri.find_first_of('@');
	if( beg != util::string::npos ) {
		++beg;
	} else {
		return -1;
	}
	bool valid_list{ false };
	/*
	 * Find the first opening [ (start of the list) and
	 * the relative closing ] (end of the list). Verify
	 * if that can possibly be a list.
	 *
	 * (This code is not safe from ill-formatted URI's.. nobody is)
	 */
	int brk_cnt{ uri[ beg ] == '[' };
	std::size_t end{ beg + 1 };
	for( ; brk_cnt > 0 && end < uri.size() ; ++end ) {
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
		return -1;
	}
    return valid_list ? static_cast<int>( end ): 0;
}

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
	int valid_list = contains_list_of_url( uri );
	if( valid_list < 0 ) {
		invalidate();
	} else if ( valid_list > 0 ) {
		end = valid_list;
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

namespace {
util::string
get_os_name()
{
#ifdef __linux__
	return "Linux";
#endif
#ifdef __APPLE__
	return "Mac OS X";
#endif
#ifdef _WIN64
	return "Windows 64 bit";
#endif
#ifdef _WIN32
	return "Windows 32 bit";
#endif

#ifdef _AIX
	return "Solaris";
#endif
	return "Unknown";
}

static util::string
get_platform()
{
#ifdef __i386__
	return "i386";
#endif
#ifdef __x86_64__
	return "x86_64";
#endif
#ifdef __arm__
	return "arm";
#endif
#ifdef __powerpc64__
	return "powerpc64";
#endif
#ifdef __aarch64__
	return "aarch64";
#endif
	return "Unknown";
}

}

enum_func_status get_def_client_attribs( vec_of_attribs& attribs )
{
	util::ostringstream ss;
	attribs.push_back( {"_client_name", PHP_MYSQL_XDEVAPI_NAME} );
	pid_t proc_pid = getpid();
	ss << proc_pid;
	attribs.push_back( {"_pid", ss.str() } );
	attribs.push_back( {"_os" , get_os_name() } );
	attribs.push_back( {"_client_version" , PHP_MYSQL_XDEVAPI_VERSION } );
	attribs.push_back( {"_client_license" , PHP_MYSQL_XDEVAPI_LICENSE } );
	attribs.push_back( {"_platform" , get_platform() } );

	static const int hostname_len{ 128 };
	char hostname[hostname_len];
	if(!gethostname(hostname,hostname_len) ){
		attribs.push_back( {"_source_host" , hostname } );
	}

	return PASS;
}

std::pair<util::string,util::string>
parse_attribute( const util::string& attribute )
{
	static const size_t max_attrib_key_size{ 32 };
	static const size_t max_attrib_val_size{ 1024 };
	util::vector<util::string> key_value;
	boost::split( key_value,
				  attribute,
				  boost::is_any_of("="));
	if( key_value.empty() ) {
		return { "", "" };
	}
	if( key_value[0].size() > max_attrib_key_size ) {
		devapi::RAISE_EXCEPTION( err_msg_invalid_attrib_key_size );
		return { "" , "" };
	}
	if( key_value.size() > 1 && key_value[1].size() > max_attrib_val_size ) {
		devapi::RAISE_EXCEPTION( err_msg_invalid_attrib_value_size );
		return { "" , "" };
	}
	if( key_value[0].size() > 0 &&
			key_value[0][0] == '_' ) {
		devapi::RAISE_EXCEPTION( err_msg_invalid_attrib_key );
		return { "" , "" };
	}
	if( key_value.size() == 2 ) {
		return { key_value[0], key_value[1] };
	}
	return { key_value[0], "" };
}

enum_func_status parse_conn_attrib(
			vec_of_attribs& attrib_container,
			util::string    user_attribs,
			bool            is_a_list = false
)
{
	static const std::string attrib_disable{ "false" };
	static const std::string attrib_enabled{ "true" };
	enum_func_status ret = PASS;
	if( boost::iequals( user_attribs, attrib_disable ) ) {
		return PASS;
	}
	if( false ==  boost::iequals( user_attribs, attrib_enabled ) &&
			false == user_attribs.empty() ) {
		if( is_a_list ) {
			if( user_attribs.size() ) {
				util::vector<util::string> attributes;
				boost::split( attributes,
							  user_attribs,
							  boost::is_any_of(","));
				for( const auto& cur_attrib : attributes ) {
					if( !cur_attrib.empty() ) {
						auto result = parse_attribute( cur_attrib );
						auto it = std::find_if( attrib_container.begin(), attrib_container.end(),
							[&result](const std::pair<util::string, util::string>& element){
									return element.first == result.first;
						} );
						if( it != attrib_container.end()){
							throw util::xdevapi_exception(util::xdevapi_exception::Code::conn_attrib_dup_key);
						}
						if( !result.first.empty() ) {
							attrib_container.push_back( result );
						} else {
							ret = FAIL;
							break;
						}
					}
				}
			}
		} else {
			throw util::xdevapi_exception(util::xdevapi_exception::Code::conn_attrib_wrong_type);
		}
	}
	if( ret == FAIL ) {
		attrib_container.clear();
		return ret;
	}
	return get_def_client_attribs( attrib_container );
}


enum_func_status extract_connection_attributes(
			drv::XMYSQLND_SESSION session,
			const util::string& uri )
{
	static const std::string conn_attrib{ "connection-attributes" };
	static const size_t      max_attrib_len{ 1024 * 64 };
	enum_func_status         ret{ PASS };
	if( session == nullptr || uri.empty() ) {
		return FAIL;
	}
	std::size_t pos = uri.find(conn_attrib.c_str());
	std::size_t end_pos{ 0 };
	if( pos != std::string::npos ) {
		pos += conn_attrib.size();
		/*
		 * We have different possible scenarios:
		 *	1) a list of attributes is provided: connection-attributes=[a:b,c:d,...,] (The list might be empty!)
		 *	2) the attributes are disabled or anabled: connection-attributes=false/true
		 *  3) empty attributes (use only the default one): connection-attributes
		 *  4) attribute key with no value: connection-attributes=foo
		 */
		bool is_attrib_list = false;
		if( uri[ pos ] == '=' ) {
			//Handle 1,2,4
			++pos;
			if( uri[ pos ] == '[' ) {
				//List of attributes
				end_pos = uri.find_first_of( ']', pos );
				if( end_pos == std::string::npos ) {
					ret = FAIL;
				} else {
					is_attrib_list = true;
				}
				++pos;
			} else {
				end_pos = uri.find_first_of( ',' , pos );
			}
		} else {
			//Handle 3
			end_pos = pos;
		}
		if( ret != FAIL ) {
			ret = parse_conn_attrib( session->get_data()->connection_attribs,
						uri.substr( pos, end_pos - pos ),
						is_attrib_list );
			std::size_t attribs_len{ 0 };
			for( const auto& item : session->get_data()->connection_attribs ) {
				attribs_len += ( item.first.size() + item.second.size() );
			}
			if( attribs_len > max_attrib_len ) {
				devapi::RAISE_EXCEPTION( err_msg_invalid_attrib_size );
				ret = FAIL;
			}
		}
	}
	else
	{
		get_def_client_attribs(session->get_data()->connection_attribs);
	}
	return ret;
}

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

void verify_connection_string(const util::string& connection_string)
{
	const auto& uri_addresses{ extract_uri_addresses(connection_string) };
	if (uri_addresses.empty()) {
		util::ostringstream os;
		os << "invalid connection string '" << connection_string << "'.";
		throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_argument, os.str());
	}

	for (const auto& uri_address : uri_addresses) {
		verify_uri_address(uri_address.first);
	}
}

util::string prepare_connect_error_msg(
	const char* last_error_msg,
	const util::string& aux_msg)
{
	util::ostringstream errmsg;

	if (last_error_msg) {
		errmsg << last_error_msg;
	}

	if (!aux_msg.empty()) {
		if (last_error_msg) errmsg << ", ";
		errmsg << aux_msg;
	}

	return errmsg.str();
}

namespace{
constexpr const char* dns_srv_prefix{ "mysqlx+" };
constexpr const char* uri_addr_slash_pref{ "://" };
constexpr const char* srv_pref{ "srv" };
}

bool verify_dns_srv_uri(
	const char* uri_string
)
{
	DBG_ENTER("verify_dns_srv_uri");
	const auto off{ strlen( dns_srv_prefix ) +
					strlen( srv_pref ) +
					strlen( uri_addr_slash_pref ) };
	if( strlen( uri_string ) <= off ) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::provided_invalid_uri);
		return false;
	}
	/*
	 * Verify that those three rules are not broken:
	 *  #1 In SRV mode specifying a port number in the connection
	 *     string URL should result in error
	 *  #2 Attempting to specify a Unix socket connection with SRV
	 *	   look up will result in error
	 *  #3 Specifying multiple hostnames while also requesting a DNS
	 *	   SRV lookup will result in an error and error
	 */
	util::string uri( uri_string + off );
	auto pos = uri.find_first_of("@");
	if( pos != util::string::npos ) {
		uri = uri.substr( pos + 1 );
	}
	//Verify #1
	pos = uri.find_first_of(':');
	if( pos != util::string::npos ) {
		DBG_ERR_FMT("Port number not allowed while using DNS SRV!");
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::port_nbr_not_allowed_with_srv_uri);
		return false;
	}
	//Verify #2
	if( ( uri[0] == '(' && uri[1] == '/' ) ||
			uri[0] == '.' || uri[0] == '/' ) {
		DBG_ERR_FMT("The URI for DNS SRV does not support unix sockets!");
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::unix_socket_not_allowed_with_srv);
		return false;
	}
	//Verify #3
	int valid_list = contains_list_of_url( uri_string );
	if( valid_list != 0 ) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::url_list_not_allowed);
		return false;
	}
	return true;
}

namespace{

using Srv_data = std::map<uint16_t,
	std::map<uint16_t,std::forward_list<std::pair<util::string,uint16_t>>>
>;

using Srv_hostname_list = std::forward_list<std::pair<util::string,uint16_t>>;

Srv_hostname_list srv_data_to_hostname_list(const Srv_data& srv_data)
{
	Srv_hostname_list result;
	/*
	* Make sure the entries are in the proper priority/weight
	* order
	*/
	Srv_hostname_list::const_iterator srv_it{ result.before_begin() };
	for( auto elem : srv_data ){
		for( auto entry : mysqlx::drv::Reverse(elem.second) ){
			for( auto srv : entry.second ) {
				srv_it = result.emplace_after(srv_it,
								srv.first, srv.second);
			}
		}
	}
	return result;
}

}

#ifndef PHP_WIN32
Srv_hostname_list query_srv_list(
	const char* host_name
)
{
	struct __res_state state;
	res_ninit(&state);

	unsigned char query_buffer[PACKETSZ];
	char          srv_hostname[MAXDNAME];
	int res = res_nsearch(&state,
						  host_name,
						  C_ANY, ns_t_srv,
						  query_buffer,
						  sizeof (query_buffer) );

	if (res < 0) {
		return Srv_hostname_list();
	}

	Srv_data srv_data;
	ns_msg   msg;
	ns_rr    rr;
	ns_initparse(query_buffer, res, &msg);

	for ( uint16_t i{0}; i < ns_msg_count (msg, ns_s_an); ++i) {
		if( 0 != ns_parserr (&msg, ns_s_an, i, &rr) ) {
			return Srv_hostname_list();
		}
		const uint16_t priority{ ntohs(*(unsigned short*)ns_rr_rdata(rr)) };
		const uint16_t weight{ ntohs(*((unsigned short*)ns_rr_rdata(rr) + 1)) };
		const uint16_t port{ ntohs(*((unsigned short*)ns_rr_rdata(rr) + 2)) };
		dn_expand(ns_msg_base(msg),
					ns_msg_end(msg),
					ns_rr_rdata(rr) + 6,
					srv_hostname,
					sizeof(srv_hostname));
		srv_data[priority][weight].emplace_front(
			std::make_pair(srv_hostname,port));
	}

	return srv_data_to_hostname_list(srv_data);
}
#else
Srv_hostname_list query_srv_list(
	const char* host_name
)
{
	PDNS_RECORD dns_records{ nullptr };
	DNS_STATUS status{ DnsQuery(
		host_name,
		DNS_TYPE_SRV,
		DNS_QUERY_STANDARD,
		nullptr,
		&dns_records,
		nullptr)
	};

	if (status != 0) {
		return {};
	}

	Srv_data srv_data;
	PDNS_RECORD dns_record{ dns_records };
	while (dns_record) {
		if (dns_record->wType == DNS_TYPE_SRV) {
			const auto& dns_data{ dns_record->Data.Srv };
			srv_data[dns_data.wPriority][dns_data.wWeight].emplace_front(
				dns_data.pNameTarget, dns_data.wPort);
		}
		dns_record = dns_record->pNext;
	}

	DnsRecordListFree(dns_records, DnsFreeRecordListDeep);

	return srv_data_to_hostname_list(srv_data);
}
#endif

static
bool requested_srv_lookup(
	const char* uri_string,
	XMYSQLND_SESSION& session
)
{
	const char*			  res = strstr(uri_string, dns_srv_prefix);
	if( res ) {
		if( strncmp( res + strlen(dns_srv_prefix),
					 srv_pref, strlen(srv_pref) ) ) {
			/*
			 * Only 'srv' allowed after mysqlx+.
			 */
			throw util::xdevapi_exception(
				util::xdevapi_exception::Code::provided_invalid_uri);
		}
		return verify_dns_srv_uri( uri_string );
	}
	return false;
}


static
vec_of_addresses convert_srv_hostname_to_uri(
	const Srv_hostname_list& srv_hostnames,
	const util::Url&         node_url
)
{
	vec_of_addresses uri;
	/*
	 * Convert the raw URL to valid URI
	 */
	for( const auto elem : srv_hostnames ){
		util::stringstream new_uri;
		new_uri << namespace_mysqlx << "://" <<
				   node_url.user << ":" <<
				   node_url.pass << "@" <<
				   elem.first;
		if( !node_url.query.empty() ) {
			new_uri << "/?" <<node_url.query;
		}
		uri.push_back( std::make_pair( new_uri.str(), elem.second ));
	}
	return uri;
}


static
vec_of_addresses dns_srv_get_hostname_list(
	const char* uri_string
)
{
	DBG_ENTER("dns_srv_get_hostname_list");
	/*
	 * Process the URI, get the hostname string
	 * and attempt to resolve it by getting the
	 * DNS SRV records
	 */
	php_url * raw_node_url = php_url_parse(uri_string);
	if( nullptr == raw_node_url ) {
		DBG_ERR_FMT("URI parsing failed!");
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::provided_invalid_uri);
		return {};
	}
	util::Url node_url(raw_node_url);
	php_url_free(raw_node_url);
	raw_node_url = nullptr;
	auto raw_hostnames = query_srv_list(node_url.host.c_str());
    if( ! raw_hostnames.empty()){
		return convert_srv_hostname_to_uri( raw_hostnames,
											node_url );
	}
	return {};
}


PHP_MYSQL_XDEVAPI_API
enum_func_status connect_session(
	const char* uri_string,
	XMYSQLND_SESSION& session)
{
	DBG_ENTER("connect_session");
	DBG_INF_FMT("URI: %s",uri_string);
	enum_func_status ret{FAIL};
	if( nullptr == uri_string ) {
		DBG_ERR_FMT("The provided URI string is null!");
		DBG_RETURN(ret);
	}

	vec_of_addresses uris;
	if( requested_srv_lookup( uri_string, session ) ) {
		/*
		 * Requested lookup with DNS SRV.
		 */
		uris = dns_srv_get_hostname_list( uri_string );
		DBG_INF_FMT("Got a valid list of %d servers from the SRV records",
					uris.size());
	} else {
		/*
		 * Verify whether a list of addresses is provided,
		 * if that's the case we need to parse those addresses
		 * and their priority in order to implement the
		 * Client Side Failover
		 */
		uris = extract_uri_addresses( uri_string );
	}
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
			//Extract connection attributes
			if( FAIL == extract_connection_attributes( session,
													   current_uri.first ) ) {
				devapi::RAISE_EXCEPTION( err_msg_internal_error );
				DBG_RETURN(ret);
			}
			Session_auth_data * auth = extract_auth_information(url.first);
			if( nullptr != auth ) {
				if( auth->ssl_mode != SSL_mode::disabled) {
					/*
					 * If Unix sockets are used then TLS connections
					 * are not allowed either implicitly nor explicitly
					 */
					if (url.second == transport_types::unix_domain_socket) {
						DBG_ERR_FMT("Connection aborted, TLS not supported for Unix sockets!");
						devapi::RAISE_EXCEPTION( err_msg_tls_not_supported_1 );
						DBG_RETURN(FAIL);
					} else if (!util::zend::is_openssl_loaded()) {
						// cannot setup secure connection when openssl is not loaded
						throw util::xdevapi_exception(
							util::xdevapi_exception::Code::openssl_unavailable);
					}
				}

				DBG_INF_FMT("Attempting to connect...");
				ret = establish_connection(session,auth,
											url.first,url.second);
				if (ret == FAIL) {
					const MYSQLND_ERROR_INFO* session_error_info{
						session->get_data()->get_error_info() };
					if (session_error_info) {
						last_error_info = *session_error_info;
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
			session->get_data()->ps_data.set_supported_ps( true );
			break;
		}
	}
	/*
	 * Do not worry about the allocated auth if
	 * the connection fails, the dtor of 'session'
	 * will clean it up
	 */
	if( FAIL == ret ) {
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

PHP_MYSQL_XDEVAPI_API
enum_func_status xmysqlnd_new_session_connect(
	const char* uri_string,
	zval* return_value)
{
	DBG_ENTER("xmysqlnd_new_session_connect");

	enum_func_status ret{ FAIL };
	mysqlx::devapi::Session_data* session_data{ create_new_session(return_value) };
	if (nullptr == session_data) {
		devapi::RAISE_EXCEPTION( err_msg_internal_error );
		DBG_RETURN(ret);
	}

	// TODO: clean it up after fully switch to exceptions / smart zvals
	try {
		ret = connect_session(uri_string, session_data->session);
	} catch(std::exception&) {
		zval_dtor(return_value);
		ZVAL_NULL(return_value);
		throw;
	}

	if( FAIL == ret ) {
		zval_dtor(return_value);
		ZVAL_NULL(return_value);
	}

	DBG_RETURN(ret);
}

} // namespace drv

} // namespace mysqlx
