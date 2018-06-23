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
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_SESSION_H
#define XMYSQLND_SESSION_H

#include "mysqlnd_api.h"
extern "C" {
#include <ext/standard/url.h>
}
#include "xmysqlnd_driver.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_stmt.h"
#include "util/strings.h"
#include "util/types.h"
#include <array>
#include <memory>

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_stmt;
struct st_xmysqlnd_schema;
struct st_xmysqlnd_stmt_op__execute;

//PHP_MYSQL_XDEVAPI_API void xmysqlnd_session_module_init();

/* XMYSQLND_SESSION_DATA::client_capabilities */
#define XMYSQLND_CLIENT_NO_FLAG	0
#define XMYSQLND_FICTIVE_FLAG	1

/* Max possible value for an host priority (Client side failovers) */
constexpr int MAX_HOST_PRIORITY{ 100 };

enum xmysqlnd_session_state
{
	SESSION_ALLOCATED = 0,
	SESSION_NON_AUTHENTICATED = 1,
	SESSION_READY = 2,
	SESSION_CLOSE_SENT = 3
};


typedef enum xmysqlnd_session_close_type
{
	SESSION_CLOSE_EXPLICIT = 0,
	SESSION_CLOSE_IMPLICIT,
	SESSION_CLOSE_DISCONNECT,
	SESSION_CLOSE_LAST	/* for checking, should always be last */
} enum_xmysqlnd_session_close_type;


typedef enum xmysqlnd_server_option
{
	XMYSQLND_SOME_SERVER_OPTION = 0,
} enum_xmysqlnd_server_option;

struct st_xmysqlnd_query_builder
{
	enum_func_status (*create)(st_xmysqlnd_query_builder* builder);
	void (*destroy)(st_xmysqlnd_query_builder* builder);
	MYSQLND_STRING query;
};



typedef struct st_xmysqlnd_session_state XMYSQLND_SESSION_STATE;
typedef enum xmysqlnd_session_state (*func_xmysqlnd_session_state__get)(const XMYSQLND_SESSION_STATE * const state_struct);
typedef void (*func_xmysqlnd_session_state__set)(XMYSQLND_SESSION_STATE * const state_struct, const enum xmysqlnd_session_state state);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session_state)
{
	func_xmysqlnd_session_state__get get;
	func_xmysqlnd_session_state__set set;
};

struct st_xmysqlnd_session_state
{
	enum xmysqlnd_session_state state;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session_state) *m;
};

#define GET_SESSION_STATE(state_struct)		(state_struct)->m->get((state_struct))
#define SET_SESSION_STATE(state_struct, s)	(state_struct)->m->set((state_struct), (s))
PHP_MYSQL_XDEVAPI_API void xmysqlnd_session_state_init(XMYSQLND_SESSION_STATE * const state);



typedef struct st_xmysqlnd_session_options
{
	size_t	unused;
} XMYSQLND_SESSION_OPTIONS;


typedef struct st_xmysqlnd_level3_io
{
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
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
	required,
	disabled,
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

/*
 * Information used to authenticate
 * the connection with the server
 */
struct st_xmysqlnd_session_auth_data
{
	st_xmysqlnd_session_auth_data();

	util::string hostname;
	unsigned int port;
	util::string username;
	util::string password;
	//SSL information
	SSL_mode ssl_mode;
	bool ssl_enabled;
	bool ssl_no_defaults;
	util::string ssl_local_pk;
	util::string ssl_local_cert;
	util::string ssl_cafile;
	util::string ssl_capath;
	util::string ssl_passphrase;
	util::string ssl_ciphers;
	util::string ssl_crl;
	util::string ssl_crlpath;
	util::string tls_version;
	Auth_mechanism auth_mechanism = Auth_mechanism::unspecified;

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

typedef std::shared_ptr< st_xmysqlnd_session > XMYSQLND_SESSION;
typedef std::shared_ptr<st_xmysqlnd_session_data> XMYSQLND_SESSION_DATA;
typedef struct st_xmysqlnd_session_auth_data XMYSQLND_SESSION_AUTH_DATA;

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

using Auth_mechanisms = util::vector<Auth_mechanism>;


struct Authentication_context
{
	st_xmysqlnd_session_data* session;
	MYSQLND_CSTRING scheme;
	util::string username;
	util::string password;
	util::string database;
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
			const MYSQLND_CSTRING& salt,
			util::vector<char>& result);

protected:
	bool calc_hash(const MYSQLND_CSTRING& salt);
	virtual void scramble(const MYSQLND_CSTRING& salt) = 0;
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
	virtual util::string prepare_continue_auth_data(const MYSQLND_CSTRING& salt) = 0;
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
	util::string prepare_continue_auth_data(const MYSQLND_CSTRING& salt) override;

protected:
	void add_prefix_to_auth_data();
	void add_scramble_to_auth_data(const MYSQLND_CSTRING& salt);

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
			st_xmysqlnd_session_data* session,
			const MYSQLND_CSTRING& scheme,
			const MYSQLND_CSTRING& database);
	~Authenticate();

	bool run();

private:
	bool init_capabilities();
	bool init_connection();
	bool gather_auth_mechanisms();
	bool authentication_loop();
	bool authenticate_with_plugin(std::unique_ptr<Auth_plugin>& auth_plugin);
	void log_custom_error_msg();
	bool is_suppress_server_messages() const;

private:
	st_xmysqlnd_session_data* session;
	const MYSQLND_CSTRING& scheme;
	const MYSQLND_CSTRING& database;

	const st_xmysqlnd_message_factory msg_factory;
	st_xmysqlnd_msg__capabilities_get caps_get;
	const XMYSQLND_SESSION_AUTH_DATA* auth;

	zval capabilities;

	Auth_mechanisms auth_mechanisms;

};

util::string  auth_mechanism_to_str(Auth_mechanism auth_mechanism);
util::strings to_auth_mech_names(const Auth_mechanisms& auth_mechanisms);
util::string  prepare_auth_data(Auth_mechanism auth_mechanism,const XMYSQLND_SESSION_AUTH_DATA* auth,const util::string& database);
std::unique_ptr<Auth_scrambler> create_auth_scrambler(const Auth_mechanism auth_mechanism,const Authentication_context& auth_ctx);

const enum_hnd_func_status
on_suppress_auth_warning(
		void* context,
		const xmysqlnd_stmt_warning_level level,
		const unsigned int code,
		const MYSQLND_CSTRING message);
const enum_hnd_func_status
on_suppress_auth_error(
		void* context,
		const unsigned int code,
		const MYSQLND_CSTRING sql_state,
		const MYSQLND_CSTRING message);

class Gather_auth_mechanisms
{
public:
	Gather_auth_mechanisms(
			const XMYSQLND_SESSION_AUTH_DATA* auth,
			const zval* capabilities,
			Auth_mechanisms* auth_mechanisms);

	bool run();

private:
	bool is_tls_enabled() const;
	bool is_auth_mechanism_supported(Auth_mechanism auth_mechanism) const;
	void add_auth_mechanism(Auth_mechanism auth_mechanism);
	void add_auth_mechanism_if_supported(Auth_mechanism auth_mechanism);

private:
	const XMYSQLND_SESSION_AUTH_DATA* auth;
	const zval* capabilities;
	Auth_mechanisms& auth_mechanisms;

};

enum_func_status           setup_crypto_connection(st_xmysqlnd_session_data* session,st_xmysqlnd_msg__capabilities_get& caps_get,const st_xmysqlnd_message_factory& msg_factory);
char*                      build_server_host_info(const util::string& format,const util::string& name,zend_bool session_persistent);
const enum_hnd_func_status xmysqlnd_session_data_handler_on_error(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
const enum_hnd_func_status xmysqlnd_session_data_handler_on_auth_continue(void* context,const MYSQLND_CSTRING input,MYSQLND_STRING* const output);
enum_func_status           xmysqlnd_session_data_set_client_id(void * context, const size_t id);

class st_xmysqlnd_session_data : public util::custom_allocable
{
public:
	st_xmysqlnd_session_data() = delete;
	st_xmysqlnd_session_data(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
							 MYSQLND_STATS * mysqlnd_stats,
							 MYSQLND_ERROR_INFO * mysqlnd_error_info);
	~st_xmysqlnd_session_data();
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	MYSQLND_STRING    get_scheme(const util::string& hostname,unsigned int port);
	enum_func_status  connect_handshake(const MYSQLND_CSTRING scheme, const MYSQLND_CSTRING database, const size_t set_capabilities);
	enum_func_status  authenticate(const MYSQLND_CSTRING scheme,const MYSQLND_CSTRING database,const size_t set_capabilities);
	enum_func_status  connect(MYSQLND_CSTRING database,unsigned int port,size_t set_capabilities);
	size_t            escape_string(char * newstr,const char * to_escapestr,const size_t to_escapestr_len);
	MYSQLND_STRING    quote_name(const MYSQLND_CSTRING name);
	unsigned int      get_error_no();
	const char*       get_error_str();
	const char*       get_sqlstate();

	const char*       get_charset_name();
	enum_func_status  set_server_option(const enum_xmysqlnd_server_option option, const char * const value);
	enum_func_status  set_client_option(enum_xmysqlnd_client_option option, const char * const value);

	const char*       get_server_host_info();
	const char*       get_protocol_info();

	enum_func_status  send_close();
	enum_func_status  ssl_set(const char * const key,const char * const cert, const char * const ca, const char * const capath, const char * const cipher);
	size_t            negotiate_client_api_capabilities(const size_t flags);
	size_t            get_client_api_capabilities();

	size_t            get_client_id();
	void              cleanup();
public:
	/* Operation related */
	XMYSQLND_L3_IO	                   io;
	/* Authentication info */
	const XMYSQLND_SESSION_AUTH_DATA*  auth;
	/* Other connection info */
	MYSQLND_STRING	                   scheme;
	MYSQLND_STRING	                   current_db;
	transport_types                    transport_type;
	/* Used only in case of non network transports */
	util::string                       socket_path;
	char*			                   server_host_info;
	size_t			                   client_id;
	const MYSQLND_CHARSET*             charset;
	/* If error packet, we use these */
	MYSQLND_ERROR_INFO*                error_info;
	MYSQLND_ERROR_INFO	               error_info_impl;
	/* Operation related */
	XMYSQLND_SESSION_STATE             state;
	/* options */
	XMYSQLND_SESSION_OPTIONS*          options;
	XMYSQLND_SESSION_OPTIONS	       options_impl;
	size_t			                   client_api_capabilities;
	/* stats */
	MYSQLND_STATS*                     stats;
	zend_bool		                   own_stats;
	/* persistent connection */
	zend_bool		                   persistent;
	/* Seed for the next transaction savepoint identifier */
	unsigned int                       savepoint_name_seed;
private:
	void free_contents();
};


struct st_xmysqlnd_session_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt);
	void * ctx;
};

struct st_xmysqlnd_session_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt, const st_xmysqlnd_stmt_result_meta* const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_session_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_session_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_session_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_session_on_statement_ok_bind
{
	enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt* const stmt, const st_xmysqlnd_stmt_execution_state* const exec_state);
	void * ctx;
};


struct st_xmysqlnd_session_query_bind_variable_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_SESSION session, st_xmysqlnd_stmt_op__execute* const stmt);
	void * ctx;
};



typedef const enum_func_status	(*func_xmysqlnd_session__init)(XMYSQLND_SESSION* session,
															   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
															   MYSQLND_STATS * stats,
															   MYSQLND_ERROR_INFO * error_info);

typedef const enum_func_status	(*func_xmysqlnd_session__connect)(XMYSQLND_SESSION session,
																  MYSQLND_CSTRING database,
																  const unsigned int port,
																  const size_t set_capabilities);

typedef const MYSQLND_CSTRING	(*func_xmysqlnd_session__get_uuid)(XMYSQLND_SESSION session);
typedef const enum_func_status	(*func_xmysqlnd_session__precache_uuids)(XMYSQLND_SESSION session);
typedef const enum_func_status	(*func_xmysqlnd_session__create_db)(XMYSQLND_SESSION  session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_session__select_db)(XMYSQLND_SESSION  session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_session__drop_db)(XMYSQLND_SESSION  session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_session__query_cb)(XMYSQLND_SESSION  session,
																   const MYSQLND_CSTRING namespace_,
																   const MYSQLND_CSTRING query,
																   const struct st_xmysqlnd_session_query_bind_variable_bind var_binder,
																   const struct st_xmysqlnd_session_on_result_start_bind on_result_start,
																   const struct st_xmysqlnd_session_on_row_bind on_row,
																   const struct st_xmysqlnd_session_on_warning_bind on_warning,
																   const struct st_xmysqlnd_session_on_error_bind on_error,
																   const struct st_xmysqlnd_session_on_result_end_bind on_result_end,
																   const struct st_xmysqlnd_session_on_statement_ok_bind on_statement_ok);

typedef const enum_func_status	(*func_xmysqlnd_session__query_cb_ex)(XMYSQLND_SESSION session,
																	  const MYSQLND_CSTRING namespace_,
																	  st_xmysqlnd_query_builder* query_builder,
																	  const struct st_xmysqlnd_session_query_bind_variable_bind var_binder,
																	  const struct st_xmysqlnd_session_on_result_start_bind on_result_start,
																	  const struct st_xmysqlnd_session_on_row_bind on_row,
																	  const struct st_xmysqlnd_session_on_warning_bind on_warning,
																	  const struct st_xmysqlnd_session_on_error_bind on_error,
																	  const struct st_xmysqlnd_session_on_result_end_bind on_result_end,
																	  const struct st_xmysqlnd_session_on_statement_ok_bind on_statement_ok);



typedef const enum_func_status	(*func_xmysqlnd_session__query)(XMYSQLND_SESSION session,
																const MYSQLND_CSTRING namespace_,
																const MYSQLND_CSTRING query,
																const struct st_xmysqlnd_session_query_bind_variable_bind var_binder);

typedef zend_ulong			(*func_xmysqlnd_session__get_server_version)(XMYSQLND_SESSION session);
typedef const char *		(*func_xmysqlnd_session__get_server_information)(const XMYSQLND_SESSION session);

typedef st_xmysqlnd_stmt* (*func_xmysqlnd_session__create_statement_object)(XMYSQLND_SESSION session);

typedef st_xmysqlnd_schema* (*func_xmysqlnd_session__create_schema_object)(XMYSQLND_SESSION session, const MYSQLND_CSTRING schema_name);

typedef const enum_func_status				(*func_xmysqlnd_session__close)(XMYSQLND_SESSION session, const enum_xmysqlnd_session_close_type close_type);

typedef XMYSQLND_SESSION	(*func_xmysqlnd_session__get_reference)(XMYSQLND_SESSION session);
typedef const enum_func_status	(*func_xmysqlnd_session__free_reference)(XMYSQLND_SESSION session);
typedef void					(*func_xmysqlnd_session__free_contents)(XMYSQLND_SESSION session);/* private */
typedef void					(*func_xmysqlnd_session__dtor)(XMYSQLND_SESSION session);



MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session)
{
	func_xmysqlnd_session__connect connect;
	func_xmysqlnd_session__create_db create_db;
	func_xmysqlnd_session__select_db select_db;
	func_xmysqlnd_session__drop_db drop_db;
	func_xmysqlnd_session__query query;
	func_xmysqlnd_session__query_cb query_cb;
	func_xmysqlnd_session__query_cb_ex query_cb_ex;
	func_xmysqlnd_session__get_server_version get_server_version;
	func_xmysqlnd_session__get_server_information get_server_information;
	func_xmysqlnd_session__create_statement_object create_statement_object;
	func_xmysqlnd_session__create_schema_object create_schema_object;
	func_xmysqlnd_session__close close;
};

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
class Uuid_generator : public util::custom_allocable
{
public:
	using pointer = Uuid_generator*;
	Uuid_generator();
	Uuid_format::uuid_t generate();
private:
	/*
	 * There is one unique session ID for
	 * each UUID, this value is created once for
	 * st_xmysqlnd_session which should reflect
	 * the lifetime of a session
	 */
	void generate_session_node_info();
	void assign_node_id( Uuid_format& uuid );
	void assign_timestamp( Uuid_format &uuid );
	uint64_t last_timestamp;
	uint16_t clock_sequence;
	Uuid_format::node_id_t session_node_id;
};

struct st_xmysqlnd_session : public util::permanent_allocable
{
	st_xmysqlnd_session() = delete;
	st_xmysqlnd_session(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
						MYSQLND_STATS * stats,
						MYSQLND_ERROR_INFO * error_info);
	~st_xmysqlnd_session();

	XMYSQLND_SESSION_DATA data;
	char * server_version_string;
	Uuid_generator::pointer session_uuid;
	zend_bool persistent;
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session) * m;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_session_data);
PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_session);

PHP_MYSQL_XDEVAPI_API XMYSQLND_SESSION xmysqlnd_session_create(const size_t client_flags,
															   const zend_bool persistent,
															   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
															   MYSQLND_STATS * stats,
															   MYSQLND_ERROR_INFO * error_info);

PHP_MYSQL_XDEVAPI_API enum_func_status xmysqlnd_new_session_connect(const char* uri_string, zval * return_value);

extern const MYSQLND_CSTRING namespace_mysqlx;
extern const MYSQLND_CSTRING namespace_sql;
extern const MYSQLND_CSTRING namespace_xplugin;

extern const struct st_xmysqlnd_session_query_bind_variable_bind noop__var_binder;
extern const struct st_xmysqlnd_session_on_result_start_bind	noop__on_result_start;
extern const struct st_xmysqlnd_session_on_row_bind			noop__on_row;
extern const struct st_xmysqlnd_session_on_warning_bind		noop__on_warning;
extern const struct st_xmysqlnd_session_on_error_bind			noop__on_error;
extern const struct st_xmysqlnd_session_on_result_end_bind		noop__on_result_end;
extern const struct st_xmysqlnd_session_on_statement_ok_bind	noop__on_statement_ok;

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_SESSION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
