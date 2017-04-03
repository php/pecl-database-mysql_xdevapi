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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_table_create.h"
#include "xmysqlnd/xmysqlnd_ddl_table_defs.h"
#include "xmysqlnd/xmysqlnd_utils.h"

#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_table_create_column_def_base.h"
#include "mysqlx_table_create_column_def.h"
#include "mysqlx_object.h"

#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"
#include "phputils/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

/*
	ColumnDef
	   ::= 'new ColumnDef(' StringLiteral ', ' ColumnType (', ' Number)? ')'
		   ( '.notNull()' )?
		   ( '.setDefault(' expr ')' )?
		   ( '.defaultCurrentTimestamp()' )?
		   ( '.autoIncrement()' )?
		   ( '.uniqueIndex()' )?
		   ( '.primaryKey() ')?
		   ( '.comment(' StringLiteral ')' )?
		   ( '.foreignKey(' StringLiteral (',' StringLiteral)+ ')' )?
		   ( '.unsigned()' )?
		   ( '.decimals(' Number ')' )?
		   ( '.charset(' StringLiteral ')' )?
		   ( '.collation(' StringLiteral ')' )?
		   ( '.binary()' )?
		   ( '.values(' StringLiteral (', ' StringLiteral)* ')' )?

	var column_def = new ColumnDef(columnName, ColumnType)
	or
	var column_def = new ColumnDef(columnName, ColumnType, length)

	.notNull()
	.setDefault(value)
	.defaultCurrentTimestamp()
	.autoIncrement()
	.uniqueIndex()
	.primaryKey()
	.comment(string)
	.foreignKey(tableName, foreignColumnName, foreignColumnName, ...)
	.unsigned()
	.decimals(int)
	.charset(charset_name)
	.collation(collation_name)
	.binary()
	.values(val, val, ...) // for enum and set types



	following are implemented in column_base_def:
	.notNull()
	.uniqueIndex()
	.primaryKey()
	.comment(string)

*/
zend_class_entry* column_def_class_entry = nullptr;


ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, type, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, length, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_set_default, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, default_value_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_default_current_timestamp, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_auto_increment, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_foreign_key, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, table_name, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_unsigned, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_decimals, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, decimals_size, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_charset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, charset, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_collation, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collation, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_binary, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_values, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, values)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_column_def::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, __construct)
{
	DBG_ENTER("mysqlx_column_def::__construct");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr name;
	phputils::string_ptr type;
	long length = Column_def::Default_length;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oss|l",
		&object_zv, column_def_class_entry,
		&name.str, &name.len,
		&type.str, &type.len,
		&length))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.init(name, type, length);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ to_value_str */
phputils::string to_value_str(zval* default_value_zv)
{
	switch (Z_TYPE_P(default_value_zv)) {
		case IS_FALSE:
		case IS_TRUE:
		case IS_NULL:
		case IS_LONG:
		case IS_DOUBLE:
			return phputils::to_string(default_value_zv);

		case IS_STRING:
			return create_table::quote_text(phputils::to_string(default_value_zv));

		default:
			throw phputils::xdevapi_exception(
				phputils::xdevapi_exception::Code::unsupported_default_value_type);
	}
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::setDefault() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, setDefault)
{
	DBG_ENTER("mysqlx_column_def::setDefault");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	zval* default_value_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oz",
		&object_zv, column_def_class_entry,
		&default_value_zv))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_default_value(to_value_str(default_value_zv));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::defaultCurrentTimestamp() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, defaultCurrentTimestamp)
{
	DBG_ENTER("mysqlx_column_def::defaultCurrentTimestamp");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	const char* Current_timestamp_value = "CURRENT_TIMESTAMP";
	data_object.column_def.set_default_value(Current_timestamp_value);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::autoIncrement() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, autoIncrement)
{
	DBG_ENTER("mysqlx_column_def::autoIncrement");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_auto_increment();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::foreignKey(string table, string[] fields) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, foreignKey)
{
	DBG_ENTER("mysqlx_column_def::foreignKey");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr table_name;
	zval* fields = nullptr;
	int fields_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os+",
		&object_zv, column_def_class_entry,
		&table_name.str, &table_name.len,
		&fields, &fields_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_foreign_key(
		table_name,
		phputils::to_strings(fields, fields_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::unsigned() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, unsigned)
{
	DBG_ENTER("mysqlx_column_def::unsigned");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_unsigned();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::decimals(long initialAutoIncrement) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, decimals)
{
	DBG_ENTER("mysqlx_column_def::decimals");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	long decimals_size = 0;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Ol",
		&object_zv, column_def_class_entry,
		&decimals_size))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_decimals(decimals_size);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::charset(string charset) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, charset)
{
	DBG_ENTER("mysqlx_column_def::charset");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr charset;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, column_def_class_entry,
		&charset.str, &charset.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_charset(charset);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::collation(string collation) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, collation)
{
	DBG_ENTER("mysqlx_column_def::collation");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr collation;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, column_def_class_entry,
		&collation.str, &collation.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_collation(collation);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::binary() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, binary)
{
	DBG_ENTER("mysqlx_column_def::binary");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_binary();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def::values(string[] values) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def, values)
{
	DBG_ENTER("mysqlx_column_def::values");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	zval* values_zv = nullptr;
	int values_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O+",
		&object_zv, column_def_class_entry,
		&values_zv, &values_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_values(
		phputils::to_strings(values_zv, values_count, &to_value_str));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_column_def_methods[] */
const zend_function_entry column_def_methods[] = {
	PHP_ME(mysqlx_column_def, __construct, arginfo_column_def_construct, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_column_def, setDefault, arginfo_column_def_set_default, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, defaultCurrentTimestamp, arginfo_column_def_default_current_timestamp, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, autoIncrement, arginfo_column_def_auto_increment, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, foreignKey, arginfo_column_def_foreign_key, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, unsigned, arginfo_column_def_unsigned, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, decimals, arginfo_column_def_decimals, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, charset, arginfo_column_def_charset, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, collation, arginfo_column_def_collation, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, binary, arginfo_column_def_binary, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def, values, arginfo_column_def_values, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


HashTable column_def_properties;

const st_mysqlx_property_entry column_def_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

} // anonymous namespace


/* {{{ mysqlx_register_column_def_class */
void mysqlx_register_column_def_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_DERIVED_CLASS(
		column_def_class_entry,
		column_def_base_class_entry,
		"ColumnDef",
		column_def_methods,
		column_def_properties,
		column_def_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_column_def_class */
void mysqlx_unregister_column_def_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&column_def_properties);
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
