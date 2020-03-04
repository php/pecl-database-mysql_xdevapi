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
#ifdef MYSQL_XDEVAPI_HAVE_ZLIB
//#include
#endif

namespace mysqlx {

namespace drv {

namespace compression {

namespace {

#ifdef MYSQL_XDEVAPI_HAVE_ZLIB

class Compressor_zlib : public Compressor
{
public:
	Compressor_zlib();
	~Compressor_zlib() override;

public:
	std::size_t compress() override;
	std::size_t decompress() override;

private:
	z_stream u_zstream;       // Uncompression ZLib stream
	z_stream c_zstream;       // Compression ZLib stream

};

Compressor_zlib::Compressor_zlib(Protocol_compression& c)
{
	// Initial functions mapping, keep the internal implementation
	c_zstream.zalloc = Z_NULL;
	c_zstream.zfree = Z_NULL;
	c_zstream.opaque = Z_NULL;
	c_zstream.total_out = 0;

	// TODO: Make the compression level adjustable
	if (deflateInit(&c_zstream, 9) != Z_OK)
		throw_error("Could not initialize compression output stream");

	// Initial functions mapping, keep the internal implementation
	u_zstream.zalloc = Z_NULL;
	u_zstream.zfree = Z_NULL;
	u_zstream.opaque = Z_NULL;

	if (inflateInit(&u_zstream) != Z_OK)
		throw_error("Could not initialize compression input stream");
}

Compressor_zlib::~Compressor_zlib()
{
	deflateEnd(&c_zstream);
	inflateEnd(&u_zstream);
}


size_t Compressor_zlib::compress(byte *src, size_t len)
{
	size_t total_compressed_len = c_zstream.total_out;

	c_zstream.next_in = src;           // Input buffer with uncompressed data
	c_zstream.avail_in = (uInt)len;    // Length of uncompressed data

	/*
	TODO: Do smarter allocation for compression buffer since
	the upper bound might be quite redundant.
	*/
	size_t deflate_size = deflateBound(&c_zstream, (uLong)len);

	// This will reallocate the buffer if needed and get its address
	c_zstream.next_out = m_protocol_compression.get_out_buf(deflate_size);

	c_zstream.avail_out = (uInt)m_protocol_compression.get_out_buf_len();

	int res = deflate(&c_zstream, Z_SYNC_FLUSH);
	if (res != Z_OK)
	return 0;

	return c_zstream.total_out - total_compressed_len;
}

size_t Compressor_zlib::uncompress(byte *dst,
  size_t dest_size, size_t compressed_size,
  size_t &bytes_consumed)
{
	u_zstream.next_in = m_protocol_compression.get_inp_buf();
	u_zstream.avail_in = (uInt)compressed_size;

	u_zstream.next_out = dst;
	u_zstream.avail_out = (uInt)dest_size;
	int inflate_res = inflate(&u_zstream, Z_SYNC_FLUSH);

	if (inflate_res != Z_OK)
	{
		inflateReset(&u_zstream);
		return COMPRESSION_ERROR;
	}

	// The number of processed compressed bytes
	bytes_consumed = compressed_size - u_zstream.avail_in;

	// The number of uncompressed bytes
	return dest_size - u_zstream.avail_out;
}

#endif

} // anonymous namespace

// ------------------------------------------

bool is_compressor_zlib_available()
{
#ifdef MYSQL_XDEVAPI_HAVE_ZLIB
	return true;
#else
	return false;
#endif
}

Compressor* create_compressor_zlib()
{
//	assert(is_compressor_zlib_available());
#ifdef MYSQL_XDEVAPI_HAVE_ZLIB
	return new Compressor_zlib();
#else
	return nullptr;
#endif
}

} // namespace compression

} // namespace drv

} // namespace mysqlx
