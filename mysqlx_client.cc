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
#include "util/allocator.h"
#include "util/exceptions.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"
#include <thread>
#include <mutex>

namespace mysqlx {

namespace devapi {

//using namespace drv;

namespace {

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

/* {{{ Connection_pool */
class Connection_pool : public util::permanent_allocable
{
public:
	Connection_pool(
		const std::string& uri,
		const Client_options& client_options);
	~Connection_pool();

public:
	void get_session(zval* return_value);
	void close();

private:
	std::mutex mtx;
	std::string connection_uri;
	Connection_pool_options options;
	//Connections connections;

};
/* }}} */

// ---------

Connection_pool::Connection_pool(
	const std::string& uri,
	const Client_options& client_options)
	: connection_uri(uri)
	, options(client_options.conn_pool_options)
{
}

Connection_pool::~Connection_pool()
{
	close();
}

void Connection_pool::get_session(zval* return_value)
{
	std::unique_lock<std::mutex> lck(mtx);
	drv::xmysqlnd_new_session_connect(connection_uri.c_str(), return_value);
}

void Connection_pool::close()
{
	std::unique_lock<std::mutex> lck(mtx);
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
	: conn_pool(uri, client_options)
{
}

using shared_client_state = std::shared_ptr<Client_state>;

using uri_to_client_state = std::map<std::string, shared_client_state>;

class Client_state_manager : public util::permanent_allocable
{
private:
	Client_state_manager() = default;
	Client_state_manager(const Client_state_manager&) = delete;
	Client_state_manager& operator=(const Client_state_manager&) = delete;

public:
//	~Client_state_manager() = default;

public:
	static Client_state_manager& get();

	shared_client_state get_client_state(
		const std::string& connection_uri,
		const Client_options& client_options);
	void release();

private:
	shared_client_state add_client_state(
		const std::string& connection_uri,
		const Client_options& client_options);

private:
	std::mutex mtx;
	uri_to_client_state client_states;
};

// ---------

Client_state_manager& Client_state_manager::get()
{
	static Client_state_manager instance;
	return instance;
}

shared_client_state Client_state_manager::get_client_state(
	const std::string& connection_uri,
	const Client_options& client_options)
{
	std::unique_lock<std::mutex> lck(mtx);
	auto it{ client_states.find(connection_uri) };
	if (it != client_states.end()) return it->second;

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

void Client_state_manager::release()
{
	client_states.clear();
}

//------------------------------------------------------------------------------

struct Client_object_data : public util::custom_allocable
{
	shared_client_state state;
};

template<typename Data_object>
Connection_pool& fetch_connection_pool(zval* from)
{
	auto& data_object{ util::fetch_data_object<Client_object_data>(from) };
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

	auto& conn_pool{ fetch_connection_pool<Client_object_data>(object_zv) };
	conn_pool.get_session(return_value);

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

	auto& conn_pool{ fetch_connection_pool<Client_object_data>(object_zv) };
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
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ client_free_storage */
void
client_free_storage(zend_object* object)
{
	util::free_object<Client_object_data>(object);
}
/* }}} */


/* {{{ client_object_allocator */
zend_object*
client_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("client_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Client_object_data>(
		class_type,
		&client_handlers,
		&client_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

/* {{{ mysqlx_new_client */
void
mysqlx_new_client(
	const util::string_view& connection_uri,
	const Client_options& client_options,
	zval* return_value)
{
	DBG_ENTER("mysqlx_new_client");

	Client_object_data& data_object{ util::init_object<Client_object_data>(client_class_entry, return_value) };
	Client_state_manager& csm{ Client_state_manager::get() };
	data_object.state = csm.get_client_state(connection_uri.to_std_string(), client_options);

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

	drv::verify_connection_string(connection_uri.to_string());
	const Client_options& client_options{ parse_client_options(client_options_desc) };
	mysqlx_new_client(connection_uri.to_string(), client_options, return_value);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ cleanup_clients */
void cleanup_clients()
{
	Client_state_manager::get().release();
}
/* }}} */

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
