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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_any2expr.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_datatypes.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"

namespace mysqlx {

namespace drv {

void
object2expr(const Mysqlx::Datatypes::Object& src_obj, Mysqlx::Expr::Object* dest_obj)
{
	DBG_ENTER("object2expr");
	for (auto& src_field : src_obj.fld())
	{
		::Mysqlx::Expr::Object_ObjectField* dest_field = dest_obj->add_fld();
		dest_field->set_key(src_field.key());

		::Mysqlx::Expr::Expr* dest_value = dest_field->mutable_value();
		const ::Mysqlx::Datatypes::Any& src_value = src_field.value();
		any2expr(src_value, dest_value);
	}
	DBG_VOID_RETURN;
}

void
array2expr(const Mysqlx::Datatypes::Array & src_array, Mysqlx::Expr::Array* dest_array)
{
	DBG_ENTER("array2expr");
	for (auto& src_value : src_array.value())
	{
		::Mysqlx::Expr::Expr* dest_value = dest_array->add_value();
		any2expr(src_value, dest_value);
	}
	DBG_VOID_RETURN;
}

void
any2expr(const Mysqlx::Datatypes::Any& src, Mysqlx::Expr::Expr* dest)
{
	DBG_ENTER("any2expr");
	switch (src.type())
	{
		case Mysqlx::Datatypes::Any::SCALAR:
			dest->set_type(Mysqlx::Expr::Expr::LITERAL);
			dest->mutable_literal()->CopyFrom(src.scalar());
			break;

		case Mysqlx::Datatypes::Any::OBJECT:
			object2expr(src.obj(), dest->mutable_object());
			break;

		case Mysqlx::Datatypes::Any::ARRAY:
			array2expr(src.array(), dest->mutable_array());
			break;

		default:
			//TODO throw
			break;
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
