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
#include "xmysqlnd_compressor_lz4.h"
#include "util/exceptions.h"
#include "proto_gen/mysqlx_connection.pb.h"

#ifdef MYSQL_XDEVAPI_HAVE_LZ4
#include <lz4.h>
#include <lz4frame.h>
#endif

namespace mysqlx {

namespace drv {

namespace compression {

#ifdef MYSQL_XDEVAPI_HAVE_LZ4

namespace {

struct lz4_exception : public std::runtime_error
{
	lz4_exception(LZ4F_errorCode_t err_code, const char* prefix)
		: std::runtime_error(prepare_error_message(err_code, prefix))
	{
	}

	std::string prepare_error_message(LZ4F_errorCode_t err_code, const char* prefix) const
	{
		std::ostringstream msg;
		msg << prefix << '(' << err_code << "): " << LZ4F_getErrorName(err_code);
		return msg.str();
	}
};

// ----------------

class Compressor_lz4 : public Compressor
{
public:
	Compressor_lz4();
	~Compressor_lz4() override;

public:
	std::string compress(const util::bytes& uncompressed_payload) override;
	util::bytes decompress(const Mysqlx::Connection::Compression& message) override;

private:
	void checked_op(std::size_t result, const char* reason);
	void create_compression_ctx();
	void create_decompression_ctx();
	void free_decompression_ctx();
	void reset_decompression_context();

private:
	LZ4F_cctx_s* compress_ctx{ nullptr };
	LZ4F_dctx_s* decompress_ctx{ nullptr };
	LZ4F_preferences_t lz4_prefs{};
};

// ----------------

Compressor_lz4::Compressor_lz4()
{
	create_compression_ctx();
	create_decompression_ctx();

	lz4_prefs.autoFlush = 1;
	lz4_prefs.frameInfo.contentSize = 0;
}

Compressor_lz4::~Compressor_lz4()
{
	LZ4F_freeCompressionContext(compress_ctx);
	LZ4F_freeDecompressionContext(decompress_ctx);
}

std::string Compressor_lz4::compress(const util::bytes& uncompressed_payload)
{
	auto assert_lz4_result = [this](std::size_t result)
	{
		if (LZ4F_isError(result)) {
			LZ4F_freeCompressionContext(compress_ctx);
			compress_ctx = nullptr;
			throw lz4_exception(result, "error during lz4 compression");
		}
	};

	std::string compressed_payload(LZ4F_compressBound(uncompressed_payload.size(), &lz4_prefs), '\0');

	std::size_t lz4_op_result = LZ4F_compressBegin(
		compress_ctx,
		const_cast<char*>(compressed_payload.data()),
		compressed_payload.size(),
		&lz4_prefs);
	assert_lz4_result(lz4_op_result);
	std::size_t written_bytes = lz4_op_result;

#if LZ4_VERSION_NUMBER < 10703
	// for older versions add header size to compressed_payload length, else LZ4F_compressUpdate
	// will fail due to too small dst buffer (error status LZ4F_ERROR_dstMaxSize_tooSmall)
	compressed_payload.resize(compressed_payload.size() + written_bytes);
#endif

	lz4_op_result = LZ4F_compressUpdate(
		compress_ctx,
		const_cast<char*>(compressed_payload.data()) + written_bytes,
		compressed_payload.size() - written_bytes,
		uncompressed_payload.data(),
		uncompressed_payload.size(),
		nullptr);
	assert_lz4_result(lz4_op_result);
	written_bytes += lz4_op_result;

	lz4_op_result = LZ4F_compressEnd(
		compress_ctx,
		const_cast<char*>(compressed_payload.data()) + written_bytes,
		compressed_payload.size() - written_bytes,
		nullptr);
	assert_lz4_result(lz4_op_result);

	written_bytes += lz4_op_result;
	compressed_payload.resize(written_bytes);

	return compressed_payload;
}

util::bytes Compressor_lz4::decompress(const Mysqlx::Connection::Compression& message)
{
	const std::string& compressed_payload = message.payload();
	const std::size_t compressed_size = compressed_payload.size();
	std::size_t all_processed_bytes = 0;
	const char* src = compressed_payload.data();

	const std::size_t uncompressed_size = static_cast<std::size_t>(message.uncompressed_size());
	util::bytes uncompressed_payload(uncompressed_size);
	std::size_t all_written_bytes = 0;
	unsigned char* dest = uncompressed_payload.data();

	bool keep_processing = true;
	do {
		std::size_t bytes_to_process = compressed_size - all_processed_bytes;
		std::size_t bytes_to_write = uncompressed_size - all_written_bytes;

		const std::size_t result = LZ4F_decompress(
			decompress_ctx,
			dest + all_written_bytes,
			&bytes_to_write,
			src + all_processed_bytes,
			&bytes_to_process,
			nullptr);

		if (LZ4F_isError(result)) {
			reset_decompression_context();
			throw lz4_exception(result, "error during lz4 decompression");
		}

		all_processed_bytes += bytes_to_process;
		assert(all_processed_bytes <= compressed_size);

		all_written_bytes += bytes_to_write;
		assert(all_written_bytes <= uncompressed_size);

		keep_processing = (result != 0) && (bytes_to_process != 0);
	} while(keep_processing);

	return uncompressed_payload;
}

void Compressor_lz4::checked_op(std::size_t result, const char* reason)
{
	if (LZ4F_isError(result)) {
		throw lz4_exception(result, reason);
	}
}

void Compressor_lz4::create_compression_ctx()
{
	checked_op(
		LZ4F_createCompressionContext(&compress_ctx, LZ4F_VERSION),
		"cannot create lz4 compression context");
}

void Compressor_lz4::create_decompression_ctx()
{
	checked_op(
		LZ4F_createDecompressionContext(&decompress_ctx, LZ4F_VERSION),
		"cannot create lz4 decompression context");
}

void Compressor_lz4::free_decompression_ctx()
{
	checked_op(
		LZ4F_freeDecompressionContext(decompress_ctx),
		"cannot free lz4 decompression context");
}

void Compressor_lz4::reset_decompression_context()
{
#if LZ4_VERSION_NUMBER >= 10800
	LZ4F_resetDecompressionContext(decompress_ctx);
#else
	free_decompression_ctx();
	create_decompression_ctx();
#endif
}

} // anonymous namespace

// ------------------------------------------

bool is_compressor_lz4_available()
{
	return true;
}

Compressor* create_compressor_lz4()
{
	return new Compressor_lz4();
}

#else

bool is_compressor_lz4_available()
{
	return false;
}

Compressor* create_compressor_lz4()
{
	throw util::xdevapi_exception(util::xdevapi_exception::Code::compressor_not_available);
}

#endif // MYSQL_XDEVAPI_HAVE_LZ4

} // namespace compression

} // namespace drv

} // namespace mysqlx
