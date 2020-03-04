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
#ifndef XMYSQLND_COMPRESSION_H
#define XMYSQLND_COMPRESSION_H

#include "xmysqlnd_wireprotocol_types.h"

namespace Mysqlx { namespace Connection { class Compression; } }

namespace mysqlx {

namespace drv {

namespace compression {

struct Configuration;
struct Compressor;

struct Compress_result
{
	std::size_t uncompressed_size;
	std::string compressed_payload;
};

class Executor
{
public:
	Executor();
	Executor(const Executor& rhs) = delete;
	Executor& operator=(const Executor& rhs) = delete;
	Executor(Executor&& rhs) = delete;
	Executor& operator=(Executor&& rhs);
	~Executor();

public:
	void reset();
	void reset(const Configuration& cfg);

	bool enabled() const;

	Compress_result compress_message(
		xmysqlnd_client_message_type packet_type,
		std::size_t payload_size,
		util::byte* payload);

	void decompress_messages(
		const Mysqlx::Connection::Compression& message,
		Messages& messages);

private:
	std::unique_ptr<Compressor> compressor;
};

} // namespace compression

} // namespace drv

} // namespace mysqlx

#endif // XMYSQLND_COMPRESSION_H
