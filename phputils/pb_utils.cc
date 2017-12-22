/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
#include "pb_utils.h"
#include "xmysqlnd/proto_gen/mysqlx_sql.pb.h"

namespace mysqlx {

namespace phputils {

namespace pb {

using namespace Mysqlx::Datatypes;

void to_any(
	std::nullptr_t /*nil*/,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_NULL);
}

void to_any(
	bool value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_BOOL);
	any.mutable_scalar()->set_v_bool(value);
}

void to_any(
	int value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_SINT);
	any.mutable_scalar()->set_v_signed_int(value);
}

void to_any(
	long value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_SINT);
	any.mutable_scalar()->set_v_signed_int(value);
}

void to_any(
	long long value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_SINT);
	any.mutable_scalar()->set_v_signed_int(value);
}

void to_any(
	unsigned int value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_UINT);
	any.mutable_scalar()->set_v_unsigned_int(value);
}

void to_any(
	unsigned long value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_UINT);
	any.mutable_scalar()->set_v_unsigned_int(value);
}

void to_any(
	unsigned long long value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_UINT);
	any.mutable_scalar()->set_v_unsigned_int(value);
}

void to_any(
	float value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_FLOAT);
	any.mutable_scalar()->set_v_float(value);
}

void to_any(
	double value,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_DOUBLE);
	any.mutable_scalar()->set_v_double(value);
}

void to_any(
	const char* str,
	const size_t length,
	Any& any)
{
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_STRING);
	any.mutable_scalar()->mutable_v_string()->set_value(str, length);
}

void to_any(
	const std::string& value,
	Any& any)
{
	to_any(value.c_str(), value.length(), any);
}

void to_any(
	const phputils::string& value,
	Any& any)
{
	to_any(value.c_str(), value.length(), any);
}

void to_any(
	const phputils::string_view& value,
	Any& any)
{
	to_any(value.c_str(), value.length(), any);
}

void to_any(
	Object* value,
	Any& any)
{
	any.set_type(Any_Type_OBJECT);
	any.set_allocated_obj(value);
}

void to_any(
	Array* value,
	Any& any)
{
	any.set_type(Any_Type_ARRAY);
	any.set_allocated_array(value);
}

// -----------------------------------------------------------------------------

Object* phputils::pb::add_object_arg(Mysqlx::Sql::StmtExecute& stmt_message)
{
	Any* stmt_arg{stmt_message.add_args()};
	stmt_arg->set_type(Any_Type_OBJECT);
	return stmt_arg->mutable_obj();
}

Array* phputils::pb::add_array_arg(Mysqlx::Sql::StmtExecute& stmt_message)
{
	Any* stmt_arg{stmt_message.add_args()};
	stmt_arg->set_type(Any_Type_ARRAY);
	return stmt_arg->mutable_array();
}

} // namespace pb

} // namespace phputils

} // namespace mysqlx
