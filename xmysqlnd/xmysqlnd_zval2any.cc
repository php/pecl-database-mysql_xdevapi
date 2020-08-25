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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_zval2any.h"

#include "util/string_utils.h"
#include "util/value.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_datatypes.pb.h"

namespace mysqlx {

namespace drv {

using namespace Mysqlx::Datatypes;

namespace {

void zval2array(const util::zvalue& zv, Mysqlx::Datatypes::Any& any)
{
	assert(zv.is_array());
	any.set_type(Any_Type_ARRAY);
	for (const auto& value : zv.values()) {
		Mysqlx::Datatypes::Any* new_value = any.mutable_array()->add_value();
		zval2any(value, *new_value);
	}
}

void zval2object(const util::zvalue& obj_zv, Mysqlx::Datatypes::Any& any)
{
	assert(obj_zv.is_object());
	any.set_type(Any_Type_OBJECT);
	Mysqlx::Datatypes::Object* obj{ any.mutable_obj() };
	for (const auto& [property_name, property_value] : obj_zv) {
		Mysqlx::Datatypes::Object_ObjectField* field{ obj->add_fld() };

		assert(property_name.is_string());
		field->set_key(property_name.c_str(), property_name.length());

		Mysqlx::Datatypes::Any* field_value{ field->mutable_value() };
		zval2any(property_value, *field_value);
	}
}

void zval2str(const util::zvalue& zv, Mysqlx::Datatypes::Any& any)
{
	assert(zv.is_string());
	any.set_type(Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Scalar_Type_V_STRING);
	any.mutable_scalar()->mutable_v_string()->set_value(zv.c_str(), zv.length());
}

void zval2other(const util::zvalue& zv, Mysqlx::Datatypes::Any& any)
{
	util::zvalue other = zv.clone();
	convert_to_string(other.ptr());
	if (other.is_string()) {
		zval2str(other, any);
	}
}

} // anonymous namespace

void
zval2any(const util::zvalue& zv, Mysqlx::Datatypes::Any & any)
{
	DBG_ENTER("zval2any");
	switch (zv.type()) {
		case util::zvalue::Type::Undefined:
			DBG_INF("IS_UNDEF");
			[[fallthrough]];

		case util::zvalue::Type::Null:
			DBG_INF("IS_NULL");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_NULL);
			break;

		case util::zvalue::Type::False:
			DBG_INF("IS_FALSE");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_BOOL);
			any.mutable_scalar()->set_v_bool(false);
			break;

		case util::zvalue::Type::True:
			DBG_INF("IS_TRUE");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_BOOL);
			any.mutable_scalar()->set_v_bool(true);
			break;

		case util::zvalue::Type::Long:
			DBG_INF_FMT("IS_LONG=%lu", zv.to_zlong());
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_SINT);
			any.mutable_scalar()->set_v_signed_int(zv.to_zlong());
			break;

		case util::zvalue::Type::Double:
			DBG_INF_FMT("IS_DOUBLE=%f", zv.to_double());
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_DOUBLE);
			any.mutable_scalar()->set_v_double(zv.to_double());
			break;

		case util::zvalue::Type::String:
			DBG_INF_FMT("IS_STRING=%s", zv.c_str());
			zval2str(zv, any);
			break;

		case util::zvalue::Type::Array:
			DBG_INF("IS_ARRAY");
			zval2array(zv, any);
			break;

		case util::zvalue::Type::Object:
			DBG_INF("IS_OBJECT");
			zval2object(zv, any);
			break;

		default:
			zval2other(zv, any);
			break;
	}
	DBG_VOID_RETURN;
}

namespace {

template<typename SignedInt>
util::zvalue sint2zval(SignedInt sint)
{
#if SIZEOF_ZEND_LONG==4
	if ((sint < ZEND_LONG_MIN) || (ZEND_LONG_MAX < sint)) {
		return util::to_string(sint);
	}
#endif
	return sint;
}

template<typename UnsignedInt>
util::zvalue uint2zval(UnsignedInt uint)
{
#if SIZEOF_ZEND_LONG==4
	if (uint > ZEND_ULONG_MAX) {
		return util::to_string(uint);
	}
#endif
	return uint;
}

} // anonymous namespace

util::zvalue scalar2zval(const Mysqlx::Datatypes::Scalar& scalar)
{
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			return sint2zval(scalar.v_signed_int());

		case Scalar_Type_V_UINT:
			return uint2zval(scalar.v_unsigned_int());

		case Scalar_Type_V_NULL:
			return nullptr;

		case Scalar_Type_V_OCTETS: {
			const auto& octets = scalar.v_octets().value();
			return util::zvalue(octets.c_str(), octets.size());
		}

		case Scalar_Type_V_DOUBLE:
			return scalar.v_double();

		case Scalar_Type_V_FLOAT:
			return mysql_float_to_double(scalar.v_float(), -1); // Fixlength, without meta maybe bad results (see mysqlnd)

		case Scalar_Type_V_BOOL:
			return scalar.v_bool();

		case Scalar_Type_V_STRING: {
			const auto& str = scalar.v_string().value();
			return util::zvalue(str.c_str(), str.size());
		}

		default:
			php_error_docref(
				nullptr,
				E_WARNING,
				"Unknown new type %s (%d)",
				Mysqlx::Datatypes::Scalar::Type_Name(scalar.type()).c_str(),
				scalar.type());
			return util::zvalue();
	}
}

namespace {

util::zvalue array2zval(const Mysqlx::Datatypes::Array& array)
{
	util::zvalue result = util::zvalue::create_array(array.value_size());
	for (int i{0}; i < array.value_size(); ++i) {
		result.push_back(any2zval(array.value(i)));
	}
	return result;
}

util::zvalue object2zval(const Mysqlx::Datatypes::Object& obj)
{
	util::zvalue result = util::zvalue::create_object();
	const int fields_count{ obj.fld_size() };
	for (int i{0}; i < fields_count; ++i) {
		const auto& field{ obj.fld(i) };
		result.set_property(field.key(), any2zval(field.value()));
	}
	return result;
}

} // anonymous namespace

util::zvalue
any2zval(const Mysqlx::Datatypes::Any& any)
{
	switch (any.type()) {
		case Any_Type_SCALAR:
			return scalar2zval(any.scalar());

		case Any_Type_OBJECT:
			return object2zval(any.obj());

		case Any_Type_ARRAY:
			return array2zval(any.array());

		default:
			assert("UNHANDLED TYPE");
			php_error_docref(
				nullptr,
				E_WARNING,
				"Unknown type %s . Please report to the developers.",
				Any::Type_Name(any.type()).c_str());
			return util::zvalue();
	}
}

uint64_t scalar2uint(const Mysqlx::Datatypes::Scalar& scalar)
{
	DBG_ENTER("scalar2uint");
	uint64_t ret{0};
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			ret = static_cast<uint64_t>(scalar.v_signed_int());
			break;
		case Scalar_Type_V_UINT:
			ret = scalar.v_unsigned_int();
			break;
		case Scalar_Type_V_NULL:
			ret = 0;
			break;
		case Scalar_Type_V_OCTETS:
			ret = std::stoull(scalar.v_octets().value());
			break;
		case Scalar_Type_V_DOUBLE:
			ret = static_cast<uint64_t>(scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ret = static_cast<uint64_t>(mysql_float_to_double(scalar.v_float(), -1));
			break;
		case Scalar_Type_V_BOOL:
			ret = scalar.v_bool() ? 1 : 0;
			break;
		case Scalar_Type_V_STRING:
			ret = std::stoull(scalar.v_string().value());
			break;
		default:
			assert(!"unexpected type");
	}
	DBG_RETURN(ret);
}

util::string scalar2string(const Mysqlx::Datatypes::Scalar& scalar)
{
	util::zvalue value = scalar2zval(scalar);
	return value.serialize(false);
}

void scalar2log(const Mysqlx::Datatypes::Scalar& scalar)
{
	DBG_ENTER("scalar2log");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	const util::string& str = scalar2string(scalar);
	DBG_INF_FMT("value=[%*s]", str.c_str(), str.length());
	DBG_VOID_RETURN;
}

void repeated2log(
	const google::protobuf::RepeatedPtrField< Mysqlx::Datatypes::Scalar >& repeated)
{
	for (auto scalar : repeated) {
		scalar2log(scalar);
	}
}

void object2log(const Mysqlx::Datatypes::Object& obj)
{
	for (int i{0}; i < obj.fld_size(); ++i) {
		any2log(obj.fld(i).value());
	}
}

void array2log(const Mysqlx::Datatypes::Array& arr)
{
	for (int i{0}; i < arr.value_size(); ++i) {
		any2log(arr.value(i));
	}
}

void any2log(const Mysqlx::Datatypes::Any& any)
{
	DBG_ENTER("any2log");
	DBG_INF_FMT("type=%s", Any::Type_Name(any.type()).c_str());
	switch (any.type()) {
		case Any_Type_SCALAR:
			scalar2log(any.scalar());
			break;

		case Any_Type_OBJECT:
			object2log(any.obj());
			break;

		case Any_Type_ARRAY:
			array2log(any.array());
			break;

		default:
			DBG_INF_FMT("Unknown type %s . Please report to the developers.", Any::Type_Name(any.type()).c_str());
			assert(!"unhandled type");
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
