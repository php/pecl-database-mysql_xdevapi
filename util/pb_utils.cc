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
#include "pb_utils.h"
#include "exceptions.h"
#include "mysqlnd_api.h"
#include "protobuf_api.h"
#include "xmysqlnd/xmysqlnd_zval2any.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_sql.pb.h"
#include "util/value.h"

namespace mysqlx {

namespace util {

namespace pb {

using namespace Mysqlx::Datatypes;

bool read_variant_64(::google::protobuf::io::CodedInputStream& input_stream, uint64_t* value)
{
	/*
		fix for protobuf 3.0.0, there is problem with routine:

		bool CodedInputStream::ReadVarint64(uint64* value)

		it stores random number in 'value' in case it cannot read next value (e.g. stream is
		at the end)
		it returns 'false' as expected, but in many places we also expected it will NOT
		touch the output 'value'


		e.g.
		uint64_t val = INIT_VALUE;
		if (!input_stream.ReadVarint64(&val)) break;

		in case it returns false in 'val' there may be random number != INIT_VALUE, while we
		would expect it will be not touched, and still INIT_VALUE

		it may cause severe problems, besides valgrind treats such values as uninitialized
		hence this helper routine to wrap CodedInputStream::ReadVarint64

		btw behaviour met on both Linux/Win x64, doesn't occur on Win32
	*/
	uint64_t tmp;
	if (input_stream.ReadVarint64(&tmp)) {
		*value = tmp;
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------

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
	const util::string& value,
	Any& any)
{
	to_any(value.c_str(), value.length(), any);
}

void to_any(
	const util::string_view& value,
	Any& any)
{
	to_any(value.data(), value.length(), any);
}

void to_any(
	const util::zvalue& zv,
	Any& any)
{
	drv::zval2any(zv.c_ptr(), any);
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

Object* add_object_arg(Mysqlx::Sql::StmtExecute& stmt_message)
{
	Any* stmt_arg{stmt_message.add_args()};
	stmt_arg->set_type(Any_Type_OBJECT);
	return stmt_arg->mutable_obj();
}

Array* add_array_arg(Mysqlx::Sql::StmtExecute& stmt_message)
{
	Any* stmt_arg{stmt_message.add_args()};
	stmt_arg->set_type(Any_Type_ARRAY);
	return stmt_arg->mutable_array();
}

// -----------------------------------------------------------------------------

void verify_limit_offset(const Mysqlx::Crud::Find& message)
{
	if (!message.has_limit()) return;

	const Mysqlx::Crud::Limit& limit{ message.limit() };
	if (!limit.has_row_count() && limit.has_offset()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::offset_without_limit_not_allowed);
	}
}

} // namespace pb

} // namespace util

} // namespace mysqlx
