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
#ifndef XMYSQLND_H
#define XMYSQLND_H

#define PHP_XMYSQLND_VERSION "mysql_xdevapi 8.0.22"
#define XMYSQLND_VERSION_ID 80022

#if PHP_DEBUG
#define MYSQLND_DBG_ENABLED 1
#else
#define MYSQLND_DBG_ENABLED 0
#endif

extern "C" {
#ifdef ZTS
#include "TSRM.h"
#endif
#include <ext/mysqlnd/mysqlnd_portability.h>
}

#include "php_mysql_xdevapi.h"
#include "xmysqlnd_enum_n_def.h"

namespace mysqlx {

namespace drv {

/* Library related */
PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_init(void);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_end(void);


PHP_MYSQL_XDEVAPI_API const char *	xmysqlnd_get_client_info();
PHP_MYSQL_XDEVAPI_API unsigned int	xmysqlnd_get_client_version();

#define XMYSQLND_METHOD(class, method) 			xmysqlnd_##class##_##method##_pub

PHP_MYSQL_XDEVAPI_API extern MYSQLND_STATS *xmysqlnd_global_stats;

} // namespace drv

} // namespace mysqlx

#endif	/* XMYSQLND_H */
