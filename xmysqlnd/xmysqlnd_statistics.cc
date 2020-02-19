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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd_enum_n_def.h"
#include "php_mysql_xdevapi.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace drv {

PHP_MYSQL_XDEVAPI_API MYSQLND_STATS* xmysqlnd_global_stats{nullptr};

const MYSQLND_STRING xmysqlnd_stats_values_names[XMYSQLND_STAT_LAST] =
{
	{ util::literal_to_mysqlnd_str("bytes_sent") },
	{ util::literal_to_mysqlnd_str("bytes_received") },
	{ util::literal_to_mysqlnd_str("packets_sent") },
	{ util::literal_to_mysqlnd_str("packets_received") },
	{ util::literal_to_mysqlnd_str("protocol_overhead_in") },
	{ util::literal_to_mysqlnd_str("protocol_overhead_out") },
	{ util::literal_to_mysqlnd_str("explicit_close") },
	{ util::literal_to_mysqlnd_str("implicit_close") },
	{ util::literal_to_mysqlnd_str("disconnect_close") },
	{ util::literal_to_mysqlnd_str("connect_success") },
	{ util::literal_to_mysqlnd_str("connect_failure") },
	{ util::literal_to_mysqlnd_str("connection_reused") },
	{ util::literal_to_mysqlnd_str("reconnect") },
	{ util::literal_to_mysqlnd_str("pconnect_success") },
	{ util::literal_to_mysqlnd_str("active_connections") },
	{ util::literal_to_mysqlnd_str("active_persistent_connections") },
};

PHP_MYSQL_XDEVAPI_API void
_xmysqlnd_get_client_stats(MYSQLND_STATS * stats_ptr, zval *return_value ZEND_FILE_LINE_DC)
{
#if ZEND_DEBUG
	UNUSED(__zend_lineno);
	UNUSED(__zend_filename);
#endif

	MYSQLND_STATS stats;
	DBG_ENTER("_xmysqlnd_get_client_stats");
	if (!stats_ptr) {
		memset(&stats, 0, sizeof(stats));
		stats_ptr = &stats;
	}
	mysqlnd_fill_stats_hash(stats_ptr, xmysqlnd_stats_values_names, return_value ZEND_FILE_LINE_CC);
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
