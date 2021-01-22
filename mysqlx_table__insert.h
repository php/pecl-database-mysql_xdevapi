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
#ifndef MYSQLX_TABLE__INSERT_H
#define MYSQLX_TABLE__INSERT_H

namespace mysqlx {

namespace devapi {

util::zvalue create_table_insert(drv::xmysqlnd_table* schema, zval * columns,const int num_of_columns);
void mysqlx_register_table__insert_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_table__insert_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_TABLE__INSERT_H */
