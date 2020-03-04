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

// #define MYSQL_XDEVAPI_HAVE_ZSTD
// #ifdef MYSQL_XDEVAPI_HAVE_ZSTD
// // extern "C" {
// // #include "../extra/zstd.h"
// // #include "../extra/zstdframe.h"
// // #include "zstd.h"
// // #include "zstdframe.h"
// #include "O:/devapi-7.1.task/devapi/src/extra/zstd/zstd.h"
// #include "O:/devapi-7.1.task/devapi/src/extra/zstd/zstdframe.h"
// // }
// #endif

// namespace mysqlx {

// namespace drv {

// namespace compression {

// #ifdef MYSQL_XDEVAPI_HAVE_ZSTD

// namespace {

// class Compressor_zstd : public Compressor
// {
// public:
// 	Compressor_zstd();
// 	~Compressor_zstd() override;

// public:
// 	std::string compress(const util::bytes& uncompressed_payload) override;
// 	util::bytes decompress(const Mysqlx::Connection::Compression& message) override;

// private:
// 	ZSTDF_cctx_s* compress_ctx{ nullptr };
// 	ZSTDF_dctx_s* decompress_ctx{ nullptr };
// 	ZSTDF_preferences_t zstd_prefs{};
// };

// Compressor_zstd::Compressor_zstd()
// {
// 	// std::cout << "--------------- Compressor_zstd::Compressor_zstd" << std::endl;

// 	if (ZSTDF_isError(ZSTDF_createCompressionContext(&compress_ctx, ZSTDF_getVersion()))) {
// 		throw std::runtime_error("cannot create zstd compression context");
// 	}

// 	if (ZSTDF_isError(ZSTDF_createDecompressionContext(&decompress_ctx, ZSTDF_getVersion()))) {
// 		throw std::runtime_error("cannot create zstd decompression context");
// 	}

// 	zstd_prefs.autoFlush = 1;
// 	zstd_prefs.frameInfo.contentSize = 0;
// }

// Compressor_zstd::~Compressor_zstd()
// {
// 	ZSTDF_freeCompressionContext(compress_ctx);
// 	ZSTDF_freeDecompressionContext(decompress_ctx);
// }

// std::string Compressor_zstd::compress(const util::bytes& uncompressed_payload)
// {
// 	auto assert_zstd_result = [this](std::size_t result)
// 	{
// 		if (ZSTDF_isError(result)) {
// 			ZSTDF_freeCompressionContext(compress_ctx);
// 			compress_ctx = nullptr;
// 			throw std::runtime_error("error during zstd compression");
// 		}
// 	};

// 	std::string compressed_payload(ZSTDF_compressBound(uncompressed_payload.size(), &zstd_prefs), '\0');

// 	std::size_t zstd_op_result = ZSTDF_compressBegin(
// 		compress_ctx,
// 		const_cast<char*>(compressed_payload.data()),
// 		compressed_payload.size(),
// 		&zstd_prefs);
// 	assert_zstd_result(zstd_op_result);
// 	std::size_t written_bytes = zstd_op_result;

// 	zstd_op_result = ZSTDF_compressUpdate(
// 		compress_ctx,
// 		const_cast<char*>(compressed_payload.data()) + written_bytes,
// 		compressed_payload.size() - written_bytes,
// 		uncompressed_payload.data(),
// 		uncompressed_payload.size(),
// 		nullptr);
// 	assert_zstd_result(zstd_op_result);
// 	written_bytes += zstd_op_result;

// 	zstd_op_result = ZSTDF_compressEnd(
// 		compress_ctx,
// 		const_cast<char*>(compressed_payload.data()) + written_bytes,
// 		compressed_payload.size() - written_bytes,
// 		nullptr);
// 	assert_zstd_result(zstd_op_result);

// 	written_bytes += zstd_op_result;
// 	compressed_payload.resize(written_bytes);

// 	return compressed_payload;
// }

// util::bytes Compressor_zstd::decompress(const Mysqlx::Connection::Compression& message)
// {
// 	// std::cout << "--------------- zstd " << message.server_messages() << std::endl;
// 	if ( (int)message.server_messages() == 13 || (int)message.server_messages() == 12) {
// 		// std::cout << "--------------- trap " << std::endl;
// 	}
// 	const std::string& payload = message.payload();
// 	const std::size_t compressed_size = payload.length();
// //	std::size_t bytes_to_process = compressed_size;
// 	std::size_t all_processed_bytes = 0;
// 	const char* src = payload.data();

// 	util::bytes uncompressed_payload;
// 	const std::size_t uncompressed_size = static_cast<std::size_t>(message.uncompressed_size());
// //	std::size_t bytes_to_write = uncompressed_size;
// 	std::size_t all_written_bytes = 0;
// 	if (uncompressed_payload.size() < uncompressed_size) {
// 		uncompressed_payload.resize(uncompressed_size);
// 	}
// 	unsigned char* dest = uncompressed_payload.data();


// 	// std::cout << "compressed_size " << compressed_size << std::endl;
// 	// std::cout << "uncompressed_size " << uncompressed_size << std::endl;
// 	bool keep_processing = true;
// 	do {
// 		std::size_t bytes_to_process = compressed_size - all_processed_bytes;
// 		std::size_t bytes_to_write = uncompressed_size - all_written_bytes;

// 		// std::cout << "bytes_to_process " << bytes_to_process << std::endl;
// 		// std::cout << "bytes_to_write " << bytes_to_write << std::endl;


// 		const std::size_t result = ZSTDF_decompress(
// 			decompress_ctx,
// 			dest + all_written_bytes,
// 			&bytes_to_write,
// 			src + all_processed_bytes,
// 			&bytes_to_process,
// 			nullptr);

// 		if (ZSTDF_isError(result)) {
// 			ZSTDF_resetDecompressionContext(decompress_ctx);
// 			throw std::runtime_error("error during zstd decompression");
// 		}

// 		all_processed_bytes += bytes_to_process;
// 		assert(all_processed_bytes <= compressed_size);

// 		all_written_bytes += bytes_to_write;
// 		assert(all_written_bytes <= uncompressed_size);

// 		// std::cout << "bytes_to_process " << bytes_to_process << std::endl;
// 		// std::cout << "bytes_to_write " << bytes_to_write << std::endl;

// 		// std::cout << "all_processed_bytes " << all_processed_bytes << std::endl;
// 		// std::cout << "all_written_bytes " << all_written_bytes << std::endl;

// 		keep_processing = (result != 0) && (bytes_to_process != 0);
// 		// if (result == 0 || current_bytes_processed == 0 /*bytes_to_write == 0*/)
// 		// 	break;
// 	} while(keep_processing);
// 	// bytes_consumed = bytes_processed;
// 	// return initial_dest_size - dest_size;

// 	return uncompressed_payload;

// 	// std::cout << "---------------" << std::endl;
// }

// } // anonymous namespace

// // ------------------------------------------

// bool is_compressor_zstd_available()
// {
// 	return true;
// }

// Compressor* create_compressor_zstd()
// {
// 	return new Compressor_zstd();
// }

// #else

// bool is_compressor_zstd_available()
// {
// 	// return true;
// 	return false;
// }

// Compressor* create_compressor_zstd()
// {
// 	throw util::xdevapi_exception(util::xdevapi_exception::Code::compressor_not_available);
// }

// #endif // MYSQL_XDEVAPI_HAVE_ZSTD

// } // namespace compression

// } // namespace drv

// } // namespace mysqlx


namespace mysqlx {

namespace drv {

namespace compression {

#ifdef MYSQL_XDEVAPI_HAVE_ZSTD

namespace {

class Compressor_zstd : public Compressor
{
public:
	std::size_t compress() override;
	std::size_t decompress() override;

private:
	z_stream u_zstream;       // Uncompression ZLib stream
	z_stream c_zstream;       // Compression ZLib stream
	bool zstd_inited = false;

};

class Compression_zstd : public Compression_algorithm
{

void init();

public:

Compression_zstd(Protocol_compression& c) :
Compression_algorithm(c)
{ init(); }

size_t compress(byte *src, size_t len) override;
size_t uncompress(byte *dst, size_t dest_size, size_t compressed_size,
				size_t &bytes_consumed) override;
~Compression_zstd();

};

void Compression_zstd::init()
{
  if (m_zstd_inited)
    return;

  // Initial functions mapping, keep the internal implementation
  m_c_zstream.zalloc = Z_NULL;
  m_c_zstream.zfree = Z_NULL;
  m_c_zstream.opaque = Z_NULL;
  m_c_zstream.total_out = 0;

  // TODO: Make the compression level adjustable
  if (deflateInit(&m_c_zstream, 9) != Z_OK)
    throw_error("Could not initialize compression output stream");

  // Initial functions mapping, keep the internal implementation
  m_u_zstream.zalloc = Z_NULL;
  m_u_zstream.zfree = Z_NULL;
  m_u_zstream.opaque = Z_NULL;

  if (inflateInit(&m_u_zstream) != Z_OK)
    throw_error("Could not initialize compression input stream");

  m_zstd_inited = true;
}


size_t Compression_zstd::compress(byte *src, size_t len)
{
  size_t total_compressed_len = m_c_zstream.total_out;

  m_c_zstream.next_in = src;           // Input buffer with uncompressed data
  m_c_zstream.avail_in = (uInt)len;    // Length of uncompressed data

  /*
    TODO: Do smarter allocation for compression buffer since
    the upper bound might be quite redundant.
  */
  size_t deflate_size = deflateBound(&m_c_zstream, (uLong)len);

  // This will reallocate the buffer if needed and get its address
  m_c_zstream.next_out = m_protocol_compression.get_out_buf(deflate_size);

  m_c_zstream.avail_out = (uInt)m_protocol_compression.get_out_buf_len();

  int res = deflate(&m_c_zstream, Z_SYNC_FLUSH);
  if (res != Z_OK)
    return 0;

  return m_c_zstream.total_out - total_compressed_len;
}


size_t Compression_zstd::uncompress(byte *dst,
  size_t dest_size, size_t compressed_size,
  size_t &bytes_consumed)
{

  m_u_zstream.next_in = m_protocol_compression.get_inp_buf();
  m_u_zstream.avail_in = (uInt)compressed_size;

  m_u_zstream.next_out = dst;
  m_u_zstream.avail_out = (uInt)dest_size;
  int inflate_res = inflate(&m_u_zstream, Z_SYNC_FLUSH);

  if (inflate_res != Z_OK)
  {
    inflateReset(&m_u_zstream);
    return COMPRESSION_ERROR;
  }

  // The number of processed compressed bytes
  bytes_consumed = compressed_size - m_u_zstream.avail_in;

  // The number of uncompressed bytes
  return dest_size - m_u_zstream.avail_out;
}


Compression_zstd::~Compression_zstd()
{
  if (m_zstd_inited)
  {
    deflateEnd(&m_c_zstream);
    inflateEnd(&m_u_zstream);
  }
}




// ------------------------------------------

std::size_t Compressor_zstd::compress()
{
	return 0;
}

std::size_t Compressor_zstd::decompress()
{
	return 0;
}

} // anonymous namespace

#endif // MYSQL_XDEVAPI_HAVE_ZSTD

// ------------------------------------------

bool is_compressor_zstd_available()
{
#ifdef MYSQL_XDEVAPI_HAVE_ZSTD
	return true;
#else
	return false;
#endif
}

Compressor* create_compressor_zstd()
{
	// assert(is_compressor_zstd_available());
#ifdef MYSQL_XDEVAPI_HAVE_ZSTD
	return new Compressor_zstd();
#else
	return nullptr;
#endif
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
