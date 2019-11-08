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
#ifndef XMYSQLND_COMPRESSION_H
#define XMYSQLND_COMPRESSION_H

#include <vector>

namespace mysqlx {

namespace util { class zvalue; }

namespace drv {

struct st_xmysqlnd_message_factory;

namespace compression {

enum class Algorithm
{
	none,
	zstd_stream,
	lz4_message,
	zlib_deflate_stream
};

enum class Server_style {
	none,
	single,
	multiple,
	group
};

enum class Client_style {
	none,
	single,
	multiple,
	group
};

using Algorithms = std::vector<Algorithm>;
using Server_styles = std::vector<Server_style>;
using Client_styles = std::vector<Client_style>;

struct Capabilities
{
	Algorithms algorithms;
	Server_styles server_styles;
	Client_styles client_styles;
};

struct Configuration
{
	Algorithm algorithm;
	Server_style server_style;
	Client_style client_style;

	Configuration(
		Algorithm algorithm = Algorithm::none,
		Server_style server_style = Server_style::none,
		Client_style client_style = Client_style::none);
	bool enabled() const;
};

bool run_setup(
	st_xmysqlnd_message_factory& msg_factory,
	const util::zvalue& capabilities,
	Configuration& negotiated_config);

} // namespace compression

} // namespace drv

} // namespace mysqlx

#endif // XMYSQLND_COMPRESSION_H
