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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "xmysqlnd_compression.h"
#include "xmysqlnd_compression_types.h"
#include "xmysqlnd_compressor.h"
#include "xmysqlnd_compressor_lz4.h"
#include "xmysqlnd_compressor_zlib.h"
#include "xmysqlnd_compressor_zstd.h"
#include "util/exceptions.h"
#include "util/types.h"
#include "util/value.h"
#include "proto_gen/mysqlx_connection.pb.h"

namespace mysqlx {

namespace drv {

namespace compression {

namespace {

const std::size_t Payload_length_size = 4;
const std::size_t Packet_type_size = 1;

template<typename Dest, typename Src>
Dest& cast_ref_to(Src& src)
{
	return *reinterpret_cast<Dest*>(&src);
}

// ------------------------------------------

class Payload_composer
{
public:
	Payload_composer(std::size_t msg_payload_size);
	util::bytes run(
		xmysqlnd_client_message_type msg_packet_type,
		std::size_t msg_payload_size,
		util::byte* msg_payload);

private:
	void add_length(std::size_t msg_payload_size);
	void add_packet_type(xmysqlnd_client_message_type msg_packet_type);
	void add_payload(
		std::size_t msg_payload_size,
		util::byte* msg_payload);

private:
	util::bytes buffer;
	util::bytes::iterator it;
};

// -----------------

Payload_composer::Payload_composer(std::size_t msg_payload_size)
{
	const std::size_t buffer_size = Payload_length_size + Packet_type_size + msg_payload_size;
	buffer.resize(buffer_size);
	it = buffer.begin();
}

util::bytes Payload_composer::run(
	xmysqlnd_client_message_type msg_packet_type,
	std::size_t msg_payload_size,
	util::byte* msg_payload)
{
	add_length(msg_payload_size);
	add_packet_type(msg_packet_type);
	add_payload(msg_payload_size, msg_payload);
	return buffer;
}

void Payload_composer::add_length(std::size_t msg_payload_size)
{
	std::uint32_t& buffer_length = cast_ref_to<std::uint32_t>(*it);
	buffer_length = static_cast<std::uint32_t>(Packet_type_size + msg_payload_size);
	std::advance(it, Payload_length_size);
}

void Payload_composer::add_packet_type(xmysqlnd_client_message_type msg_packet_type)
{
	*it = static_cast<util::byte>(msg_packet_type);
	std::advance(it, Packet_type_size);
}

void Payload_composer::add_payload(
	std::size_t msg_payload_size,
	util::byte* msg_payload)
{
	std::copy_n(
		msg_payload,
		msg_payload_size,
		it);
}

// ------------------------------------------

class Message_extractor
{
public:
	Message_extractor(Messages& messages);

public:
	void run(const util::bytes& uncompressed_payload);

private:
	std::size_t extract_length();
	xmysqlnd_server_message_type extract_type();
	util::bytes extract_payload(std::size_t msg_length);

private:
	Messages& messages;
	util::bytes::const_iterator it;
};

// -----------------

Message_extractor::Message_extractor(Messages& messages)
	: messages(messages)
{
}

void Message_extractor::run(const util::bytes& uncompressed_payload)
{
	it = uncompressed_payload.begin();

	while (it != uncompressed_payload.end()) {
		std::size_t msg_length = extract_length();
		xmysqlnd_server_message_type msg_type = extract_type();
		util::bytes msg_payload = extract_payload(msg_length);
		// due to some mysterious reasons this line causes vs2017 internal error
		// messages.emplace_back(msg_type, std::move(msg_payload));
		messages.push_back({msg_type, msg_payload});
	}
}

std::size_t Message_extractor::extract_length()
{
	std::size_t msg_length = cast_ref_to<const std::uint32_t>(*it);
	std::advance(it, Payload_length_size);
	return msg_length;
}

xmysqlnd_server_message_type Message_extractor::extract_type()
{
	xmysqlnd_server_message_type packet_type
		= static_cast<xmysqlnd_server_message_type>(*it);
	std::advance(it, Packet_type_size);
	return packet_type;
}

util::bytes Message_extractor::extract_payload(std::size_t msg_length)
{
	std::size_t payload_size = msg_length - Packet_type_size;
	util::bytes payload(it, it + payload_size);
	std::advance(it, payload_size);
	return payload;
}

} // anonymous namespace

// ------------------------------------------

Executor::Executor()
{
}

Executor& Executor::operator=(Executor&& rhs)
{
	if (this != &rhs) {
		compressor = std::move(rhs.compressor);
	}
	return *this;
}

Executor::~Executor()
{
}

// ----------------------------------------------------------------------------

namespace {

Compressor* create_compressor(const Configuration& cfg)
{
	assert(cfg.enabled());

	using compressor_creator = Compressor* (*)();
	using algorithm_to_compressor_creator = std::map<Algorithm, compressor_creator>;
	static const algorithm_to_compressor_creator compressor_creators{
		{ Algorithm::zstd_stream, create_compressor_zstd },
		{ Algorithm::lz4_message, create_compressor_lz4 },
		{ Algorithm::zlib_deflate_stream, create_compressor_zlib }
	};

	compressor_creator comp_creator{ compressor_creators.at(cfg.algorithm) };
	return comp_creator();
}

} // anonymous namespace

void Executor::reset()
{
	compressor.reset();
}

void Executor::reset(const Configuration& cfg)
{
	if (cfg.enabled()) {
		compressor.reset(create_compressor(cfg));
	} else {
		reset();
	}
}

bool Executor::enabled() const
{
	return compressor != nullptr;
}

// ----------------------------------------------------------------------------

Compress_result Executor::compress_message(
	xmysqlnd_client_message_type msg_packet_type,
	std::size_t msg_payload_size,
	util::byte* msg_payload)
{
	assert(enabled());
	Payload_composer payload_composer(msg_payload_size);
	const util::bytes& uncompressed_payload = payload_composer.run(msg_packet_type, msg_payload_size, msg_payload);
	return Compress_result{
		uncompressed_payload.size(),
		compressor->compress(uncompressed_payload)
	};
}

void Executor::decompress_messages(const Mysqlx::Connection::Compression& message, Messages& messages)
{
	assert(enabled());
	const util::bytes& uncompressed_payload{ compressor->decompress(message) };
	Message_extractor msg_extractor(messages);
	msg_extractor.run(uncompressed_payload);
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
