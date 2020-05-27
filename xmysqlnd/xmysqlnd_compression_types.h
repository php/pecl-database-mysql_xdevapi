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
#ifndef XMYSQLND_COMPRESSION_TYPES_H
#define XMYSQLND_COMPRESSION_TYPES_H

#include <optional>

namespace mysqlx {

namespace drv {

namespace compression {

enum class Policy {
	required,
	preferred,
	disabled
};

enum class Algorithm
{
	none,
	zstd_stream,
	lz4_message,
	zlib_deflate_stream
};

#ifdef MYSQL_XDEVAPI_DEV_MODE
const std::size_t Client_compression_threshold = 200;
#else
const std::size_t Client_compression_threshold = 1000;
#endif

struct Configuration
{
	Algorithm algorithm;

	// informs the server that it can combine multiple types of
	// messages into a single compressed payload
	std::optional<bool> combine_mixed_messages;

	// if set this informs the server that it should not combine
	// more than x number of messages inside a single compressed payload.
	std::optional<int> max_combine_messages;

	Configuration(Algorithm algorithm = Algorithm::none);
	bool enabled() const;
};

} // namespace compression

} // namespace drv

} // namespace mysqlx

#endif // XMYSQLND_COMPRESSION_TYPES_H
