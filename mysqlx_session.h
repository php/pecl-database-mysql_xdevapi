/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#ifndef MYSQLX_SESSION_H
#define MYSQLX_SESSION_H

#include "xmysqlnd/xmysqlnd_session.h"
#include "util/allocator.h"

namespace mysqlx {

namespace devapi {

extern zend_class_entry *mysqlx_session_class_entry;

struct Session_data : public util::custom_allocable
{
	drv::XMYSQLND_SESSION session;
	Session_data() = default;
	bool close_connection();
};

util::zvalue create_session();
util::zvalue create_session(drv::XMYSQLND_SESSION session);
void mysqlx_register_session_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_session_class(SHUTDOWN_FUNC_ARGS);

PHP_FUNCTION(mysql_xdevapi_getSession);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_SESSION_H */
