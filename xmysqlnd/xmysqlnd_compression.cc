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
#include "util/types.h"
#include "util/value.h"

namespace mysqlx {

namespace drv {

namespace compression {

namespace {

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
	util::zvalue compression_caps{ raw_capabilities["compression"] };
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
		{ "deflate", Algorithm::zlib_deflate },
		{ "zlib", Algorithm::zlib_deflate },
		{ "lz4", Algorithm::lz4 },
	};

	auto it{ algorithm_mapping.find(algorithm_name) };
	if (it == algorithm_mapping.end()) return;

	const Algorithm algorithm{ it->second };
	capabilities.algorithms.push_back(algorithm);
}

bool Gather_capabilities::add_supported_algorithms(const util::zvalue& compression_caps)
{
	util::zvalue algorithms{ compression_caps.get_property("algorithm") };
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
		{ "single", Server_style::single },
		{ "multiple", Server_style::multiple },
		{ "group", Server_style::group },
	};

	auto it{ server_style_mapping.find(server_style_name) };
	if (it == server_style_mapping.end()) return;

	const Server_style server_style{ it->second };
	capabilities.server_styles.push_back(server_style);
}

bool Gather_capabilities::add_supported_server_styles(const util::zvalue& compression_caps)
{
	util::zvalue server_styles{ compression_caps.get_property("server_style") };
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
		{ "single", Client_style::single },
		{ "multiple", Client_style::multiple },
		{ "group", Client_style::group },
	};

	auto it{ client_style_mapping.find(client_style_name) };
	if (it == client_style_mapping.end()) return;

	const Client_style client_style{ it->second };
	capabilities.client_styles.push_back(client_style);
}

bool Gather_capabilities::add_supported_client_styles(const util::zvalue& compression_caps)
{
	util::zvalue client_styles{ compression_caps.get_property("client_style") };
	if (!client_styles.is_array()) return false;

	for (const auto& client_style : client_styles.values()) {
		add_supported_client_style(client_style);
	}

	return !capabilities.client_styles.empty();
}

// ----------------------------------------------------------------------------

class Run_setup
{
public:
	Run_setup(st_xmysqlnd_message_factory& msg_factory);

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
	Configuration config;
};

// ----------------------------------------------------------------------------

Run_setup::Run_setup(st_xmysqlnd_message_factory& msg_factory)
	: msg_factory(msg_factory)
{
}

bool Run_setup::run(const util::zvalue& raw_capabilities)
{
	if (!gather_capabilities(raw_capabilities)) return false;
	return negotiate();
}

// ----------------------------------------------------------------------------

bool Run_setup::gather_capabilities(const util::zvalue& raw_capabilities)
{
	Gather_capabilities gc(capabilities);
	return gc.run(raw_capabilities);
}

bool Run_setup::negotiate()
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

				Configuration config{ algorithm, server_style, client_style };
				if (negotiate(config)) return true;
			}
		}
	}

	return false;
}

bool Run_setup::is_algorithm_supported(Algorithm algorithm) const
{
	const auto& algorithms{ capabilities.algorithms };
	return std::find(algorithms.begin(), algorithms.end(), algorithm) != algorithms.end();
}

bool Run_setup::is_server_style_supported(Server_style server_style) const
{
	const auto& server_styles{ capabilities.server_styles };
	return std::find(server_styles.begin(), server_styles.end(), server_style) != server_styles.end();
}

bool Run_setup::is_client_style_supported(Client_style client_style) const
{
	const auto& client_styles{ capabilities.client_styles };
	return std::find(client_styles.begin(), client_styles.end(), client_style) != client_styles.end();
}

bool Run_setup::negotiate(const Configuration& config)
{
	//(algorithm, server_style, client_style);
	st_xmysqlnd_msg__capabilities_set caps_set{ msg_factory.get__capabilities_set(&msg_factory) };
	//st_xmysqlnd_msg__capabilities_get caps_get{ msg_factory.get__capabilities_get(&msg_factory) };

	// "compression":{"client_style":"single", "server_style":"single", "algorithm": "defalte"})
	util::zvalue cap_names("compression");
	//util::zvalue cap_names("algorithm");
	util::zvalue cap_values(util::zvalue::create_object());
//	cap_values.set_property();

	//zval** capability_names{ util::mem_alloc<zval*>() };
	//zval** capability_values{ util::mem_alloc<zval*>() };


	//	const std::string capa_name{"compression_algorithm"};
	//	const struct st_xmysqlnd_on_error_bind on_error =
	//	{ xmysqlnd_session_data_handler_on_error, (void*) this };
	//	zval zv_capa_name;
	//	zval zv_algo_name;
	//	ZVAL_STRINGL(&zv_capa_name,capa_name.c_str(),capa_name.size());
	//	ZVAL_STRINGL(&zv_algo_name,algorithm.c_str(),algorithm.size());
	//	capability_names[0] = &zv_capa_name;
	//	capability_values[0] = &zv_algo_name;
	//	if( PASS == caps_set.send_request(&caps_set,
	//									  1,
	//									  capability_names,
	//									  capability_values))
	//	{
	//		DBG_INF_FMT("Cap. send request with compression_algorithm=%s success, reading response..!",
	//					algorithm.c_str());
	//		zval zvalue;
	//		ZVAL_NULL(&zvalue);
	//		caps_get.init_read(&caps_get, on_error);
	//		ret = caps_get.read_response(&caps_get,
	//									 &zvalue);
	//		if( ret == PASS ) {
	//			DBG_INF_FMT("Cap. response OK, enabled compression with %s",
	//						algorithm.c_str());
	//		} else {
	//			DBG_ERR_FMT("Negative response from the server, is not possible to enable the compression.");
	//			devapi::RAISE_EXCEPTION( err_msg_compres_negotiation_failed );
	//		}
	//		zval_ptr_dtor(&zvalue);
	//	}
	//}
	return false;
}

} // anonymous namespace

// ----------------------------------------------------------------------------

void run_setup(
	st_xmysqlnd_message_factory& msg_factory,
	const util::zvalue& capabilities)
{
	Run_setup rs(msg_factory);
	rs.run(capabilities);
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
