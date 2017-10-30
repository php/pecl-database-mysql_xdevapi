/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
extern "C" {
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
}
#include "xmysqlnd_enum_n_def.h"
#include "php_mysql_xdevapi.h"

namespace mysqlx {

namespace drv {

PHP_MYSQL_XDEVAPI_API MYSQLND_STATS *xmysqlnd_global_stats = NULL;



/* {{{ mysqlnd_stats_values_names */
const MYSQLND_STRING xmysqlnd_stats_values_names[XMYSQLND_STAT_LAST] =
{
	{ MYSQLND_STR_W_LEN("bytes_sent") },
	{ MYSQLND_STR_W_LEN("bytes_received") },
	{ MYSQLND_STR_W_LEN("packets_sent") },
	{ MYSQLND_STR_W_LEN("packets_received") },
	{ MYSQLND_STR_W_LEN("protocol_overhead_in") },
	{ MYSQLND_STR_W_LEN("protocol_overhead_out") },
	{ MYSQLND_STR_W_LEN("explicit_close") },
	{ MYSQLND_STR_W_LEN("implicit_close") },
	{ MYSQLND_STR_W_LEN("disconnect_close") },
	{ MYSQLND_STR_W_LEN("connect_success") },
	{ MYSQLND_STR_W_LEN("connect_failure") },
	{ MYSQLND_STR_W_LEN("connection_reused") },
	{ MYSQLND_STR_W_LEN("reconnect") },
	{ MYSQLND_STR_W_LEN("pconnect_success") },
	{ MYSQLND_STR_W_LEN("active_connections") },
	{ MYSQLND_STR_W_LEN("active_persistent_connections") },
};
/* }}} */


/* {{{ _xmysqlnd_get_client_stats */
PHP_MYSQL_XDEVAPI_API void
_xmysqlnd_get_client_stats(MYSQLND_STATS * stats_ptr, zval *return_value ZEND_FILE_LINE_DC)
{
	MYSQLND_STATS stats;
	DBG_ENTER("_xmysqlnd_get_client_stats");
	if (!stats_ptr) {
		memset(&stats, 0, sizeof(stats));
		stats_ptr = &stats;
	}
	mysqlnd_fill_stats_hash(stats_ptr, xmysqlnd_stats_values_names, return_value ZEND_FILE_LINE_CC);
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
