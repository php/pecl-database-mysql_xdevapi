/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "xmysqlnd_compression.h"
#include "xmysqlnd_wireprotocol.h"
#include "util/exceptions.h"
#include "util/types.h"
#include "util/value.h"

namespace mysqlx {

namespace drv {

namespace compression {

namespace {

const std::string PROPERTY_ALGORITHM{ "algorithm" };
const std::string PROPERTY_SERVER_STYLE{ "server_style" };
const std::string PROPERTY_CLIENT_STYLE{ "client_style" };

const std::string ALGORITHM_LZ4{ "lz4" };
const std::string ALGORITHM_ZLIB{ "zlib" };
const std::string ALGORITHM_DEFLATE{ "deflate" };
const std::string ALGORITHM_ZLIB_DEFLATE{ "deflate" };

const std::string SERVER_STYLE_NONE{ "none" };
const std::string SERVER_STYLE_SINGLE{ "single" };
const std::string SERVER_STYLE_MULTIPLE{ "multiple" };
const std::string SERVER_STYLE_GROUP{ "group" };

const std::string CLIENT_STYLE_NONE{ "none" };
const std::string CLIENT_STYLE_SINGLE{ "single" };
const std::string CLIENT_STYLE_MULTIPLE{ "multiple" };
const std::string CLIENT_STYLE_GROUP{ "group" };

// ----------------------------------------------------------------------------

class Gather_capabilities
{
public:
	Gather_capabilities(Capabilities& capabilities);

public:
	bool run(const util::zvalue& capabilities);

private:
	void add_supported_algorithm(const util::zvalue& zalgorithm);
	bool add_supported_algorithms(const util::zvalue& compression_caps);

	void add_supported_server_style(const util::zvalue& zserver_style);
	bool add_supported_server_styles(const util::zvalue& compression_caps);

	void add_supported_client_style(const util::zvalue& zclient_styles);
	bool add_supported_client_styles(const util::zvalue& compression_caps);

private:
	Capabilities& capabilities;
};

// ------------------------------------------

Gather_capabilities::Gather_capabilities(Capabilities& capabilities)
	: capabilities(capabilities)
{
}

bool Gather_capabilities::run(const util::zvalue& raw_capabilities)
{
	if (!raw_capabilities.is_array()) return false;

	const util::zvalue& compression_caps{ raw_capabilities["compression"] };
	if (!compression_caps.is_object()) return false;

	return add_supported_algorithms(compression_caps)
		&& add_supported_server_styles(compression_caps)
		&& add_supported_client_styles(compression_caps);
}

void Gather_capabilities::add_supported_algorithm(const util::zvalue& zalgorithm)
{
	if (!zalgorithm.is_string()) return;

	const std::string& algorithm_name{ zalgorithm.to_std_string() };
	using name_to_algorithm = std::map<std::string, Algorithm, util::iless>;
	static const name_to_algorithm algorithm_mapping{
		{ ALGORITHM_DEFLATE, Algorithm::zlib_deflate },
		{ ALGORITHM_ZLIB, Algorithm::zlib_deflate },
		{ ALGORITHM_LZ4, Algorithm::lz4 },
	};

	auto it{ algorithm_mapping.find(algorithm_name) };
	if (it == algorithm_mapping.end()) return;

	const Algorithm algorithm{ it->second };
	capabilities.algorithms.push_back(algorithm);
}

bool Gather_capabilities::add_supported_algorithms(const util::zvalue& compression_caps)
{
	const util::zvalue& algorithms{ compression_caps.get_property(PROPERTY_ALGORITHM) };
	if (!algorithms.is_array()) return false;

	for (const auto& algorithm : algorithms.values()) {
		add_supported_algorithm(algorithm);
	}

	return !capabilities.algorithms.empty();
}

// ---------------------

void Gather_capabilities::add_supported_server_style(const util::zvalue& zserver_style)
{
	if (!zserver_style.is_string()) return;

	const std::string& server_style_name{ zserver_style.to_std_string() };
	using name_to_server_style = std::map<std::string, Server_style, util::iless>;
	static const name_to_server_style server_style_mapping{
		{ SERVER_STYLE_SINGLE, Server_style::single },
		{ SERVER_STYLE_MULTIPLE, Server_style::multiple },
		{ SERVER_STYLE_GROUP, Server_style::group },
	};

	auto it{ server_style_mapping.find(server_style_name) };
	if (it == server_style_mapping.end()) return;

	const Server_style server_style{ it->second };
	capabilities.server_styles.push_back(server_style);
}

bool Gather_capabilities::add_supported_server_styles(const util::zvalue& compression_caps)
{
	const util::zvalue& server_styles{ compression_caps.get_property(PROPERTY_SERVER_STYLE) };
	if (!server_styles.is_array()) return false;

	for (const auto& server_style : server_styles.values()) {
		add_supported_server_style(server_style);
	}

	return !capabilities.server_styles.empty();
}

// ---------------------

void Gather_capabilities::add_supported_client_style(const util::zvalue& zclient_style)
{
	if (!zclient_style.is_string()) return;

	const std::string& client_style_name{ zclient_style.to_std_string() };
	using name_to_client_style = std::map<std::string, Client_style, util::iless>;
	static const name_to_client_style client_style_mapping{
		{ CLIENT_STYLE_SINGLE, Client_style::single },
		{ CLIENT_STYLE_MULTIPLE, Client_style::multiple },
		{ CLIENT_STYLE_GROUP, Client_style::group },
	};

	auto it{ client_style_mapping.find(client_style_name) };
	if (it == client_style_mapping.end()) return;

	const Client_style client_style{ it->second };
	capabilities.client_styles.push_back(client_style);
}

bool Gather_capabilities::add_supported_client_styles(const util::zvalue& compression_caps)
{
	const util::zvalue& client_styles{ compression_caps.get_property(PROPERTY_CLIENT_STYLE) };
	if (!client_styles.is_array()) return false;

	for (const auto& client_style : client_styles.values()) {
		add_supported_client_style(client_style);
	}

	return !capabilities.client_styles.empty();
}

// ----------------------------------------------------------------------------

class Negotiate
{
public:
	Negotiate(st_xmysqlnd_message_factory& msg_factory);

public:
	bool run(const Configuration& config);

private:
	std::string to_string(Algorithm algorithm) const;
	std::string to_string(Server_style server_style) const;
	std::string to_string(Client_style client_style) const;

private:
	static const enum_hnd_func_status handler_on_error(
		void* context,
		const unsigned int code,
		const MYSQLND_CSTRING sql_state,
		const MYSQLND_CSTRING message);

private:
	st_xmysqlnd_message_factory& msg_factory;
//	const Configuration& config;
};

// ------------------------------------------

Negotiate::Negotiate(st_xmysqlnd_message_factory& msg_factory)
	: msg_factory(msg_factory)
{
}

bool Negotiate::run(const Configuration& config)
{
	// "compression":{"client_style":"single", "server_style":"single", "algorithm": "defalte"})
	util::zvalue cap_compression_name("compression");
	util::zvalue cap_compression_value(util::zvalue::create_object());
	cap_compression_value.set_property(PROPERTY_ALGORITHM, to_string(config.algorithm));
	cap_compression_value.set_property(PROPERTY_SERVER_STYLE, to_string(config.server_style));
	cap_compression_value.set_property(PROPERTY_CLIENT_STYLE, to_string(config.client_style));

	st_xmysqlnd_msg__capabilities_set caps_set{ msg_factory.get__capabilities_set(&msg_factory) };
	zval* cap_names[]{cap_compression_name.ptr()};
	zval* cap_values[]{cap_compression_value.ptr()};
	if (caps_set.send_request(&caps_set, 1, cap_names, cap_values) != PASS) {
		return false;
	}

	const st_xmysqlnd_on_error_bind on_error{ handler_on_error, this };
	caps_set.init_read(&caps_set, on_error);
	util::zvalue result;
	return caps_set.read_response(&caps_set, result.ptr()) == PASS;
}

// ------------------------------------------

std::string Negotiate::to_string(Algorithm algorithm) const
{
	using algorithm_to_name = std::map<Algorithm, std::string>;
	static const algorithm_to_name algorithm_mapping{
		{ Algorithm::zlib_deflate, ALGORITHM_DEFLATE },
		{ Algorithm::lz4, ALGORITHM_LZ4 },
	};
	return algorithm_mapping.at(algorithm);
}

std::string Negotiate::to_string(Server_style server_style) const
{
	using server_style_to_name = std::map<Server_style, std::string>;
	static const server_style_to_name server_style_mapping{
		{ Server_style::single, SERVER_STYLE_SINGLE },
		{ Server_style::multiple, SERVER_STYLE_MULTIPLE },
		{ Server_style::group, SERVER_STYLE_GROUP },
	};
	return server_style_mapping.at(server_style);
}

std::string Negotiate::to_string(Client_style client_style) const
{
	using client_style_to_name = std::map<Client_style, std::string>;
	static const client_style_to_name client_style_mapping{
		{ Client_style::single, CLIENT_STYLE_SINGLE },
		{ Client_style::multiple, CLIENT_STYLE_MULTIPLE },
		{ Client_style::group, CLIENT_STYLE_GROUP },
	};
	return client_style_mapping.at(client_style);
}

const enum_hnd_func_status Negotiate::handler_on_error(
	void* context,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	throw util::xdevapi_exception(
		code,
		util::string(sql_state.s, sql_state.l),
		util::string(message.s, message.l));
}

// ----------------------------------------------------------------------------

class Setup
{
public:
	Setup(
		st_xmysqlnd_message_factory& msg_factory,
		Configuration& negotiated_config);

public:
	bool run(const util::zvalue& capabilities);

private:
	bool gather_capabilities(const util::zvalue& raw_capabilities);
	bool negotiate();
	bool is_algorithm_supported(Algorithm algorithm) const;
	bool is_server_style_supported(Server_style server_style) const;
	bool is_client_style_supported(Client_style client_style) const;
	bool negotiate(const Configuration& config);

private:
	st_xmysqlnd_message_factory& msg_factory;
	Capabilities capabilities;
	Configuration& negotiated_config;
};

// ------------------------------------------

Setup::Setup(
	st_xmysqlnd_message_factory& msg_factory,
	Configuration& negotiated_config)
	: msg_factory(msg_factory)
	, negotiated_config(negotiated_config)
{
}

bool Setup::run(const util::zvalue& raw_capabilities)
{
	if (!gather_capabilities(raw_capabilities)) return false;
	return negotiate();
}

// ------------------------------------------

bool Setup::gather_capabilities(const util::zvalue& raw_capabilities)
{
	Gather_capabilities gc(capabilities);
	return gc.run(raw_capabilities);
}

bool Setup::negotiate()
{
	// algorithms, server styles and client styles in the order how they should be negotiated
	const Algorithms algorithms{
		Algorithm::lz4,
		Algorithm::zlib_deflate
	};

	const Server_styles server_styles{
		Server_style::group,
		Server_style::multiple,
		Server_style::single
	};

	const Client_styles client_styles{
		Client_style::group,
		Client_style::multiple,
		Client_style::single
	};

	for (auto algorithm : algorithms) {
		if (!is_algorithm_supported(algorithm)) continue;

		for (auto server_style : server_styles) {
			if (!is_server_style_supported(server_style)) continue;

			for (auto client_style : client_styles) {
				if (!is_client_style_supported(client_style)) continue;

				Configuration config(algorithm, server_style, client_style);
				if (negotiate(config)) {
					negotiated_config = config;
					return true;
				}
			}
		}
	}

	return false;
}

// ---------------------

bool Setup::is_algorithm_supported(Algorithm algorithm) const
{
	const auto& algorithms{ capabilities.algorithms };
	return std::find(
		algorithms.begin(),
		algorithms.end(),
		algorithm) != algorithms.end();
}

bool Setup::is_server_style_supported(Server_style server_style) const
{
	const auto& server_styles{ capabilities.server_styles };
	return std::find(
		server_styles.begin(),
		server_styles.end(),
		server_style) != server_styles.end();
}

bool Setup::is_client_style_supported(Client_style client_style) const
{
	const auto& client_styles{ capabilities.client_styles };
	return std::find(
		client_styles.begin(),
		client_styles.end(),
		client_style) != client_styles.end();
}

// ---------------------

bool Setup::negotiate(const Configuration& config)
{
	Negotiate negotiate(msg_factory);
	return negotiate.run(config);
}

} // anonymous namespace

// ----------------------------------------------------------------------------

Configuration::Configuration(
	Algorithm algorithm,
	Server_style server_style,
	Client_style client_style)
	: algorithm(algorithm)
	, server_style(server_style)
	, client_style(client_style)
{
}

bool Configuration::enabled() const
{
	return (algorithm != Algorithm::none)
		&& ((server_style != Server_style::none) || (client_style != Client_style::none));
}

// ----------------------------------------------------------------------------

bool run_setup(
	st_xmysqlnd_message_factory& msg_factory,
	const util::zvalue& capabilities,
	Configuration& negotiated_config)
{
	Setup setup(msg_factory, negotiated_config);
	return setup.run(capabilities);
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
