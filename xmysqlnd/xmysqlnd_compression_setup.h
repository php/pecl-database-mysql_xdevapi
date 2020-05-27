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
#ifndef XMYSQLND_COMPRESSION_SETUP_H
#define XMYSQLND_COMPRESSION_SETUP_H

#include "xmysqlnd_compression_types.h"
#include "util/strings.h"

namespace mysqlx {

namespace util { class zvalue; }

namespace drv {

struct st_xmysqlnd_message_factory;

namespace compression {

struct Setup_data
{
	Policy policy;
	const boost::optional<util::std_strings>& algorithms;
	st_xmysqlnd_message_factory& msg_factory;
	const util::zvalue& capabilities;
};

Configuration run_setup(const Setup_data& data);

} // namespace compression

} // namespace drv

} // namespace mysqlx

#endif // XMYSQLND_COMPRESSION_SETUP_H
