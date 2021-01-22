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
#ifndef MYSQLX_CRUD_OPERATION_LIMITABLE_H
#define MYSQLX_CRUD_OPERATION_LIMITABLE_H

namespace mysqlx {

namespace devapi {

extern zend_class_entry * mysqlx_crud_operation_limitable_interface_entry;

void mysqlx_register_crud_operation_limitable_interface(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_crud_operation_limitable_interface(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_CRUD_OPERATION_LIMITABLE_H */
