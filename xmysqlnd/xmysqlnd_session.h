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
#ifndef XMYSQLND_SESSION_H
#define XMYSQLND_SESSION_H

#include "mysqlnd_api.h"
extern "C" {
#include <ext/standard/url.h>
}
#include "xmysqlnd_compression_types.h"
#include "xmysqlnd_compression.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_stmt.h"
#include "util/strings.h"
#include "util/types.h"
#include <array>
#include <memory>

namespace mysqlx {

namespace drv {

class xmysqlnd_stmt;
class xmysqlnd_schema;
struct st_xmysqlnd_stmt_op__execute;

/* Max possible value for an host priority (Client side failovers) */
constexpr int MAX_HOST_PRIORITY{ 100 };

enum xmysqlnd_session_state
{
	// Allocated but not connected or there was failure when trying
	// to connect with pre-allocated connection
	SESSION_ALLOCATED = 0,

	// while trying to setup connection to server
	SESSION_CONNECTING = 1,

	// connected, but not authenticated yet
	SESSION_NON_AUTHENTICATED = 2,

	// authenticated successfully, and ready to use
	SESSION_READY = 3,

	// request to close have been sent
	SESSION_CLOSE_SENT = 4,

	// connection is closed
	SESSION_CLOSED = 5,
};


typedef enum xmysqlnd_session_close_type
{
	SESSION_CLOSE_EXPLICIT = 0,
	SESSION_CLOSE_IMPLICIT,
	SESSION_CLOSE_DISCONNECT,
	SESSION_CLOSE_LAST	/* for checking, should always be last */
} enum_xmysqlnd_session_close_type;

struct st_xmysqlnd_query_builder
{
	enum_func_status (*create)(st_xmysqlnd_query_builder* builder);
	void (*destroy)(st_xmysqlnd_query_builder* builder);
	util::string query;
};

typedef struct st_xmysqlnd_session_state XMYSQLND_SESSION_STATE;
typedef enum xmysqlnd_session_state (*func_xmysqlnd_session_state__get)(const XMYSQLND_SESSION_STATE * const state_struct);
typedef void (*func_xmysqlnd_session_state__set)(XMYSQLND_SESSION_STATE * const state_struct, const enum xmysqlnd_session_state state);

struct st_xmysqlnd_session_state : public util::permanent_allocable
{
public:
	st_xmysqlnd_session_state();
	xmysqlnd_session_state get() const;
	void set(const xmysqlnd_session_state new_state);
private:
	xmysqlnd_session_state state;
};

typedef struct st_xmysqlnd_session_options
{
	size_t	unused;
} XMYSQLND_SESSION_OPTIONS;


typedef struct st_xmysqlnd_level3_io : public util::permanent_allocable
{
	st_xmysqlnd_level3_io() = default;
	st_xmysqlnd_level3_io(MYSQLND_VIO* vio, XMYSQLND_PFC* pfc)
		: vio(vio)
		, pfc(pfc)
	{
	}
	st_xmysqlnd_level3_io(st_xmysqlnd_level3_io&& rhs)
		: vio(rhs.vio)
		, pfc(rhs.pfc)
	{
		rhs.pfc = nullptr;
		rhs.vio = nullptr;
	}
	st_xmysqlnd_level3_io& operator=(st_xmysqlnd_level3_io&& rhs)
	{
		vio = rhs.vio;
		rhs.vio = nullptr;

		pfc = rhs.pfc;
		rhs.pfc = nullptr;

		return *this;
	}

	MYSQLND_VIO* vio{ nullptr };
	XMYSQLND_PFC* pfc{ nullptr };
} XMYSQLND_L3_IO;

/*
 * Needed during the connection phase to
 * select the correct socket
 */
enum class transport_types {
	windows_pipe,
	unix_domain_socket,
	network, //tcp or others
	none //To signal an error situation
};

/*
 * Supported SSL modes
 */
enum class SSL_mode
{
	not_specified, //Will be turned to 'required' during the connection
	disabled,
	any_secure,
	required,
	verify_ca,
	verify_identity
};

enum class Auth_mechanism
{
	unspecified,
	mysql41,
	plain,
	external,
	sha256_memory
};

enum class Tls_version
{
	unspecified,
	tls_v1_0,
	tls_v1_1,
	tls_v1_2,
	tls_v1_3
};

using Tls_versions = util::vector<Tls_version>;

/*
 * Information used to authenticate
 * the connection with the server
 */
struct Session_auth_data
{
	Session_auth_data();

	std::string hostname;
	unsigned int port;
	std::string username;
	std::string password;
	std::optional<int> connection_timeout;
	compression::Policy compression_policy{ compression::Policy::preferred };
	boost::optional<util::std_strings> compression_algorithms;

	//SSL information
	SSL_mode ssl_mode;
	bool ssl_no_defaults;
	Tls_versions tls_versions;
	util::std_strings tls_ciphersuites;
	std::string ssl_local_pk;
	std::string ssl_local_cert;
	std::string ssl_cafile;
	std::string ssl_capath;
	util::std_strings ssl_ciphers;
	bool ssl_allow_self_signed_cert{ true };
	Auth_mechanism auth_mechanism{ Auth_mechanism::unspecified };

	/*
	 * On demand we need to provide a list of supported ciphers,
	 * this list must be compatible with OSSA: TLS Ciphers and Versions,
	 * (Oracle, Approved Security Technologies..)
	 *
	 * Those are the ciphers that the server will provide to OpenSSL
	 * and yaSSL plus the deprecated and unacceptable ciphers from OSSA.
	 */
	static const std::vector<std::string> supported_ciphers;
};

typedef std::shared_ptr< xmysqlnd_session > XMYSQLND_SESSION;
typedef std::shared_ptr<xmysqlnd_session_data> XMYSQLND_SESSION_DATA;

using vec_of_addresses = util::vector< std::pair<util::string,long> >;
using vec_of_attribs = util::vector< std::pair<util::string,util::string > >;

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

void verify_connection_string(const util::string& connection_string);

using Auth_mechanisms = util::vector<Auth_mechanism>;


struct Authentication_context
{
	xmysqlnd_session_data* session;
	util::string_view scheme;
	util::string username;
	util::string password;
	util::string default_schema;
};


// classes to calculate client_hash for given authentication mechanism
class Auth_scrambler
{
protected:
	Auth_scrambler(
			const Authentication_context& context,
			const unsigned int hash_length);

public:
	virtual ~Auth_scrambler();

	void run(
			const util::string_view& salt,
			util::vector<char>& result);

protected:
	bool calc_hash(const util::string_view& salt);
	virtual void scramble(const util::string_view& salt) = 0;
	void hex_hash(util::vector<char>& hexed_hash);

protected:
	const Authentication_context& context;

	const unsigned int Hash_length;
	const unsigned int Scramble_length{ SCRAMBLE_LENGTH };

	util::vector<unsigned char> hash;

};

class Auth_plugin
{
public:
	virtual ~Auth_plugin() {}
	virtual const char* get_mech_name() const = 0;
	virtual util::string prepare_start_auth_data() = 0;
	virtual util::string prepare_continue_auth_data(const util::string_view& salt) = 0;
};

// -------------

class Auth_plugin_base : public Auth_plugin
{
protected:
	Auth_plugin_base(
			const char* mech_name,
			const Authentication_context& context);

public:
	const char* get_mech_name() const override;
	util::string prepare_start_auth_data() override;
	util::string prepare_continue_auth_data(const util::string_view& salt) override;

protected:
	void add_prefix_to_auth_data();
	void add_scramble_to_auth_data(const util::string_view& salt);

	void add_to_auth_data(const util::string& str);
	void add_to_auth_data(const util::vector<char>& data);
	void add_to_auth_data(char chr);

	virtual std::unique_ptr<Auth_scrambler> get_scrambler();

	util::string auth_data_to_string() const;

protected:
	const char* mech_name;
	const Authentication_context& context;
	util::vector<char> auth_data;
};


class Authenticate
{
public:
	Authenticate(
		xmysqlnd_session_data* session,
		const util::string_view& scheme,
		const util::string& default_schema);
	~Authenticate();

	bool run(bool re_auth = false);
	zval get_capabilities();
private:
	bool run_auth();
	bool run_re_auth();

	bool init_capabilities();
	void setup_compression();
	bool init_connection();
	bool gather_auth_mechanisms();
	bool authentication_loop();
	bool authenticate_with_plugin(std::unique_ptr<Auth_plugin>& auth_plugin);
	void raise_multiple_auth_mechanisms_algorithm_error();
	bool is_multiple_auth_mechanisms_algorithm() const;
private:
	xmysqlnd_session_data* session;
	const util::string_view scheme;
	const util::string& default_schema;

	st_xmysqlnd_message_factory msg_factory;
	st_xmysqlnd_msg__capabilities_get caps_get;
	const Session_auth_data* auth;

	zval capabilities;

	Auth_mechanisms auth_mechanisms;

};

util::string  auth_mechanism_to_str(Auth_mechanism auth_mechanism);
util::strings to_auth_mech_names(const Auth_mechanisms& auth_mechanisms);
std::unique_ptr<Auth_scrambler> create_auth_scrambler(const Auth_mechanism auth_mechanism,const Authentication_context& auth_ctx);

const enum_hnd_func_status
on_suppress_auth_warning(
		void* context,
		const xmysqlnd_stmt_warning_level level,
		const unsigned int code,
		const util::string_view& message);
const enum_hnd_func_status
on_suppress_auth_error(
		void* context,
		const unsigned int code,
		const util::string_view& sql_state,
		const util::string_view& message);

class Gather_auth_mechanisms
{
public:
	Gather_auth_mechanisms(
			const Session_auth_data* auth,
			const zval* capabilities,
			Auth_mechanisms* auth_mechanisms);

	bool run();

private:
	bool is_tls_enabled() const;
	bool is_auth_mechanism_supported(Auth_mechanism auth_mechanism) const;
	void add_auth_mechanism(Auth_mechanism auth_mechanism);
	void add_auth_mechanism_if_supported(Auth_mechanism auth_mechanism);

private:
	const Session_auth_data* auth;
	const zval* capabilities;
	Auth_mechanisms& auth_mechanisms;

};

bool set_connection_timeout(
	const std::optional<int>& connection_timeout,
	MYSQLND_VIO* vio);

const enum_hnd_func_status xmysqlnd_session_data_handler_on_error(void * context, const unsigned int code, const util::string_view& sql_state, const util::string_view& message);
const enum_hnd_func_status xmysqlnd_session_data_handler_on_auth_continue(void* context,const util::string_view& input,util::string* const output);
enum_func_status           xmysqlnd_session_data_set_client_id(void * context, const size_t id);

class xmysqlnd_session_data : public util::permanent_allocable
{
public:
	xmysqlnd_session_data(
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
		MYSQLND_STATS* mysqlnd_stats,
		MYSQLND_ERROR_INFO* mysqlnd_error_info);
	xmysqlnd_session_data(xmysqlnd_session_data&& rhs) noexcept;
	~xmysqlnd_session_data();

	xmysqlnd_session_data() = delete;
	xmysqlnd_session_data(const xmysqlnd_session_data&) = delete;
	xmysqlnd_session_data& operator=(const xmysqlnd_session_data&) = delete;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	st_xmysqlnd_message_factory create_message_factory();
	std::string get_scheme(const std::string& hostname, unsigned int port);
	enum_func_status connect_handshake(
		const util::string_view& scheme,
		const util::string& default_schema,
		const size_t set_capabilities);
	enum_func_status authenticate(
		const util::string_view& scheme,
		const util::string& default_schema,
		const size_t set_capabilities,
		const bool re_auth = false);
	enum_func_status  connect(
		const util::string& default_schema,
		unsigned int port,
		size_t set_capabilities);
	util::string quote_name(const util::string_view& name);
	unsigned int      get_error_no();
	const char*       get_error_str();
	const char*       get_sqlstate();
	const MYSQLND_ERROR_INFO* get_error_info() const;

	enum_func_status  set_client_option(enum_xmysqlnd_client_option option, const char * const value);
	enum_func_status  send_client_attributes();

	enum_func_status  send_reset(bool keep_open);
	enum_func_status  send_close();
	bool is_closed() const { return state.get() == SESSION_CLOSED; }
	size_t            negotiate_client_api_capabilities(const size_t flags);

	bool is_session_properly_supported();
	size_t            get_client_id();
	void              cleanup();
public:
	/* Operation related */
	XMYSQLND_L3_IO	                   io;
	/* Authentication info */
	std::unique_ptr<Session_auth_data> auth;
	Auth_mechanisms                    auth_mechanisms;
	/* Other connection stuff */
	std::string                        scheme;
	std::string                        default_schema;
	transport_types                    transport_type;
	compression::Executor compression_executor;
	/* Used only in case of non network transports */
	std::string                        socket_path;
	std::string                        server_host_info;
	size_t			                   client_id;
	const MYSQLND_CHARSET*             charset;
	/* If error packet, we use these */
	MYSQLND_ERROR_INFO*                error_info{nullptr};
	MYSQLND_ERROR_INFO	               error_info_impl{};
	/* Operation related */
	XMYSQLND_SESSION_STATE             state;
	size_t			                   client_api_capabilities;
	/*
		before 8.0.16 session wasn't properly supported:
		- it wasn't possible to reset session without reauthentication
		- Session::Close meant Connection::Close
		more details: WL#12375 WL#12396
	*/
	mutable std::optional<bool>      session_properly_supported;
	/* stats */
	MYSQLND_STATS*                     stats;
	zend_bool		                   own_stats;
	/* persistent connection */
	zend_bool persistent{ TRUE };
	/* Seed for the next transaction savepoint identifier */
	unsigned int                       savepoint_name_seed;
	vec_of_attribs                     connection_attribs;
	drv::Prepare_stmt_data             ps_data;
	util::zvalue                       capabilities;
private:
	void free_contents();
	Mysqlx::Datatypes::Object*  prepare_client_attr_object();
};


struct st_xmysqlnd_session_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt);
	void * ctx;
};

struct st_xmysqlnd_session_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt, const st_xmysqlnd_stmt_result_meta* const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_session_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_session_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt, const unsigned int code, const util::string_view& sql_state, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_session_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_session_on_statement_ok_bind
{
	enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, xmysqlnd_stmt* const stmt, const st_xmysqlnd_stmt_execution_state* const exec_state);
	void * ctx;
};


struct st_xmysqlnd_session_query_bind_variable_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt_op__execute* const stmt);
	void * ctx;
};

typedef XMYSQLND_SESSION	(*func_xmysqlnd_session__get_reference)(XMYSQLND_SESSION session);
typedef const enum_func_status	(*func_xmysqlnd_session__free_reference)(XMYSQLND_SESSION session);
typedef void					(*func_xmysqlnd_session__free_contents)(XMYSQLND_SESSION session);/* private */
typedef void					(*func_xmysqlnd_session__dtor)(XMYSQLND_SESSION session);


#define UUID_VERSION      0x1000
/*
 *
 * The formal definition of the UUID string representation is
 * provided by the following ABNF [7]:
 *
 *   UUID                   = time-low "-" time-mid "-"
 *                            time-high-and-version "-"
 *                            clock-seq-and-reserved
 *                            clock-seq-low "-" node
 *   time-low               = 4hexOctet
 *   time-mid               = 2hexOctet
 *   time-high-and-version  = 2hexOctet
 *   clock-seq-and-reserved = hexOctet
 *   clock-seq-low          = hexOctet
 *   node                   = 6hexOctet
 *
 *  In the struct the elements are reversed as required
 *  by the DevAPI specification
 */
#define UUID_NODE_ID_SIZE 6
#define UUID_SIZE 32
struct Uuid_format
{
	using uuid_t = std::array< char, UUID_SIZE >;
	using node_id_t = std::array< unsigned char, UUID_NODE_ID_SIZE >;
	node_id_t node_id;
	uint16_t clock_seq; //clock-seq-and-reserved and clock-seq-low
	uint16_t time_hi_and_version;
	uint16_t time_mid;
	uint32_t time_low;

	Uuid_format();
	uuid_t get_uuid();
};

/*
 * Implementation of the UUID
 * generation algorithm as specified by
 * http://www.ietf.org/rfc/rfc4122.txt
 */
class Uuid_generator : public util::permanent_allocable
{
public:
	using pointer = Uuid_generator*;
	Uuid_generator();
	Uuid_format::uuid_t generate();
private:
	/*
	 * There is one unique session ID for
	 * each UUID, this value is created once for
	 * xmysqlnd_session which should reflect
	 * the lifetime of a session
	 */
	void generate_session_node_info();
	void assign_node_id( Uuid_format& uuid );
	void assign_timestamp( Uuid_format &uuid );
	uint64_t last_timestamp;
	uint16_t clock_sequence;
	Uuid_format::node_id_t session_node_id;
};

struct Connection_pool_callback
{
	virtual void on_close(XMYSQLND_SESSION closing_connection) = 0;
};

class xmysqlnd_session : public util::permanent_allocable, public std::enable_shared_from_this<xmysqlnd_session>
{
public:
	xmysqlnd_session(
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
		MYSQLND_STATS* stats,
		MYSQLND_ERROR_INFO* error_info);
	xmysqlnd_session(xmysqlnd_session&& rhs) noexcept;
	~xmysqlnd_session();

	xmysqlnd_session() = delete;
	xmysqlnd_session(const xmysqlnd_session& rhs) = delete;
	xmysqlnd_session& operator=(const xmysqlnd_session& rhs) = delete;

	enum_func_status xmysqlnd_schema_operation(const util::string_view& operation, const util::string_view& db);

	const enum_func_status connect(
		const util::string& default_schema,
		const unsigned int port,
		const size_t set_capabilities);
	const enum_func_status reset();
	const enum_func_status create_db(const util::string_view& db);
	const enum_func_status select_db(const util::string_view& db);
	const enum_func_status drop_db(const util::string_view& db);

    const enum_func_status	query(const std::string_view& namespace_,
                                  const util::string_view& query,
                                  const st_xmysqlnd_session_query_bind_variable_bind var_binder);

    const enum_func_status	query_cb(
                                                                       const std::string_view& namespace_,
                                                                       const util::string_view& query,
                                                                       const st_xmysqlnd_session_query_bind_variable_bind var_binder,
                                                                       const st_xmysqlnd_session_on_result_start_bind on_result_start,
                                                                       const st_xmysqlnd_session_on_row_bind on_row,
                                                                       const st_xmysqlnd_session_on_warning_bind on_warning,
                                                                       const st_xmysqlnd_session_on_error_bind on_error,
                                                                       const st_xmysqlnd_session_on_result_end_bind on_result_end,
                                                                       const st_xmysqlnd_session_on_statement_ok_bind on_statement_ok);

    zend_ulong			get_server_version();

    xmysqlnd_stmt* create_statement_object(XMYSQLND_SESSION session_handle);

	xmysqlnd_schema* create_schema_object(const util::string_view& schema_name);

    const enum_func_status close(const enum_xmysqlnd_session_close_type close_type);
	bool is_closed() const { return data->is_closed(); }

	void set_pooled(Connection_pool_callback* conn_pool_callback) { pool_callback = conn_pool_callback; }
	bool is_pooled() const { return pool_callback != nullptr; }

	XMYSQLND_SESSION_DATA get_data();

	XMYSQLND_SESSION_DATA data;
	std::string server_version_string;
	std::unique_ptr<Uuid_generator> session_uuid;
	Connection_pool_callback* pool_callback{ nullptr };
	zend_bool persistent{ TRUE };
};

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION xmysqlnd_session_create(const size_t client_flags,
															   const zend_bool persistent,
															   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
															   MYSQLND_STATS * stats,
															   MYSQLND_ERROR_INFO * error_info);

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION create_session(const bool persistent);

PHP_MYSQL_XDEVAPI_API enum_func_status connect_session(
	const char* uri_string,
	XMYSQLND_SESSION& session);
PHP_MYSQL_XDEVAPI_API enum_func_status xmysqlnd_new_session_connect(
	const char* uri_string,
	zval* return_value);

constexpr util::string_view namespace_mysqlx{ "mysqlx" };
constexpr util::string_view namespace_sql{ "sql" };

extern const st_xmysqlnd_session_query_bind_variable_bind noop__var_binder;
extern const st_xmysqlnd_session_on_result_start_bind	noop__on_result_start;
extern const st_xmysqlnd_session_on_row_bind			noop__on_row;
extern const st_xmysqlnd_session_on_warning_bind		noop__on_warning;
extern const st_xmysqlnd_session_on_error_bind			noop__on_error;
extern const st_xmysqlnd_session_on_result_end_bind		noop__on_result_end;
extern const st_xmysqlnd_session_on_statement_ok_bind	noop__on_statement_ok;

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_SESSION_H */
