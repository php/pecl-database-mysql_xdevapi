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
#include "xmysqlnd_compressor.h"
#include "xmysqlnd_compressor_zstd.h"
#include "util/exceptions.h"
#include "proto_gen/mysqlx_connection.pb.h"

#ifdef MYSQL_XDEVAPI_HAVE_ZSTD
#include <zstd.h>
#endif

namespace mysqlx {

namespace drv {

namespace compression {

#ifdef MYSQL_XDEVAPI_HAVE_ZSTD

namespace {

class Compressor_zstd : public Compressor
{
public:
	Compressor_zstd();
	~Compressor_zstd() override;

public:
	std::string compress(const util::bytes& uncompressed_payload) override;
	util::bytes decompress(const Mysqlx::Connection::Compression& message) override;

private:
	ZSTD_CStream* compression_stream = nullptr;
	ZSTD_DStream* decompression_stream = nullptr;
};

Compressor_zstd::Compressor_zstd()
{
	compression_stream = ZSTD_createCStream();
	const int Compression_level = -1;
	if (ZSTD_isError(ZSTD_initCStream(compression_stream, Compression_level))) {
		throw std::runtime_error("cannot create zstd compression stream");
	}

	decompression_stream = ZSTD_createDStream();
	if (ZSTD_isError(ZSTD_initDStream(decompression_stream))) {
		throw std::runtime_error("cannot create zstd decompression stream");
	}
}

Compressor_zstd::~Compressor_zstd()
{
	ZSTD_freeCStream(compression_stream);
	ZSTD_freeDStream(decompression_stream);
}

std::string Compressor_zstd::compress(const util::bytes& uncompressed_payload)
{
	ZSTD_inBuffer in_buffer{
		uncompressed_payload.data(),
		uncompressed_payload.size(),
		0
	};

	std::string compressed_payload(ZSTD_compressBound(uncompressed_payload.size()), '\0');
	ZSTD_outBuffer out_buffer{
		const_cast<char*>(compressed_payload.data()),
		compressed_payload.size(),
		0
	};

	while (in_buffer.pos < in_buffer.size) {
		std::size_t op_result = ZSTD_compressStream(compression_stream, &out_buffer, &in_buffer);
		if (ZSTD_isError(op_result)) {
			throw std::runtime_error("error during zstd compression");
		}
	}

	std::size_t op_result = ZSTD_flushStream(compression_stream, &out_buffer);
	if (ZSTD_isError(op_result)) {
		throw std::runtime_error("error during zstd compression flush");
	}

	compressed_payload.resize(out_buffer.pos);
	return compressed_payload;
}

util::bytes Compressor_zstd::decompress(const Mysqlx::Connection::Compression& message)
{
	const std::string& compressed_payload = message.payload();
	ZSTD_inBuffer in_buffer{
		compressed_payload.data(),
		compressed_payload.size(),
		0
	};

	const std::size_t uncompressed_size = static_cast<std::size_t>(message.uncompressed_size());
	util::bytes uncompressed_payload(uncompressed_size);
	ZSTD_outBuffer out_buffer{
		uncompressed_payload.data(),
		uncompressed_size,
		0
	};

	while ((out_buffer.pos < out_buffer.size) && (in_buffer.pos < in_buffer.size)) {
		std::size_t op_result = ZSTD_decompressStream(decompression_stream, &out_buffer, &in_buffer);
		if (ZSTD_isError(op_result)) {
			throw std::runtime_error("error during zstd decompression");
		}
	}

	assert(compressed_payload.size() == in_buffer.pos);
	assert(uncompressed_payload.size() == out_buffer.pos);
	return uncompressed_payload;
}

} // anonymous namespace

// ------------------------------------------

bool is_compressor_zstd_available()
{
	return true;
}

Compressor* create_compressor_zstd()
{
	return new Compressor_zstd();
}

#else

bool is_compressor_zstd_available()
{
	return false;
}

Compressor* create_compressor_zstd()
{
	throw util::xdevapi_exception(util::xdevapi_exception::Code::compressor_not_available);
}

#endif // MYSQL_XDEVAPI_HAVE_ZSTD

} // namespace compression

} // namespace drv

} // namespace mysqlx
