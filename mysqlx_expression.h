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
#ifndef MYSQLX_EXPRESSION_H
#define MYSQLX_EXPRESSION_H

#include "util/strings.h"

namespace mysqlx {

namespace devapi {

zend_bool is_a_mysqlx_expression(const zval * const value);
zval* get_mysqlx_expression(const zval * const object_zv);
void mysqlx_new_expression(zval* return_value, const util::string_view& expression);
void mysqlx_register_expression_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_expression_class(SHUTDOWN_FUNC_ARGS);

PHP_FUNCTION(mysql_xdevapi__expression);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_EXPRESSION_H */
