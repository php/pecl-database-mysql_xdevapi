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
#ifndef XMYSQLND_COMPRESSOR_H
#define XMYSQLND_COMPRESSOR_H

#include "xmysqlnd_wireprotocol_types.h"

namespace Mysqlx { namespace Connection { class Compression; } }

namespace mysqlx {

namespace drv {

namespace compression {

struct Compressor
{
	virtual ~Compressor() = default;

	virtual std::string compress(const util::bytes& uncompressed_payload) = 0;
	virtual util::bytes decompress(const Mysqlx::Connection::Compression& message) = 0;
};

} // namespace compression

} // namespace drv

} // namespace mysqlx

#endif
