/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#ifndef MYSQLX_RESULTSET__DATA_ROW_H
#define MYSQLX_RESULTSET__DATA_ROW_H

#include "xmysqlnd/proto_gen/mysqlx_resultset.pb.h"

namespace mysqlx {

namespace devapi {

namespace msg {

void mysqlx_new_data_row(zval * return_value, const Mysqlx::Resultset::Row & message);
void mysqlx_register_data_row_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_data_row_class(SHUTDOWN_FUNC_ARGS);

} // namespace msg

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_RESULTSET__DATA_ROW_H */
