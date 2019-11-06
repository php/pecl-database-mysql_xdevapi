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
#ifndef XMYSQLND_PRIV_H
#define XMYSQLND_PRIV_H

namespace mysqlx {

namespace drv {

#ifndef XMYSQLND_CORE_STATISTICS_DISABLED

#define XMYSQLND_INC_GLOBAL_STATISTIC(statistic) \
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4389) \
	MYSQLND_INC_STATISTIC(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, (enum_mysqlnd_collected_stats)(statistic)) \
	MYSQLX_RESTORE_WARNINGS()

#define XMYSQLND_DEC_GLOBAL_STATISTIC(statistic) \
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4389) \
	MYSQLND_DEC_STATISTIC(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, (enum_mysqlnd_collected_stats)(statistic)) \
	MYSQLX_RESTORE_WARNINGS()

#define XMYSQLND_INC_GLOBAL_STATISTIC_W_VALUE2(statistic1, value1, statistic2, value2) \
	MYSQLND_INC_STATISTIC_W_VALUE2(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, (statistic1), (value1), (statistic2), (value2))

#define XMYSQLND_INC_SESSION_STATISTIC(session_stats, statistic) \
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4389) \
	MYSQLND_INC_STATISTIC(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, (enum_mysqlnd_collected_stats)(statistic)); \
	MYSQLND_INC_STATISTIC(MYSQL_XDEVAPI_G(collect_statistics), (session_stats), (enum_mysqlnd_collected_stats)(statistic)); \
	MYSQLX_RESTORE_WARNINGS()

#define XMYSQLND_INC_SESSION_STATISTIC_W_VALUE(session_stats, statistic, value) \
	MYSQLND_INC_STATISTIC_W_VALUE(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, (enum_mysqlnd_collected_stats)(statistic), (value)); \
	MYSQLND_INC_STATISTIC_W_VALUE(MYSQL_XDEVAPI_G(collect_statistics), (session_stats), (enum_mysqlnd_collected_stats)(statistic), (value));

#define XMYSQLND_INC_SESSION_STATISTIC_W_VALUE2(session_stats, statistic1, value1, statistic2, value2) \
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4389) \
	MYSQLND_INC_STATISTIC_W_VALUE2(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, \
									(enum_mysqlnd_collected_stats)(statistic1), (value1), \
									(enum_mysqlnd_collected_stats)(statistic2), (value2)); \
	MYSQLND_INC_STATISTIC_W_VALUE2(MYSQL_XDEVAPI_G(collect_statistics), (session_stats), \
									(enum_mysqlnd_collected_stats)(statistic1), (value1), \
									(enum_mysqlnd_collected_stats)(statistic2), (value2)); \
	MYSQLX_RESTORE_WARNINGS()

#define XMYSQLND_INC_SESSION_STATISTIC_W_VALUE3(session_stats, statistic1, value1, statistic2, value2, statistic3, value3) \
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4389) \
	MYSQLND_INC_STATISTIC_W_VALUE3(MYSQL_XDEVAPI_G(collect_statistics), xmysqlnd_global_stats, \
									(enum_mysqlnd_collected_stats)(statistic1), (value1), \
									(enum_mysqlnd_collected_stats)(statistic2), (value2), \
									(enum_mysqlnd_collected_stats)(statistic3), (value3)); \
	MYSQLND_INC_STATISTIC_W_VALUE3(MYSQL_XDEVAPI_G(collect_statistics), (session_stats), \
									(enum_mysqlnd_collected_stats)(statistic1), (value1), \
									(enum_mysqlnd_collected_stats)(statistic2), (value2), \
									(enum_mysqlnd_collected_stats)(statistic3), (value3)); \
	MYSQLX_RESTORE_WARNINGS()

#else

#define XMYSQLND_INC_GLOBAL_STATISTIC(enum_mysqlnd_collected_stats)(statistic)
#define XMYSQLND_DEC_CONN_STATISTIC(session_stats, statistic)
#define XMYSQLND_INC_GLOBAL_STATISTIC_W_VALUE2(statistic1, value1, statistic2, value2)
#define XMYSQLND_INC_CONN_STATISTIC(session_stats, statistic)
#define XMYSQLND_INC_CONN_STATISTIC_W_VALUE(session_stats, statistic, value)
#define XMYSQLND_INC_CONN_STATISTIC_W_VALUE2(session_stats, statistic1, value1, statistic2, value2)
#define XMYSQLND_INC_CONN_STATISTIC_W_VALUE3(session_stats, statistic1, value1, statistic2, value2, statistic3, value3)

#endif /* XMYSQLND_CORE_STATISTICS_DISABLED */

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_PRIV_H */
