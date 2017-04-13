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
#include "xmysqlnd/xmysqlnd_ddl_table_defs.h"
#include "xmysqlnd/xmysqlnd_utils.h"

#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_table_create_column_def_base.h"
#include "mysqlx_table_create_generated_column_def.h"
#include "mysqlx_object.h"

#include "phputils/allocator.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

/*
	var generatedColumn = new GeneratedColumnDef(columnName, ColumnType, expr)

	.stored()
	.uniqueIndex()
	.comment(string)
	.notNull()
	.primaryKey()
*/
namespace
{

zend_class_entry* generated_column_def_class_entry = nullptr;

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_construct, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, type, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, expression, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_generated_column_def_stored, 0, ZEND_RETURN_VALUE, 1)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_generated_column_def::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_generated_column_def, __construct)
{
	DBG_ENTER("mysqlx_generated_column_def::__construct");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_input_param name;
	phputils::string_input_param type;
	phputils::string_input_param expression;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Osss",
		&object_zv, generated_column_def_class_entry,
		&name.str, &name.len,
		&type.str, &type.len,
		&expression.str, &expression.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.init(name, type, expression);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_generated_column_def::stored() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_generated_column_def, stored)
{
	DBG_ENTER("mysqlx_generated_column_def::stored");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, generated_column_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_stored();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_generated_column_def_methods[] */
const zend_function_entry generated_column_def_methods[] = {
	PHP_ME(mysqlx_generated_column_def, __construct, nullptr, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_generated_column_def, stored, arginfo_generated_column_def_stored, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


HashTable generated_column_def_properties;

const st_mysqlx_property_entry generated_column_def_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

} // anonymous namespace


/* {{{ mysqlx_register_generated_column_def_class */
void mysqlx_register_generated_column_def_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_DERIVED_CLASS(
		generated_column_def_class_entry,
		column_def_base_class_entry,
		"GeneratedColumnDef",
		generated_column_def_methods,
		generated_column_def_properties,
		generated_column_def_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_generated_column_def_class */
void mysqlx_unregister_generated_column_def_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&generated_column_def_properties);
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
