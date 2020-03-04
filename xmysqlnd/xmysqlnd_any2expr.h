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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_ANY2EXPR_H
#define XMYSQLND_ANY2EXPR_H

namespace Mysqlx {
namespace Datatypes {

	class Any;
	class Array;
	class Object;

} // namespace Datatypes

namespace Expr {

	class Array;
	class Expr;
	class Object;

} // namespace Expr

} // namespace Mysqlx

namespace mysqlx {

namespace drv {

void object2expr(const Mysqlx::Datatypes::Object& src_obj, Mysqlx::Expr::Object* dest_obj);
void array2expr(const Mysqlx::Datatypes::Array& src_array, Mysqlx::Expr::Array* dest_array);
void any2expr(const Mysqlx::Datatypes::Any& src, Mysqlx::Expr::Expr* dest);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_ANY2EXPR_H */
