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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysql_float_to_double.h>
}
#include "xmysqlnd.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_datatypes.pb.h"

namespace mysqlx {

namespace drv {

using namespace Mysqlx::Datatypes;

/* {{{ zval2any */
PHP_MYSQL_XDEVAPI_API enum_func_status
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
			any.mutable_scalar()->set_v_signed_int(Z_DVAL_P(zv));
			break;
		case IS_STRING:
			DBG_INF_FMT("IS_STRING=%s", Z_STRVAL_P(zv));
			any.set_type(Any_Type_SCALAR);
			any.mutable_scalar()->set_type(Scalar_Type_V_STRING);
			any.mutable_scalar()->mutable_v_string()->set_value(Z_STRVAL_P(zv), Z_STRLEN_P(zv));
			break;
		case IS_ARRAY: {
			DBG_INF("IS_ARRAY");
			zval * entry;
			any.set_type(Any_Type_ARRAY);
			ZEND_HASH_FOREACH_VAL(Z_ARR_P(zv), entry) {
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
			any.set_type(Any_Type_OBJECT);
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
/* }}} */


/* {{{ scalar2zval */
PHP_MYSQL_XDEVAPI_API enum_func_status
scalar2zval(const Mysqlx::Datatypes::Scalar & scalar, zval * zv)
{
	DBG_ENTER("any2zval");
	zval_ptr_dtor(zv);
	ZVAL_UNDEF(zv);
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
#if SIZEOF_ZEND_LONG==4
			if (UNEXPECTED(scalar.v_signed_int() >= ZEND_LONG_MAX)) {
				char tmp[22];
				snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, scalar.v_signed_int());
				ZVAL_STRING(zv, tmp);
			} else
#endif
			{
				ZVAL_LONG(zv, scalar.v_signed_int());
			}
			break;
		case Scalar_Type_V_UINT:
#if SIZEOF_ZEND_LONG==8
			if (scalar.v_unsigned_int() > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
			if (scalar.v_unsigned_int() > L64(2147483647)) {
#endif
				char tmp[22];
				snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, scalar.v_unsigned_int());
				ZVAL_STRING(zv, tmp);
			} else {
				ZVAL_LONG(zv, scalar.v_unsigned_int());
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
			php_error_docref(NULL, E_WARNING, "Unknown new type %s (%d)", Mysqlx::Datatypes::Scalar::Type_Name(scalar.type()).c_str(), scalar.type());
			DBG_RETURN(FAIL);
			;// assert
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ any2zval */
PHP_MYSQL_XDEVAPI_API enum_func_status
any2zval(const Mysqlx::Datatypes::Any & any, zval * zv)
{
	DBG_ENTER("any2zval");
	zval_ptr_dtor(zv);
	ZVAL_UNDEF(zv);
	switch (any.type()) {
		case Any_Type_SCALAR:
			scalar2zval(any.scalar(), zv);
#if 0
			switch (any.scalar().type()) {
				case Scalar_Type_V_SINT:
#if SIZEOF_ZEND_LONG==4
					if (UNEXPECTED(any.scalar().v_signed_int() >= ZEND_LONG_MAX)) {
						char tmp[22];
						snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, any.scalar().v_signed_int());
						ZVAL_STRING(zv, tmp);
					} else
#endif
					{
						ZVAL_LONG(zv, any.scalar().v_signed_int());
					}
					break;
				case Scalar_Type_V_UINT:
#if SIZEOF_ZEND_LONG==8
					if (any.scalar().v_unsigned_int() > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
					if (any.scalar().v_unsigned_int() > L64(2147483647)) {
#endif
						char tmp[22];
						snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, any.scalar().v_unsigned_int());
						ZVAL_STRING(zv, tmp);
					} else {
						ZVAL_LONG(zv, any.scalar().v_unsigned_int());
					}
					break;
				case Scalar_Type_V_NULL:
					ZVAL_NULL(zv);
					break;
				case Scalar_Type_V_OCTETS:
					ZVAL_STRINGL(zv, any.scalar().v_octets().value().c_str(), any.scalar().v_octets().value().size() - 1);
					break;
				case Scalar_Type_V_DOUBLE:
					ZVAL_DOUBLE(zv, any.scalar().v_double());
					break;
				case Scalar_Type_V_FLOAT:
					ZVAL_DOUBLE(zv, mysql_float_to_double(any.scalar().v_float(), -1)); // Fixlength, without meta maybe bad results (see mysqlnd)
					break;
				case Scalar_Type_V_BOOL:
					ZVAL_BOOL(zv, any.scalar().v_bool());
					break;
				case Scalar_Type_V_STRING:
					ZVAL_STRINGL(zv, any.scalar().v_string().value().c_str(), any.scalar().v_string().value().size());
					break;
				default:
					;// assert
			}
#endif
			break;
		case Any_Type_OBJECT: {
			zval properties;
			ZVAL_UNDEF(&properties);
			array_init_size(&properties, any.obj().fld_size());

			object_init(zv);
			for (unsigned int i = 0; i < any.obj().fld_size(); ++i) {
				zval entry;
				ZVAL_UNDEF(&entry);
				any2zval(any.obj().fld(i).value(), &entry);
				if (Z_REFCOUNTED(entry)) {
					Z_ADDREF(entry);
				}
				add_assoc_zval_ex(zv, any.obj().fld(i).key().c_str(), any.obj().fld(i).key().size(), &entry);
				zend_hash_next_index_insert(Z_ARRVAL_P(zv), &entry);
				zval_ptr_dtor(zv);
			}

			if (!zend_standard_class_def->default_properties_count && !zend_standard_class_def->__set) {
				Z_OBJ_P(zv)->properties = Z_ARR(properties);
			} else {
				zend_merge_properties(zv, Z_ARRVAL(properties));
			}
			break;
		}
		case Any_Type_ARRAY:
			array_init_size(zv, any.array().value_size());
			for (unsigned int i = 0; i < any.array().value_size(); ++i) {
				zval entry;
				ZVAL_UNDEF(&entry);
				any2zval(any.array().value(i), &entry);
				zend_hash_next_index_insert(Z_ARRVAL_P(zv), &entry);
			}
			break;
#ifndef PHP_DEBUG
		default:
			php_error_docref(NULL, E_WARNING, "Unknown type %s . Please report to the developers.", Any::Type_Name(any.type()).c_str());
			DBG_RETURN(FAIL);
#else
			DBG_INF_FMT("UNHANDLED TYPE");
			exit(0);
#endif
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ scalar2uint */
PHP_MYSQL_XDEVAPI_API uint64_t
scalar2uint(const Mysqlx::Datatypes::Scalar & scalar)
{
	uint64_t ret = 0;
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
			ret = ZEND_STRTOL(scalar.v_octets().value().c_str(), NULL, 10);
			break;
		case Scalar_Type_V_DOUBLE:
			ret = scalar.v_double();
			break;
		case Scalar_Type_V_FLOAT:
			ret = mysql_float_to_double(scalar.v_float(), -1);
			break;
		case Scalar_Type_V_BOOL:
			ret = scalar.v_bool();
			break;
		case Scalar_Type_V_STRING:
			ret = ZEND_STRTOL(scalar.v_string().value().c_str(), NULL, 10);
			break;
		default:
			;// assert
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ scalar2uint */
PHP_MYSQL_XDEVAPI_API int64_t
scalar2sint(const Mysqlx::Datatypes::Scalar & scalar)
{
	int64_t ret = 0;
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
			ret = ZEND_STRTOL(scalar.v_octets().value().c_str(), NULL, 10);
			break;
		case Scalar_Type_V_DOUBLE:
			ret = scalar.v_double();
			break;
		case Scalar_Type_V_FLOAT:
			ret = mysql_float_to_double(scalar.v_float(), -1);
			break;
		case Scalar_Type_V_BOOL:
			ret = scalar.v_bool();
			break;
		case Scalar_Type_V_STRING:
			ret = ZEND_STRTOL(scalar.v_string().value().c_str(), NULL, 10);
			break;
		default:
			;// assert
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ scalar2string */
PHP_MYSQL_XDEVAPI_API MYSQLND_STRING
scalar2string(const Mysqlx::Datatypes::Scalar & scalar)
{
	MYSQLND_STRING ret = {NULL, 0};
	DBG_ENTER("scalar2string");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
			ret.l = mnd_sprintf(&ret.s, 0, MYSQLND_LLU_SPEC, scalar.v_signed_int());
			break;
		case Scalar_Type_V_UINT:
			ret.l = mnd_sprintf(&ret.s, 0, MYSQLND_LLU_SPEC, scalar.v_unsigned_int());
			break;
		case Scalar_Type_V_NULL:
			break;
		case Scalar_Type_V_OCTETS: {
			const MYSQLND_CSTRING from = { scalar.v_octets().value().c_str(), scalar.v_octets().value().size() };
			ret = mnd_dup_cstring(from, FALSE /*persistent*/);
			break;
		}
		case Scalar_Type_V_DOUBLE:
			ret.l = mnd_sprintf(&ret.s, 0, "%f", scalar.v_double());
			break;
		case Scalar_Type_V_FLOAT:
			ret.l = mnd_sprintf(&ret.s, 0, "%f", mysql_float_to_double(scalar.v_float(), -1));
			break;
		case Scalar_Type_V_BOOL:{
			const MYSQLND_CSTRING from = { scalar.v_bool()? "TRUE":"FALSE", scalar.v_bool()? sizeof("TRUE")-1: sizeof("FALSE")-1 };
			ret = mnd_dup_cstring(from, FALSE /*persistent*/);
			break;
		}
		case Scalar_Type_V_STRING:{
			const MYSQLND_CSTRING from = { scalar.v_string().value().c_str(), scalar.v_string().value().size() };
			ret = mnd_dup_cstring(from, FALSE /*persistent*/);
			break;
		}
		default:
			;// assert
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ scalar2log */
PHP_MYSQL_XDEVAPI_API void
scalar2log(const Mysqlx::Datatypes::Scalar & scalar)
{
	DBG_ENTER("scalar2log");
	DBG_INF_FMT("subtype=%s", Scalar::Type_Name(scalar.type()).c_str());
	switch (scalar.type()) {
		case Scalar_Type_V_SINT:
#if SIZEOF_ZEND_LONG==4
			if (UNEXPECTED(scalar.v_signed_int() >= ZEND_LONG_MAX)) {
				char tmp[22];
				snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, scalar.v_unsigned_int());
				DBG_INF_FMT("value=%s", tmp);
			} else
#endif
			{
				DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, scalar.v_signed_int());
			}
			break;
		case Scalar_Type_V_UINT:
#if SIZEOF_ZEND_LONG==8
			if (scalar.v_unsigned_int() > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
			if (scalar.v_unsigned_int() > L64(2147483647)) {
#endif
				char tmp[22];
				snprintf(tmp, sizeof(tmp), MYSQLND_LLU_SPEC, scalar.v_unsigned_int());
				DBG_INF_FMT("value=%s", tmp);
			} else {
				DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, scalar.v_unsigned_int());
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
/* }}} */


/* {{{ any2log */
PHP_MYSQL_XDEVAPI_API void
any2log(const Mysqlx::Datatypes::Any & any)
{
	DBG_ENTER("any2log");
	DBG_INF_FMT("type=%s", Any::Type_Name(any.type()).c_str());
	switch (any.type()) {
		case Any_Type_SCALAR:
			scalar2log(any.scalar());
			break;
		case Any_Type_OBJECT: {
			for (unsigned int i = 0; i < any.obj().fld_size(); ++i) {
				any2log(any.obj().fld(i).value());
			}
			break;
		}
		case Any_Type_ARRAY:
			for (unsigned int i = 0; i < any.array().value_size(); ++i) {
				any2log(any.array().value(i));
			}
			break;
#ifndef PHP_DEBUG
		default:
			DBG_INF_FMT("Unknown type %s . Please report to the developers.", Any::Type_Name(any.type()).c_str());
			DBG_VOID_RETURN;
#else
			DBG_INF_FMT("UNHANDLED TYPE");
			exit(0);
#endif
	}
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
