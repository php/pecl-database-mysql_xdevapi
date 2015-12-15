/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/

#ifndef XMYSQLND_ENUM_N_DEF_H
#define XMYSQLND_ENUM_N_DEF_H

//#define XMYSQLND_MAX_PACKET_SIZE		UINT32_MAX
#define XMYSQLND_MAX_PACKET_SIZE		(1024L * 1024L)  /* TODO: not -1 ? */
#define XMYSQLND_NODE_SESSION_CHARSET	"utf8"

typedef enum xmysqlnd_collected_stats
{
	XMYSQLND_STAT_BYTES_SENT,
	XMYSQLND_STAT_BYTES_RECEIVED,
	XMYSQLND_STAT_PACKETS_SENT,
	XMYSQLND_STAT_PACKETS_RECEIVED,
	XMYSQLND_STAT_PROTOCOL_OVERHEAD_IN,
	XMYSQLND_STAT_PROTOCOL_OVERHEAD_OUT,
	XMYSQLND_STAT_CLOSE_EXPLICIT,
	XMYSQLND_STAT_CLOSE_IMPLICIT,
	XMYSQLND_STAT_CLOSE_DISCONNECT,
	XMYSQLND_STAT_CONNECT_SUCCESS,
	XMYSQLND_STAT_CONNECT_FAILURE,
	XMYSQLND_STAT_CONNECT_REUSED,
	XMYSQLND_STAT_RECONNECT,
	XMYSQLND_STAT_PCONNECT_SUCCESS,
	XMYSQLND_STAT_OPENED_CONNECTIONS,
	XMYSQLND_STAT_OPENED_PERSISTENT_CONNECTIONS,
	XMYSQLND_STAT_LAST /* Should be always the last */
} enum_xmysqlnd_collected_stats;


typedef enum xmysqlnd_client_option
{
	XMYSQLND_OPT_READ_TIMEOUT,
} enum_xmysqlnd_client_option;


#endif	/* XMYSQLND_ENUM_N_DEF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
