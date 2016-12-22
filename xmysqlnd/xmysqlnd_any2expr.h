/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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

namespace xmysqlnd
{

PHP_MYSQL_XDEVAPI_API void object2expr(const Mysqlx::Datatypes::Object& src_obj, Mysqlx::Expr::Object* dest_obj);
PHP_MYSQL_XDEVAPI_API void array2expr(const Mysqlx::Datatypes::Array& src_array, Mysqlx::Expr::Array* dest_array);
PHP_MYSQL_XDEVAPI_API void any2expr(const Mysqlx::Datatypes::Any& src, Mysqlx::Expr::Expr* dest);

} // namespace xmysqlnd

#endif	/* XMYSQLND_ANY2EXPR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
