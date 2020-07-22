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

void zval2object(zval* zv, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Any_Type_OBJECT);
	Mysqlx::Datatypes::Object* obj{ any.mutable_obj() };
	HashTable* properties{ zend_std_get_properties(zv) };
	zend_string* property_name{nullptr};
	zval* property_value{nullptr};
	MYSQLX_HASH_FOREACH_STR_KEY_VAL(properties, property_name, property_value) {
		if (property_name && property_value) {
			Mysqlx::Datatypes::Object_ObjectField* field{ obj->add_fld() };
			field->set_key(ZSTR_VAL(property_name), ZSTR_LEN(property_name));
			Mysqlx::Datatypes::Any* field_value{ field->mutable_value() };
			zval2any(property_value, *field_value);
		}
	} ZEND_HASH_FOREACH_END();
}

} // anonymous namespace

enum_func_status
zval2any(const zval * const zv, Mysqlx::Datatypes::Any & any)
{
	DBG_ENTER("zval2any");
	switch (Z_TYPE_P(zv)) {
		case IS_UNDEF:
			DBG_INF("IS_UNDEF");
			/* fallthrough */
		case IS_NULL:
			DBG_INF("IS_NULL");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_NULL);
			break;
		case IS_FALSE:
			DBG_INF("IS_FALSE");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_BOOL);
			any.mutable_scalar()->set_v_bool(false);
			break;
		case IS_TRUE:
			DBG_INF("IS_TRUE");
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_BOOL);
			any.mutable_scalar()->set_v_bool(true);
			break;
		case IS_LONG:
			DBG_INF_FMT("IS_LONG=%lu", Z_LVAL_P(zv));
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_SINT);
			any.mutable_scalar()->set_v_signed_int(Z_LVAL_P(zv));
			break;
		case IS_DOUBLE:
			DBG_INF_FMT("IS_DOUBLE=%f", Z_DVAL_P(zv));
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_DOUBLE);
			any.mutable_scalar()->set_v_double(Z_DVAL_P(zv));
			break;
		case IS_STRING:
			DBG_INF_FMT("IS_STRING=%s", Z_STRVAL_P(zv));
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_STRING);
			any.mutable_scalar()->mutable_v_string()->set_value(Z_STRVAL_P(zv), Z_STRLEN_P(zv));
			break;
		case IS_ARRAY: {
			DBG_INF("IS_ARRAY");
			zval* entry{nullptr};
			any.set_type(Any_Type_ARRAY);
			MYSQLX_HASH_FOREACH_VAL(Z_ARR_P(zv), entry) {
				DBG_INF("ENTRY");
				Mysqlx::Datatypes::Any entry_any;
				Mysqlx::Datatypes::Any * new_value = any.mutable_array()->add_value();
				ZVAL_DEREF(entry);
				zval2any(entry, entry_any);
				new_value->CopyFrom(entry_any); /* maybe Swap() as the internal value will be empty anyway */
			} ZEND_HASH_FOREACH_END();
			break;
		}
		case IS_OBJECT: {
			DBG_INF("IS_OBJECT");
			zval2object(const_cast<zval*>(zv), any);
			break;
		}
		default:
			zval to_str;
			ZVAL_COPY(&to_str, zv);
			convert_to_string(&to_str);
			break;
	}
	DBG_RETURN(PASS);
}

enum_func_status
zval2any(const util::zvalue& zv, Mysqlx::Datatypes::Any& any)
{
	return zval2any(zv.ptr(), any);
}

enum_func_status
scalar2zval(const Mysqlx::Datatypes::Scalar & scalar, zval * zv)
{
	DBG_ENTER("scalar2zval");
	zval_ptr_dtor(zv);
	ZVAL_UNDEF(zv);
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
#if SIZEOF_ZEND_LONG==4
			if (UNEXPECTED(scalar.v_signed_int() >= ZEND_LONG_MAX)) {
				char tmp[22];
				snprintf(tmp, sizeof(tmp), "%s", util::to_string(scalar.v_signed_int()).c_str());
				ZVAL_STRING(zv, tmp);
			} else
#endif
			{
				ZVAL_LONG(zv, static_cast<zend_long>(scalar.v_signed_int()));
			}
			break;
		case Scalar_Type_V_UINT:
#if SIZEOF_ZEND_LONG==8
			if (scalar.v_unsigned_int() > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
			if (scalar.v_unsigned_int() > L64(2147483647)) {
#endif
				char tmp[22];
				snprintf(tmp, sizeof(tmp), "%s", util::to_string(scalar.v_unsigned_int()).c_str());
				ZVAL_STRING(zv, tmp);
			} else {
				ZVAL_LONG(zv, static_cast<zend_long>(scalar.v_unsigned_int()));
			}
			break;
		case Scalar_Type_V_NULL:
			ZVAL_NULL(zv);
			break;
		case Scalar_Type_V_OCTETS:
			ZVAL_STRINGL(zv, scalar.v_octets().value().c_str(), scalar.v_octets().value().size() - 1);
			break;
		case Scalar_Type_V_DOUBLE:
			ZVAL_DOUBLE(zv, scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ZVAL_DOUBLE(zv, mysql_float_to_double(scalar.v_float(), -1)); // Fixlength, without meta maybe bad results (see mysqlnd)
			break;
		case Scalar_Type_V_BOOL:
			ZVAL_BOOL(zv, scalar.v_bool());
			break;
		case Scalar_Type_V_STRING:
			ZVAL_STRINGL(zv, scalar.v_string().value().c_str(), scalar.v_string().value().size());
			break;
		default:
			php_error_docref(nullptr, E_WARNING, "Unknown new type %s (%d)", Mysqlx::Datatypes::Scalar::Type_Name(scalar.type()).c_str(), scalar.type());
			DBG_RETURN(FAIL);
			;// assert
	}
	DBG_RETURN(PASS);
}

enum_func_status
any2zval(const Mysqlx::Datatypes::Any & any, zval * zv)
{
	DBG_ENTER("any2zval");
	zval_ptr_dtor(zv);
	ZVAL_UNDEF(zv);
	switch (any.type()) {
		case Any_Type_SCALAR:
			scalar2zval(any.scalar(), zv);
			break;
		case Any_Type_OBJECT: {
			zval properties;
			ZVAL_UNDEF(&properties);
			const int fields_count{ any.obj().fld_size() };
			array_init_size(&properties, fields_count);

			for (int i{0}; i < fields_count; ++i) {
				zval entry;
				ZVAL_UNDEF(&entry);
				const auto& field{ any.obj().fld(i) };
				any2zval(field.value(), &entry);
				if (Z_REFCOUNTED(entry)) {
					Z_ADDREF(entry);
				}
				add_assoc_zval_ex(&properties, field.key().c_str(), field.key().size(), &entry);
				zend_hash_next_index_insert(Z_ARRVAL(properties), &entry);
			}

			object_init(zv);
			zend_merge_properties(zv, Z_ARRVAL(properties));
			zval_ptr_dtor(&properties);
			break;
		}
		case Any_Type_ARRAY:
			array_init_size(zv, any.array().value_size());
			for (int i{0}; i < any.array().value_size(); ++i) {
				zval entry;
				ZVAL_UNDEF(&entry);
				any2zval(any.array().value(i), &entry);
				zend_hash_next_index_insert(Z_ARRVAL_P(zv), &entry);
			}
			break;
		default:
#ifndef PHP_DEBUG
			php_error_docref(nullptr, E_WARNING, "Unknown type %s . Please report to the developers.", Any::Type_Name(any.type()).c_str());
			DBG_RETURN(FAIL);
#else
			DBG_INF_FMT("UNHANDLED TYPE");
			exit(0);
#endif
	}
	DBG_RETURN(PASS);
}

enum_func_status any2zval(const Mysqlx::Datatypes::Any& any, util::zvalue& zv)
{
	return any2zval(any, zv.ptr());
}

util::zvalue any2zval(const Mysqlx::Datatypes::Any& any)
{
	util::zvalue value;
	any2zval(any, value.ptr());
	return value;
}

uint64_t
scalar2uint(const Mysqlx::Datatypes::Scalar & scalar)
{
	uint64_t ret{0};
	DBG_ENTER("scalar2uint");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			ret = scalar.v_signed_int();
			break;
		case Scalar_Type_V_UINT:
			ret = scalar.v_unsigned_int();
			break;
		case Scalar_Type_V_NULL:
			ret = 0;
			break;
		case Scalar_Type_V_OCTETS:
			ret = ZEND_STRTOL(scalar.v_octets().value().c_str(), nullptr, 10);
			break;
		case Scalar_Type_V_DOUBLE:
			ret = static_cast<uint64_t>(scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ret = static_cast<uint64_t>(mysql_float_to_double(scalar.v_float(), -1));
			break;
		case Scalar_Type_V_BOOL:
			ret = scalar.v_bool();
			break;
		case Scalar_Type_V_STRING:
			ret = ZEND_STRTOL(scalar.v_string().value().c_str(), nullptr, 10);
			break;
		default:
			;// assert
	}
	DBG_RETURN(ret);
}

int64_t
scalar2sint(const Mysqlx::Datatypes::Scalar & scalar)
{
	int64_t ret{0};
	DBG_ENTER("scalar2uint");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			ret = scalar.v_signed_int();
			break;
		case Scalar_Type_V_UINT:
			ret = scalar.v_unsigned_int();
			break;
		case Scalar_Type_V_NULL:
			ret = 0;
			break;
		case Scalar_Type_V_OCTETS:
			ret = ZEND_STRTOL(scalar.v_octets().value().c_str(), nullptr, 10);
			break;
		case Scalar_Type_V_DOUBLE:
			ret = static_cast<int64_t>(scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ret = static_cast<int64_t>(mysql_float_to_double(scalar.v_float(), -1));
			break;
		case Scalar_Type_V_BOOL:
			ret = scalar.v_bool();
			break;
		case Scalar_Type_V_STRING:
			ret = ZEND_STRTOL(scalar.v_string().value().c_str(), nullptr, 10);
			break;
		default:
			;// assert
	}
	DBG_RETURN(ret);
}

util::string
scalar2string(const Mysqlx::Datatypes::Scalar & scalar)
{
	util::string ret;
	DBG_ENTER("scalar2string");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			ret = util::to_string(scalar.v_signed_int());
			break;
		case Scalar_Type_V_UINT:
			ret = util::to_string(scalar.v_unsigned_int());
			break;
		case Scalar_Type_V_NULL:
			break;
		case Scalar_Type_V_OCTETS:
			ret.assign(scalar.v_octets().value().c_str(), scalar.v_octets().value().size());
			break;
		case Scalar_Type_V_DOUBLE:
			ret = util::to_string(scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ret = util::to_string(scalar.v_float());
			break;
		case Scalar_Type_V_BOOL:{
			ret = scalar.v_bool() ? "TRUE" : "FALSE";
			break;
		}
		case Scalar_Type_V_STRING:{
			ret.assign(scalar.v_string().value().c_str(), scalar.v_string().value().size());
			break;
		}
		default:
			;// assert
	}
	DBG_RETURN(ret);
}

void
scalar2log(const Mysqlx::Datatypes::Scalar & scalar)
{
	DBG_ENTER("scalar2log");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
#if SIZEOF_ZEND_LONG==4
			if (UNEXPECTED(scalar.v_signed_int() >= ZEND_LONG_MAX)) {
				char tmp[22];
				snprintf(tmp, sizeof(tmp), "%s", util::to_string(scalar.v_unsigned_int()).c_str());
				DBG_INF_FMT("value=%s", tmp);
			} else
#endif
			{
				DBG_INF_FMT("value=" MYSQLX_LLU_SPEC, scalar.v_signed_int());
			}
			break;
		case Scalar_Type_V_UINT:
#if SIZEOF_ZEND_LONG==8
			if (scalar.v_unsigned_int() > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
			if (scalar.v_unsigned_int() > L64(2147483647)) {
#endif
				char tmp[22];
				snprintf(tmp, sizeof(tmp), "%s", util::to_string(scalar.v_unsigned_int()).c_str());
				DBG_INF_FMT("value=%s", tmp);
			} else {
				DBG_INF_FMT("value=" MYSQLX_LLU_SPEC, scalar.v_unsigned_int());
			}
			break;
		case Scalar_Type_V_NULL:
			break;
		case Scalar_Type_V_OCTETS:
			DBG_INF_FMT("value=[%*s]", scalar.v_octets().value().size(), scalar.v_octets().value().c_str());
			break;
		case Scalar_Type_V_DOUBLE:
			DBG_INF_FMT("value=%f", scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			DBG_INF_FMT("value=%f", mysql_float_to_double(scalar.v_float(), -1));
			break;
		case Scalar_Type_V_BOOL:
			DBG_INF_FMT("value=%s", scalar.v_bool()? "TRUE":"FALSE");
			break;
		case Scalar_Type_V_STRING:
			DBG_INF_FMT("value=[%*s]", scalar.v_string().value().size(), scalar.v_string().value().c_str());
			break;
		default:
			;// assert
	}
	DBG_VOID_RETURN;
}

void
any2log(const Mysqlx::Datatypes::Any & any)
{
	DBG_ENTER("any2log");
	DBG_INF_FMT("type=%s", Any::Type_Name(any.type()).c_str());
	switch (any.type()) {
		case Any_Type_SCALAR:
			scalar2log(any.scalar());
			break;
		case Any_Type_OBJECT: {
			for (int i{0}; i < any.obj().fld_size(); ++i) {
				any2log(any.obj().fld(i).value());
			}
			break;
		}
		case Any_Type_ARRAY:
			for (int i{0}; i < any.array().value_size(); ++i) {
				any2log(any.array().value(i));
			}
			break;
		default:
#ifndef PHP_DEBUG
			DBG_INF_FMT("Unknown type %s . Please report to the developers.", Any::Type_Name(any.type()).c_str());
			DBG_VOID_RETURN;
#else
			DBG_INF_FMT("UNHANDLED TYPE");
			exit(0);
#endif
	}
	DBG_VOID_RETURN;
}

void repeated2log(
	const google::protobuf::RepeatedPtrField< Mysqlx::Datatypes::Scalar >& repeated)
{
	for (auto scalar : repeated) {
		scalar2log(scalar);
	}
}

} // namespace drv

} // namespace mysqlx
