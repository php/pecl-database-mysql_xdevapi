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
#ifndef XMYSQLND_DRIVER_H
#define XMYSQLND_DRIVER_H

#include "php_mysql_xdevapi.h"

namespace mysqlx {

namespace drv {

PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_init(void);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_end(void);

PHP_MYSQL_XDEVAPI_API const char * xmysqlnd_get_client_info();
PHP_MYSQL_XDEVAPI_API unsigned int xmysqlnd_get_client_version();

typedef enum xmysqlnd_handler_func_status
{
	HND_PASS = PASS,
	HND_FAIL = FAIL,
	HND_PASS_RETURN_FAIL = 3,
	HND_AGAIN = 4,
	HND_AGAIN_ASYNC = 5,
	HND_DEFAULT_ACTION = 6,
} enum_hnd_func_status;

} // namespace drv

} // namespace mysqlx

#include "xmysqlnd_object_factory.h"

#endif /* XMYSQLND_DRIVER_H */
