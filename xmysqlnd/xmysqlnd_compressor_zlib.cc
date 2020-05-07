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
#include "xmysqlnd_compressor_zlib.h"
#include "util/exceptions.h"
#include "proto_gen/mysqlx_connection.pb.h"

#ifdef MYSQL_XDEVAPI_HAVE_ZLIB
#include <zlib.h>
#endif

namespace mysqlx {

namespace drv {

namespace compression {

#ifdef MYSQL_XDEVAPI_HAVE_ZLIB

namespace {

template<typename T>
uInt to_uint(T value)
{
	return static_cast<uInt>(value);
}

class Compressor_zlib : public Compressor
{
public:
	Compressor_zlib();
	~Compressor_zlib() override;

public:
	std::string compress(const util::bytes& uncompressed_payload) override;
	util::bytes decompress(const Mysqlx::Connection::Compression& message) override;

private:
	z_stream compress_stream;
	z_stream decompress_stream;
};

Compressor_zlib::Compressor_zlib()
{
	// compression
	compress_stream.zalloc = Z_NULL;
	compress_stream.zfree = Z_NULL;
	compress_stream.opaque = Z_NULL;
	compress_stream.total_out = 0;

	const int Compression_level = 9;
	if (deflateInit(&compress_stream, Compression_level) != Z_OK) {
		throw std::runtime_error("cannot initialize zlib compression stream");
	}

	// decompression
	decompress_stream.zalloc = Z_NULL;
	decompress_stream.zfree = Z_NULL;
	decompress_stream.opaque = Z_NULL;

	if (inflateInit(&decompress_stream) != Z_OK) {
		throw std::runtime_error("cannot initialize zlib decompression stream");
	}
}

Compressor_zlib::~Compressor_zlib()
{
	deflateEnd(&compress_stream);
	inflateEnd(&decompress_stream);
}

std::string Compressor_zlib::compress(const util::bytes& uncompressed_payload)
{
	compress_stream.next_in = const_cast<util::byte*>(uncompressed_payload.data());
	compress_stream.avail_in = to_uint(uncompressed_payload.size());

	const std::size_t previous_total_out = compress_stream.total_out;
	std::string compressed_payload(deflateBound(&compress_stream, to_uint(uncompressed_payload.size())), '\0');
	compress_stream.next_out = reinterpret_cast<util::byte*>(&compressed_payload.front());
	compress_stream.avail_out = to_uint(compressed_payload.size());

	if (deflate(&compress_stream, Z_SYNC_FLUSH) != Z_OK) {
		throw std::runtime_error("error during zlib compression");
	}

	compressed_payload.resize(compress_stream.total_out - previous_total_out);
	return compressed_payload;
}

util::bytes Compressor_zlib::decompress(const Mysqlx::Connection::Compression& message)
{
	const std::string& compressed_payload = message.payload();
	decompress_stream.next_in = reinterpret_cast<util::byte*>(const_cast<char*>(compressed_payload.data()));
	decompress_stream.avail_in = to_uint(compressed_payload.size());

	const std::size_t uncompressed_size = static_cast<std::size_t>(message.uncompressed_size());
	util::bytes uncompressed_payload(uncompressed_size);
	decompress_stream.next_out = uncompressed_payload.data();
	decompress_stream.avail_out = to_uint(uncompressed_size);

	if (inflate(&decompress_stream, Z_SYNC_FLUSH) != Z_OK) {
		inflateReset(&decompress_stream);
		throw std::runtime_error("error during zlib decompression");
	}

	assert((decompress_stream.avail_in == 0) && (decompress_stream.avail_out == 0));
	return uncompressed_payload;
}

} // anonymous namespace

// ------------------------------------------

bool is_compressor_zlib_available()
{
	return true;
}

Compressor* create_compressor_zlib()
{
	return new Compressor_zlib();
}

#else

bool is_compressor_zlib_available()
{
	return false;
}

Compressor* create_compressor_zlib()
{
	throw util::xdevapi_exception(util::xdevapi_exception::Code::compressor_not_available);
}

#endif // MYSQL_XDEVAPI_HAVE_ZLIB

} // namespace compression

} // namespace drv

} // namespace mysqlx
