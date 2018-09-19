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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "util/allocator.h"
#include "util/exceptions.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"
#include <chrono>
#include <condition_variable> 
#include <mutex>
#include <thread>

namespace mysqlx {

namespace devapi {

namespace {

using namespace std::chrono_literals;

struct Connection_pool_options {
	const int Default_size{ 25 };
	const int Default_idle_time{ 0 };
	const int Default_queue_timeout{ 0 };

	bool enabled{ true };
	int max_size{ Default_size };
	int max_idle_time{ Default_idle_time };
	int queue_timeout{ Default_queue_timeout };
}; // Connection_pool_options

struct Client_options
{
	Connection_pool_options conn_pool_options;
}; // Client_options

//------------------------------------------------------------------------------

using ptree_string = util::json::ptree_string;
using ptree = boost::property_tree::basic_ptree<ptree_string, ptree_string>;

class Client_options_parser
{
public:
	Client_options_parser(const util::string_view& options_json);

public:
	Client_options run();

private:
	void verify_options_description();
	void verify_option(const std::string& option_name);

	void assign_options();
	template<
		typename T,
		typename Value_checker = std::function<bool(T val)>>
	void assign_option(
		const char* option_name,
		T& client_option,
		Value_checker value_checker = [](T) -> bool { return true; });

private:
	ptree options_desc;
	Client_options client_options;
}; // Client_options_parser

// ---------

Client_options_parser::Client_options_parser(const util::string_view& options_json)
{
	util::istringstream is(options_json.to_string());
	boost::property_tree::read_json(is, options_desc);
}

Client_options Client_options_parser::run()
{
	verify_options_description();
	assign_options();
	return client_options;
}

void Client_options_parser::verify_options_description()
{
	for (auto option : options_desc) {
		const std::string option_name{ option.first.data() };
		verify_option(option_name);
	}
}

void Client_options_parser::verify_option(const std::string& option_name)
{
	static const std::set<std::string> allowed_options{
		"enabled",
  		"maxSize",
  		"maxIdleTime",
  		"queueTimeOut"
	};

	if (!allowed_options.count(option_name)) {
		util::ostringstream os;
		os << "Client option '" << option_name << "' is not recognized as valid.";
		throw std::invalid_argument(os.str().c_str());
	}
}

void Client_options_parser::assign_options()
{
	Connection_pool_options& pool_options{client_options.conn_pool_options};
	assign_option("enabled", pool_options.enabled);
	assign_option("maxSize", pool_options.max_size, [](int val){ return 0 < val; });
	assign_option("maxIdleTime", pool_options.max_idle_time, [](int val){ return 0 <= val; });
	assign_option("queueTimeOut", pool_options.queue_timeout, [](int val){ return 0 <= val; });
}

template<typename T, typename Value_checker>
void Client_options_parser::assign_option(
	const char* option_name,
	T& client_option,
	Value_checker value_checker)
{
	auto option_desc{ options_desc.get_optional<T>(option_name) };
	if (!option_desc) return;

	auto option_value{ option_desc.get() };
	if (!value_checker(option_value)) {
		util::ostringstream os;
		os << "Client option '" << option_name << "' does not support value '" << option_value << "'.";
		throw std::invalid_argument(os.str().c_str());
	}

	client_option = option_value;
}

// ---------

Client_options parse_client_options(const util::string_view& options_json)
{
	if (options_json.empty()) return Client_options();

	Client_options_parser client_opts_parser(options_json);
	return client_opts_parser.run();
}

//------------------------------------------------------------------------------

using Time_point = std::chrono::system_clock::time_point;

struct Idle_connection
{
	Idle_connection(
		drv::XMYSQLND_SESSION connection,
		const std::chrono::milliseconds& max_idle_time)
		: connection(connection)
		, expiration_time(std::chrono::system_clock::now() + max_idle_time)
	{
	}
	drv::XMYSQLND_SESSION connection;
	Time_point expiration_time;
};


/* {{{ Connection_pool */
class Connection_pool
	: public util::permanent_allocable
	, private drv::Connection_pool_callback
{
public:
	Connection_pool(
		const std::string& uri,
		const Connection_pool_options& options);
	~Connection_pool();

public:
	drv::XMYSQLND_SESSION get_connection();
	void prune_expired_connections();
	void close();

private:
	//Connection_pool_callback
	virtual void on_close(drv::XMYSQLND_SESSION closing_connection);

private:
	bool is_full() const;
	bool has_idle_connection() const;
	drv::XMYSQLND_SESSION create_non_pooled_connection();
	drv::XMYSQLND_SESSION prepare_closed_connection();
	drv::XMYSQLND_SESSION add_new_connection();
	drv::XMYSQLND_SESSION pop_idle_connection();
	drv::XMYSQLND_SESSION try_pop_idle_connection(std::unique_lock<std::mutex>& lck);
	bool wait_for_idle_connection(std::unique_lock<std::mutex>& lck);

private:
	std::mutex mtx;
	std::condition_variable on_idle_connection_added;

	std::string connection_uri;
	const bool pooling_disabled;
	const std::size_t max_size;
	const std::chrono::milliseconds max_idle_time;
	const std::chrono::milliseconds queue_timeout;

	using Active_connections = std::set<drv::XMYSQLND_SESSION>;
	Active_connections active_connections;

	using Idle_connections = std::deque<Idle_connection>;
	Idle_connections idle_connections;

	Time_point next_prune_time;
};
/* }}} */

// ---------

Connection_pool::Connection_pool(
	const std::string& uri,
	const Connection_pool_options& options)
	: connection_uri(uri)
	, pooling_disabled(!options.enabled)
	, max_size(static_cast<std::size_t>(options.max_size))
	, max_idle_time(options.Default_idle_time)
	, queue_timeout(options.queue_timeout)
{
}

Connection_pool::~Connection_pool()
{
	close();
}

drv::XMYSQLND_SESSION Connection_pool::get_connection()
{
	if (pooling_disabled) return create_non_pooled_connection();

	std::unique_lock<std::mutex> lck(mtx);

	if (has_idle_connection()) return pop_idle_connection();

	if (is_full()) return try_pop_idle_connection(lck);

	return add_new_connection();
}

void Connection_pool::prune_expired_connections()
{
	if (max_idle_time == 0ms) return;

	std::lock_guard<std::mutex> lck(mtx);

	const Time_point now{ std::chrono::system_clock::now() };
	if (now <= next_prune_time) return;

	auto it{
		std::find_if(
			idle_connections.begin(),
			idle_connections.end(),
			[=](const Idle_connection& conn)
				{ return now < conn.expiration_time; })
	};

	idle_connections.erase(idle_connections.begin(), it);

	next_prune_time = now + max_idle_time;
}

void Connection_pool::close()
{
	std::lock_guard<std::mutex> lck(mtx);
	active_connections.clear();
	idle_connections.clear();
}

bool Connection_pool::is_full() const
{
	const std::size_t connections_count{ active_connections.size() + idle_connections.size() };
	assert(connections_count <= max_size);
	return connections_count == max_size;
}

bool Connection_pool::has_idle_connection() const
{
	return !idle_connections.empty();
}

drv::XMYSQLND_SESSION Connection_pool::create_non_pooled_connection()
{
	return drv::create_session(false);
}

drv::XMYSQLND_SESSION Connection_pool::prepare_closed_connection()
{
	drv::XMYSQLND_SESSION closed_connection{ drv::create_session(true) };
	closed_connection->get_data()->state.set(drv::SESSION_CLOSED);
	return closed_connection;
}

drv::XMYSQLND_SESSION Connection_pool::add_new_connection()
{
	drv::XMYSQLND_SESSION connection{ drv::create_session(true) };
	active_connections.insert(connection);
	connection->set_pooled(this);
	return connection;
}

drv::XMYSQLND_SESSION Connection_pool::pop_idle_connection()
{
	assert(has_idle_connection());
	drv::XMYSQLND_SESSION connection{ idle_connections.front().connection };
	idle_connections.pop_front();
	active_connections.insert(connection);
	connection->reset();
	return connection;
}

drv::XMYSQLND_SESSION Connection_pool::try_pop_idle_connection(std::unique_lock<std::mutex>& lck)
{
	if (wait_for_idle_connection(lck)) return pop_idle_connection();

	util::ostringstream os;
	os << "couldn't get connection from pool - queue timeout elapsed" << connection_uri.c_str();
	throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error, os.str());
}

bool Connection_pool::wait_for_idle_connection(std::unique_lock<std::mutex>& lck)
{
	if (queue_timeout != 0ms) {
		return on_idle_connection_added.wait_for(lck, queue_timeout, [this]{ return has_idle_connection(); });
	} else {
		on_idle_connection_added.wait(lck, [this]{ return has_idle_connection(); });
		return true;
	}
}

void Connection_pool::on_close(drv::XMYSQLND_SESSION closing_connection)
{
	std::lock_guard<std::mutex> lck(mtx);
	auto it{ active_connections.find(closing_connection) };
	if (it != active_connections.end()) {
		active_connections.erase(it);
		drv::XMYSQLND_SESSION idle_connection{ prepare_closed_connection() };
		std::swap(*idle_connection, *closing_connection);
		idle_connections.push_back({ idle_connection, max_idle_time });
		on_idle_connection_added.notify_one();
	}
}

//------------------------------------------------------------------------------

struct Client_state : public util::permanent_allocable
{
	Client_state(
		const std::string& uri,
		const Client_options& client_options);

	Connection_pool conn_pool;
};

Client_state::Client_state(
	const std::string& uri,
	const Client_options& client_options)
	: conn_pool(uri, client_options.conn_pool_options)
{
}

using shared_client_state = std::shared_ptr<Client_state>;

using Uri_to_client_state = std::map<std::string, shared_client_state>;

class Client_state_manager : public util::permanent_allocable
{
private:
	Client_state_manager() = default;
	Client_state_manager(const Client_state_manager&) = delete;
	Client_state_manager& operator=(const Client_state_manager&) = delete;

public:
	static Client_state_manager& get();

	shared_client_state get_client_state(
		const std::string& connection_uri,
		const util::string_view& client_options_desc);
	void prune_expired_connections();
	void release_all_clients();

private:
	shared_client_state add_client_state(
		const std::string& connection_uri,
		const Client_options& client_options);

private:
	std::mutex mtx;
	Uri_to_client_state client_states;
};

// ---------

Client_state_manager& Client_state_manager::get()
{
	static Client_state_manager instance;
	return instance;
}

shared_client_state Client_state_manager::get_client_state(
	const std::string& connection_uri,
	const util::string_view& client_options_desc)
{
	std::lock_guard<std::mutex> lck(mtx);
	auto it{ client_states.find(connection_uri) };
	if (it != client_states.end()) return it->second;

	const Client_options& client_options{ parse_client_options(client_options_desc) };
	return add_client_state(connection_uri, client_options);
}

shared_client_state Client_state_manager::add_client_state(
	const std::string& connection_uri,
	const Client_options& client_options)
{
	shared_client_state client_state{ std::make_shared<Client_state>(connection_uri, client_options) };
	auto result{ client_states.insert(std::make_pair(connection_uri, client_state)) };
	if (!result.second) {
		util::ostringstream os;
		os << "cannot add client for " << connection_uri.c_str();
		throw util::xdevapi_exception(util::xdevapi_exception::Code::runtime_error, os.str());
	}

	return client_state;
}

void Client_state_manager::prune_expired_connections()
{
	for (auto& uri_to_client_state : client_states) {
		auto& client_state{ uri_to_client_state.second };
		client_state->conn_pool.prune_expired_connections();
	}
}

void Client_state_manager::release_all_clients()
{
	client_states.clear();
}

//------------------------------------------------------------------------------

struct Client_data : public util::custom_allocable
{
	shared_client_state state;
};

Connection_pool& fetch_connection_pool(zval* from)
{
	auto& data_object{ util::fetch_data_object<Client_data>(from) };
	return data_object.state->conn_pool;
}

//------------------------------------------------------------------------------

zend_class_entry* client_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_client__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_client__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_client::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_client, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */

/* {{{ mysqlx_client::getSession() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_client, getSession)
{
	DBG_ENTER("mysqlx_client::getSession");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(
		execute_data, getThis(),
		"O", &object_zv, client_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& conn_pool{ fetch_connection_pool(object_zv) };
	drv::XMYSQLND_SESSION connection{ conn_pool.get_connection() };
	mysqlx_new_session(return_value, connection);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlx_client::close() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_client, close)
{
	DBG_ENTER("mysqlx_client::close");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(
		execute_data, getThis(),
		"O", &object_zv, client_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& conn_pool{ fetch_connection_pool(object_zv) };
	conn_pool.close();

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ client_methods[] */
const zend_function_entry client_methods[] = {
	PHP_ME(mysqlx_client, __construct, nullptr, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_client, getSession, arginfo_client__get_session, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_client, close, arginfo_client__close, ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */

zend_object_handlers client_handlers;
HashTable client_properties;

const st_mysqlx_property_entry client_property_entries[] =
{
	{{nullptr, 0}, nullptr, nullptr}
};

/* {{{ client_free_storage */
void
client_free_storage(zend_object* object)
{
	util::free_object<Client_data>(object);
}
/* }}} */


/* {{{ client_object_allocator */
zend_object*
client_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("client_object_allocator");
	st_mysqlx_object* mysqlx_object{ util::alloc_object<Client_data>(
		class_type,
		&client_handlers,
		&client_properties)
	};
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

/* {{{ mysqlx_new_client */
void
mysqlx_new_client(
	const util::string_view& connection_uri,
	const util::string_view& client_options_desc,
	zval* return_value)
{
	DBG_ENTER("mysqlx_new_client");

	Client_data& data_object{ util::init_object<Client_data>(client_class_entry, return_value) };
	Client_state_manager& csm{ Client_state_manager::get() };
	data_object.state = csm.get_client_state(connection_uri.to_std_string(), client_options_desc);

	DBG_VOID_RETURN;
}
/* }}} */

} // anonymous namespace

/* {{{ mysqlx_register_client_class */
void
mysqlx_register_client_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		client_class_entry,
		"Client",
		mysqlx_std_object_handlers,
		client_handlers,
		client_object_allocator,
		client_free_storage,
		client_methods,
		client_properties,
		client_property_entries);
}
/* }}} */

/* {{{ mysqlx_unregister_client_class */
void
mysqlx_unregister_client_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&client_properties);
}
/* }}} */

// ---------

/* {{{ mysqlx\\mysql_xdevapi_getClient */
MYSQL_XDEVAPI_PHP_FUNCTION(mysql_xdevapi_getClient)
{
	util::string_view connection_uri;
	util::string_view client_options_desc;

	RETVAL_NULL();

	DBG_ENTER("mysql_xdevapi_getClient");
	if (FAILURE == util::zend::parse_function_parameters(execute_data, "s|s",
		&connection_uri.str, &connection_uri.len,
		&client_options_desc.str, &client_options_desc.len))
	{
		DBG_VOID_RETURN;
	}

	#if PHP_DEBUG
	drv::verify_connection_string(connection_uri.to_string());
	#endif
	mysqlx_new_client(connection_uri.to_string(), client_options_desc, return_value);

	DBG_VOID_RETURN;
}
/* }}} */

namespace client {

/* {{{ prune_expired_connections */
void prune_expired_connections()
{
	Client_state_manager::get().prune_expired_connections();
}
/* }}} */

/* {{{ release_all_clients */
void release_all_clients()
{
	Client_state_manager::get().release_all_clients();
}
/* }}} */

} // namespace client

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
