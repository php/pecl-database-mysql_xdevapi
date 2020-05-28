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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "xmysqlnd_compression_setup.h"
#include "xmysqlnd_compressor_lz4.h"
#include "xmysqlnd_compressor_zlib.h"
#include "xmysqlnd_compressor_zstd.h"
#include "xmysqlnd_wireprotocol.h"
#include "util/exceptions.h"
#include "util/types.h"
#include "util/value.h"

namespace mysqlx {

namespace drv {

namespace compression {

namespace {

const std::string PROPERTY_ALGORITHM{ "algorithm" };
const std::string PROPERTY_SERVER_COMBINE_MIXED_MESSAGES{ "server_combine_mixed_messages" };
const std::string PROPERTY_CLIENT_MAX_COMBINE_MESSAGES{ "server_max_combine_messages" };

const std::string ALGORITHM_ZSTD_STREAM{ "zstd_stream" };
const std::string ALGORITHM_LZ4_MESSAGE{ "lz4_message" };
const std::string ALGORITHM_ZLIB_STREAM{ "zlib_stream" };
const std::string ALGORITHM_DEFLATE_STREAM{ "deflate_stream" };
const std::string ALGORITHM_ZLIB_DEFLATE_STREAM{ "deflate_stream" };

const std::string ALGORITHM_ALIAS_ZSTD{ "zstd" };
const std::string ALGORITHM_ALIAS_LZ4{ "lz4" };
const std::string ALGORITHM_ALIAS_ZLIB{ "zlib" };
const std::string ALGORITHM_ALIAS_DEFLATE{ "deflate" };

// ----------------------------------------------------------------------------

using Algorithms = std::vector<Algorithm>;

Algorithm to_algorithm(const std::string& algorithm_name)
{
	using name_to_algorithm = std::map<std::string, Algorithm, util::iless>;
	static const name_to_algorithm algorithm_mapping{
		{ ALGORITHM_ZSTD_STREAM, Algorithm::zstd_stream },
		{ ALGORITHM_LZ4_MESSAGE, Algorithm::lz4_message },
		{ ALGORITHM_DEFLATE_STREAM, Algorithm::zlib_deflate_stream },
		{ ALGORITHM_ZLIB_STREAM, Algorithm::zlib_deflate_stream },

		{ ALGORITHM_ALIAS_ZSTD, Algorithm::zstd_stream },
		{ ALGORITHM_ALIAS_LZ4, Algorithm::lz4_message },
		{ ALGORITHM_ALIAS_ZLIB, Algorithm::zlib_deflate_stream },
		{ ALGORITHM_ALIAS_DEFLATE, Algorithm::zlib_deflate_stream },
	};

	auto it{ algorithm_mapping.find(algorithm_name) };
	return (it != algorithm_mapping.end()) ? it->second : Algorithm::none;
}

Algorithms to_algorithms(const util::std_strings& algorithm_names)
{
	Algorithms algorithms;
	for (const std::string& algorithm_name : algorithm_names) {
		const Algorithm algorithm{ to_algorithm(algorithm_name) };
		if (algorithm != Algorithm::none) {
			algorithms.push_back(algorithm);
		}
	}
	return algorithms;
}


// ----------------------------------------------------------------------------

struct Capabilities
{
	Algorithms supported_algorithms;
};

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

	return add_supported_algorithms(compression_caps);
}

void Gather_capabilities::add_supported_algorithm(const util::zvalue& zalgorithm)
{
	if (!zalgorithm.is_string()) return;

	const std::string& algorithm_name{ zalgorithm.to_std_string() };
	const Algorithm algorithm{ to_algorithm(algorithm_name) };
	if (algorithm != Algorithm::none) {
		capabilities.supported_algorithms.push_back(algorithm);
	}
}

bool Gather_capabilities::add_supported_algorithms(const util::zvalue& compression_caps)
{
	const util::zvalue& algorithms{ compression_caps.get_property(PROPERTY_ALGORITHM) };
	if (!algorithms.is_array()) return false;

	for (const auto& algorithm : algorithms.values()) {
		add_supported_algorithm(algorithm);
	}

	return !capabilities.supported_algorithms.empty();
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

private:
	static const enum_hnd_func_status handler_on_error(
		void* context,
		const unsigned int code,
		const util::string_view& sql_state,
		const util::string_view& message);

private:
	st_xmysqlnd_message_factory& msg_factory;
};

// ------------------------------------------

Negotiate::Negotiate(st_xmysqlnd_message_factory& msg_factory)
	: msg_factory(msg_factory)
{
}

bool Negotiate::run(const Configuration& config)
{
	/*
		samples:
		"compression":{"algorithm": "deflate_stream", "server_max_combine_messages" : 10 })
		"compression":{"algorithm": "lz4_message", "server_combine_mixed_messages" : true })
	*/
	util::zvalue cap_compression_name("compression");
	util::zvalue cap_compression_value(util::zvalue::create_object());
	cap_compression_value.set_property(PROPERTY_ALGORITHM, to_string(config.algorithm));
	if (config.combine_mixed_messages) {
		cap_compression_value.set_property(PROPERTY_SERVER_COMBINE_MIXED_MESSAGES, *config.combine_mixed_messages);
	}
	if (config.max_combine_messages) {
		cap_compression_value.set_property(PROPERTY_CLIENT_MAX_COMBINE_MESSAGES, *config.max_combine_messages);
	}

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
		{ Algorithm::zstd_stream, ALGORITHM_ZSTD_STREAM },
		{ Algorithm::lz4_message, ALGORITHM_LZ4_MESSAGE },
		{ Algorithm::zlib_deflate_stream, ALGORITHM_DEFLATE_STREAM },
	};
	return algorithm_mapping.at(algorithm);
}

const enum_hnd_func_status Negotiate::handler_on_error(
	void* context,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	throw util::xdevapi_exception(code, sql_state, message);
}

// ----------------------------------------------------------------------------

class Setup
{
public:
	Setup(const Setup_data& data);

public:
	Configuration run(const util::zvalue& capabilities);

private:
	Algorithms prepare_algorithms_to_negotiate(
		const boost::optional<util::std_strings>& algorithms_custom_order) const;
	bool gather_capabilities(const util::zvalue& raw_capabilities);
	bool negotiate();
	bool negotiate(const Configuration& config);

	bool is_algorithm_supported(Algorithm algorithm) const;
	bool is_algorithm_supported_by_server(Algorithm algorithm) const;
	bool is_any_compressor_available() const;
	bool is_compressor_available(const Algorithm algorithm) const;

private:
	const Policy policy;
	const Algorithms algorithms;
	st_xmysqlnd_message_factory& msg_factory;
	Capabilities capabilities;
	Configuration negotiated_config;
};

// ------------------------------------------

Setup::Setup(const Setup_data& data)
	: policy(data.policy)
	, algorithms(prepare_algorithms_to_negotiate(data.algorithms))
	, msg_factory(data.msg_factory)
{
}

Configuration Setup::run(const util::zvalue& raw_capabilities)
{
	if (policy == compression::Policy::disabled) {
		return negotiated_config;
	}

	const bool required = (policy == compression::Policy::required);

	if (!gather_capabilities(raw_capabilities) && required) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::compression_not_supported);
	}

	if (required && !is_any_compressor_available()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::compressor_not_available);
	}

	if (!negotiate() && required) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::compression_negotiation_failure);
	}

	return negotiated_config;
}

// ------------------------------------------

Algorithms Setup::prepare_algorithms_to_negotiate(
	const boost::optional<util::std_strings>& algorithms_custom_order) const
{
	if (algorithms_custom_order) {
		// prepare custom list of algorithms to negotiate
		return to_algorithms(*algorithms_custom_order);
	}

	// default order of algorithms to negotiate
	return Algorithms{
		Algorithm::zstd_stream,
		Algorithm::lz4_message,
		Algorithm::zlib_deflate_stream
	};
}

bool Setup::gather_capabilities(const util::zvalue& raw_capabilities)
{
	Gather_capabilities gc(capabilities);
	return gc.run(raw_capabilities);
}

bool Setup::negotiate()
{
	for (auto algorithm : algorithms) {
		if (!is_algorithm_supported(algorithm)) continue;

		Configuration config(algorithm);
		if (negotiate(config)) {
			negotiated_config = config;
			return true;
		}
	}

	return false;
}

bool Setup::negotiate(const Configuration& config)
{
	Negotiate negotiate(msg_factory);
	return negotiate.run(config);
}

// ---------------------

bool Setup::is_algorithm_supported(Algorithm algorithm) const
{
	return is_algorithm_supported_by_server(algorithm) && is_compressor_available(algorithm);
}

bool Setup::is_algorithm_supported_by_server(Algorithm algorithm) const
{
	const auto& algorithms{ capabilities.supported_algorithms };
	return std::find(
		algorithms.begin(),
		algorithms.end(),
		algorithm) != algorithms.end();
}

bool Setup::is_any_compressor_available() const
{
	return std::any_of(
		algorithms.begin(),
		algorithms.end(),
		[this](Algorithm algorithm) { return is_compressor_available(algorithm); }
	);
}

bool Setup::is_compressor_available(const Algorithm algorithm) const
{
	using compressor_checker = bool (*)();
	using algorithm_to_compressor_checker = std::map<Algorithm, compressor_checker>;
	static const algorithm_to_compressor_checker compressor_checkers{
		{ Algorithm::zstd_stream, is_compressor_zstd_available },
		{ Algorithm::lz4_message, is_compressor_lz4_available },
		{ Algorithm::zlib_deflate_stream, is_compressor_zlib_available }
	};

	auto it{ compressor_checkers.find(algorithm) };
	if (it == compressor_checkers.end()) return false;
	compressor_checker comp_checker{ it->second };
	return comp_checker();
}

} // anonymous namespace

// ----------------------------------------------------------------------------

Configuration run_setup(const Setup_data& data)
{
	Setup setup(data);
	return setup.run(data.capabilities);
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
