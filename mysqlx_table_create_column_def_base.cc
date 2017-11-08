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
#include "php_api.h"
extern "C" {
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_ddl_table_defs.h"

#include "mysqlx_class_properties.h"
#include "mysqlx_table_create_column_def_base.h"
#include "mysqlx_object.h"

#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

/*
	ColumnDefBase(columnName, ColumnType)
	ColumnDefBase(columnName, ColumnType, size)

	.notNull()
	.uniqueIndex()
	.primaryKey()
	.comment(string)
*/
zend_class_entry* column_def_base_class_entry{nullptr};

namespace
{

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_base_not_null, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_base_unique_index, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_base_primary_key, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_column_def_base_comment, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, comment, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_column_def_base::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def_base, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def_base::notNull() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def_base, notNull)
{
	DBG_ENTER("mysqlx_column_def_base::notNull");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_base_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_not_null();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def_base::uniqueIndex () */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def_base, uniqueIndex)
{
	DBG_ENTER("mysqlx_column_def_base::uniqueIndex");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_base_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_unique_index();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def_base::primaryKey() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def_base, primaryKey)
{
	DBG_ENTER("mysqlx_column_def_base::primaryKey");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, column_def_base_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.enable_primary_key();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_column_def_base::comment() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_def_base, comment)
{
	DBG_ENTER("mysqlx_column_def_base::comment");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view comment;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, column_def_base_class_entry,
		&comment.str, &comment.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(object_zv);
	data_object.column_def.set_comment(comment);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


const zend_function_entry column_def_base_methods[] = {
	PHP_ME(mysqlx_column_def_base, __construct, nullptr, ZEND_ACC_PROTECTED)

	PHP_ME(mysqlx_column_def_base, notNull, arginfo_column_def_base_not_null, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def_base, uniqueIndex, arginfo_column_def_base_unique_index, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def_base, primaryKey, arginfo_column_def_base_primary_key, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_def_base, comment, arginfo_column_def_base_comment, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


zend_object_handlers column_def_base_handlers;
HashTable column_def_base_properties;

const st_mysqlx_property_entry column_def_base_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_column_def_base_free_storage */
void mysqlx_column_def_base_free_storage(zend_object* object)
{
	phputils::free_object<Column_def_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_column_def_base_object_allocator */
zend_object* php_mysqlx_column_def_base_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_column_def_base_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Column_def_data>(
		class_type,
		&column_def_base_handlers,
		&column_def_base_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace


/* {{{ mysqlx_register_column_def_base_class */
void mysqlx_register_column_def_base_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		column_def_base_class_entry,
		"ColumnDefBase",
		mysqlx_std_object_handlers,
		column_def_base_handlers,
		php_mysqlx_column_def_base_object_allocator,
		mysqlx_column_def_base_free_storage,
		column_def_base_methods,
		column_def_base_properties,
		column_def_base_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_column_def_base_class */
void mysqlx_unregister_column_def_base_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&column_def_base_properties);
}
/* }}} */


/* {{{ get_column_def_from_object */
drv::Column_def get_column_def_from_object(zval* column_def_zv)
{
	zend_class_entry* column_def_ce
		= (Z_TYPE_P(column_def_zv) == IS_OBJECT) ? Z_OBJCE_P(column_def_zv) : nullptr;
	if (column_def_ce->parent != column_def_base_class_entry) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_table_column);
	}

	auto& data_object = phputils::fetch_data_object<Column_def_data>(column_def_zv);
	return data_object.column_def;
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
