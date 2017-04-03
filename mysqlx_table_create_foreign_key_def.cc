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
#include "mysqlx_table_create_foreign_key_def.h"
#include "mysqlx_object.h"

#include "phputils/allocator.h"
#include "phputils/object.h"
#include "phputils/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

/*
	FkSpec
		::= 'new ForeignKeyDef()'
			'.fields(' StringLiteral (',' StringLiteral)* ')'
			'.refersTo(' StringLiteral (',' StringLiteral)+ ')' // table and columns
			( '.onDelete(' ('Restrict' | 'Cascade' | 'SetNull') ')' )?
			( '.onUpdate(' ('Restrict' | 'Cascade' | 'SetNull') ')' )?
*/
zend_class_entry* foreign_key_def_class_entry = nullptr;

namespace
{

ZEND_BEGIN_ARG_INFO_EX(arginfo_foreign_key_def_construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_foreign_key_def_fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_foreign_key_def_refers_to, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, refers_to_table, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, refers_to_columns)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_foreign_key_def_on_delete, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, on_delete_mode, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_foreign_key_def_on_update, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, on_update_mode, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


struct Foreign_key_def_data : public phputils::custom_allocable
{
	Foreign_key_def fk_def;
};

/* {{{ mysqlx_foreign_key_def::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_foreign_key_def, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_foreign_key_def::fields(string[] columns) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_foreign_key_def, fields)
{
	DBG_ENTER("mysqlx_foreign_key_def::fields");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	zval* fields_zv = nullptr;
	int fields_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O+",
		&object_zv, foreign_key_def_class_entry,
		&fields_zv, &fields_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Foreign_key_def_data>(object_zv);
	data_object.fk_def.set_fields(phputils::to_strings(fields_zv, fields_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_foreign_key_def::refersTo(string table, string[] fields) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_foreign_key_def, refersTo)
{
	DBG_ENTER("mysqlx_foreign_key_def::refersTo");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr refers_to_table;
	zval* refers_to_columns = nullptr;
	int refers_to_column_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os+",
		&object_zv, foreign_key_def_class_entry,
		&refers_to_table.str, &refers_to_table.len,
		&refers_to_columns, &refers_to_column_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Foreign_key_def_data>(object_zv);
	data_object.fk_def.set_refers_to(
		refers_to_table,
		phputils::to_strings(refers_to_columns, refers_to_column_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_foreign_key_def::onDelete(string onDeleteMode) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_foreign_key_def, onDelete)
{
	DBG_ENTER("mysqlx_foreign_key_def::onDelete");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr on_delete_mode;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, foreign_key_def_class_entry,
		&on_delete_mode.str, &on_delete_mode.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Foreign_key_def_data>(object_zv);
	data_object.fk_def.set_on_delete_mode(on_delete_mode);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_foreign_key_def::onUpdate(string onUpdateMode) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_foreign_key_def, onUpdate)
{
	DBG_ENTER("mysqlx_foreign_key_def::onUpdate");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	phputils::string_ptr on_update_mode;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, foreign_key_def_class_entry,
		&on_update_mode.str, &on_update_mode.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Foreign_key_def_data>(object_zv);
	data_object.fk_def.set_on_update_mode(on_update_mode);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_foreign_key_def_methods[] */
const zend_function_entry foreign_key_def_methods[] = {
	PHP_ME(mysqlx_foreign_key_def, __construct, arginfo_foreign_key_def_construct, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_foreign_key_def, fields, arginfo_foreign_key_def_fields, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_foreign_key_def, refersTo, arginfo_foreign_key_def_refers_to, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_foreign_key_def, onDelete, arginfo_foreign_key_def_on_delete, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_foreign_key_def, onUpdate, arginfo_foreign_key_def_on_update, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


zend_object_handlers foreign_key_def_handlers;
HashTable foreign_key_def_properties;

const st_mysqlx_property_entry foreign_key_def_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_foreign_key_def_free_storage */
void mysqlx_foreign_key_def_free_storage(zend_object* object)
{
	phputils::free_object<Foreign_key_def_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_foreign_key_def_object_allocator */
zend_object* php_mysqlx_foreign_key_def_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_foreign_key_def_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Foreign_key_def_data>(
		class_type,
		&foreign_key_def_handlers,
		&foreign_key_def_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace


/* {{{ mysqlx_register_foreign_key_def_class */
void mysqlx_register_foreign_key_def_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		foreign_key_def_class_entry,
		"ForeignKeyDef",
		mysqlx_std_object_handlers,
		foreign_key_def_handlers,
		php_mysqlx_foreign_key_def_object_allocator,
		mysqlx_foreign_key_def_free_storage,
		foreign_key_def_methods,
		foreign_key_def_properties,
		foreign_key_def_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_foreign_key_def_class */
void mysqlx_unregister_foreign_key_def_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&foreign_key_def_properties);
}
/* }}} */


/* {{{ get_foreign_key_def_from_object */
Foreign_key_def get_foreign_key_def_from_object(zval* fk_def_zv)
{
	zend_class_entry* fk_def_ce = Z_OBJCE_P(fk_def_zv);
	if (fk_def_ce != foreign_key_def_class_entry) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_foreign_key);
	}

	auto& data_object = phputils::fetch_data_object<Foreign_key_def_data>(fk_def_zv);
	return data_object.fk_def;
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
