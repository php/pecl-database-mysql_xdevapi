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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_PROTOCOL_DUMPER_H
#define XMYSQLND_PROTOCOL_DUMPER_H

namespace mysqlx {

namespace drv {

void xmysqlnd_dump_string_to_log(const char * prefix, const char * s, const size_t len);
void xmysqlnd_dump_server_message(const zend_uchar packet_type, const void * payload, const int payload_size);
void xmysqlnd_dump_client_message(const zend_uchar packet_type, const void * payload, const int payload_size);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_PROTOCOL_DUMPER_H */
