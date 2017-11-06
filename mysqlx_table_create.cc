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
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_ddl_table_defs.h"
#include "xmysqlnd/xmysqlnd_table_create.h"
#include "xmysqlnd/xmysqlnd_utils.h"

#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_table_create.h"
#include "mysqlx_table_create_column_def_base.h"
#include "mysqlx_table_create_foreign_key_def.h"
#include "mysqlx_object.h"

#include "phputils/allocator.h"
#include "phputils/object.h"
#include "phputils/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

/*
	CreateTableFunction
		::= '.createTable(' StringLiteral (',' Boolean)? ')'
			( '.addColumn(' ColumnDef ')' )*
			( '.addPrimaryKey(' StringLiteral (',' StringLiteral)* ')' )?
			( '.addIndex(' StringLiteral (',' StringLiteral)+ ')' )*
			( '.addUniqueIndex(' StringLiteral (',' StringLiteral)+ ')' )*
			( '.addForeignKey(' StringLiteral ',' FkSpec ')' )*
			( '.setInitialAutoIncrement(' Number ')' )?
			( '.setDefaultCharset(' StringLiteral ')' )?
			( '.setDefaultCollation(' StringLiteral ')' )?
			( '.setComment(' StringLiteral ')' )?
			( '.temporary()' )?
			( '.as(' (StringLiteral | SelectStatement) ')' )?
			'.execute()'

	CreateTableFunction
		::= '.createTable(' StringLiteral (',' Boolean)? ')'
			'.like(' StringLiteral ')'
			'.execute()'
*/
zend_class_entry* table_create_class_entry{nullptr};

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_add_column, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, column, IS_OBJECT, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_add_primary_key, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, primary_key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_add_index, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, columns)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_add_unique_index, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, columns)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_add_foreign_key, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, table_name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, fields, IS_OBJECT, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_set_initial_auto_increment, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, init_auto_increment, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_set_default_charset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, default_charset, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_set_default_collation, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, default_collation, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_set_comment, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, comment, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_temporary, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_as, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, defined_as, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_like, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, template_table_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_table_create_get_sql_query, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ Table_create_data */
struct Table_create_data : public phputils::custom_allocable
{
	~Table_create_data()
	{
		xmysqlnd_node_session_free(session);
	}

	st_xmysqlnd_node_session* session{nullptr};
	drv::Table_def table_def;
};
/* }}} */


/* {{{ mysqlx_table_create::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::addColumn(Column_def column) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, addColumn)
{
	DBG_ENTER("mysqlx_table_create::addColumn");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	zval* column_def_zv;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oz",
		&object_zv, table_create_class_entry,
		&column_def_zv))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.add_column(get_column_def_from_object(column_def_zv));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::addPrimaryKey(string[] fields) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, addPrimaryKey)
{
	DBG_ENTER("mysqlx_table_create::addPrimaryKey");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	zval* fields_zv{nullptr};
	int fields_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O+",
		&object_zv, table_create_class_entry,
		&fields_zv, &fields_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_primary_key(phputils::to_strings(fields_zv, fields_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::addIndex(string name, string[] fields) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, addIndex)
{
	DBG_ENTER("mysqlx_table_create::addIndex");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view index_name;
	zval* fields_zv{nullptr};
	int fields_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os+",
		&object_zv, table_create_class_entry,
		&index_name.str, &index_name.len,
		&fields_zv, &fields_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.add_index(index_name, phputils::to_strings(fields_zv, fields_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ proto mixed mysqlx_table_create::addUniqueIndex(string name, string[] fields) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, addUniqueIndex)
{
	DBG_ENTER("mysqlx_table_create::addUniqueIndex");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view index_name;
	zval* fields_zv{nullptr};
	int fields_count = 0;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os+",
		&object_zv, table_create_class_entry,
		&index_name.str, &index_name.len,
		&fields_zv, &fields_count))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.add_unique_index(index_name, phputils::to_strings(fields_zv, fields_count));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::addForeignKey(string name, Foreign_key fkey) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, addForeignKey)
{
	DBG_ENTER("mysqlx_table_create::addForeignKey");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view fkey_name;
	zval* fk_def_zv;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "OsO",
		&object_zv, table_create_class_entry,
		&fkey_name.str, &fkey_name.len,
		&fk_def_zv, foreign_key_def_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.add_foreign_key(fkey_name, get_foreign_key_def_from_object(fk_def_zv));

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::setInitialAutoIncrement(long initialAutoIncrement) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, setInitialAutoIncrement)
{
	DBG_ENTER("mysqlx_table_create::setInitialAutoIncrement");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	zend_long initial_auto_increment = 0;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Ol",
		&object_zv, table_create_class_entry,
		&initial_auto_increment))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_initial_auto_increment(initial_auto_increment);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::setDefaultCharset(string defaultCharset) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, setDefaultCharset)
{
	DBG_ENTER("mysqlx_table_create::setDefaultCharset");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view default_charset;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, table_create_class_entry,
		&default_charset.str, &default_charset.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_default_charset(default_charset);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::setDefaultCollation(string DefaultCollation) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, setDefaultCollation)
{
	DBG_ENTER("mysqlx_table_create::setDefaultCollation");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view default_collation;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, table_create_class_entry,
		&default_collation.str, &default_collation.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_default_collation(default_collation);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::setComment(string Comment) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, setComment)
{
	DBG_ENTER("mysqlx_table_create::setComment");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view comment;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, table_create_class_entry,
		&comment.str, &comment.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_comment(comment);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::temporary() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, temporary)
{
	DBG_ENTER("mysqlx_table_create::temporary");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, table_create_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.enable_temporary();

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::as(string || selectTable) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, as)
{
	DBG_ENTER("mysqlx_table_create::as");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view defined_as;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, table_create_class_entry,
		&defined_as.str, &defined_as.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_defined_as(defined_as);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::like(string Like) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, like)
{
	DBG_ENTER("mysqlx_table_create::like");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	phputils::string_view template_table_name;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, table_create_class_entry,
		&template_table_name.str, &template_table_name.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	data_object.table_def.set_like(template_table_name);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, execute)
{
	DBG_ENTER("mysqlx_table_create::execute");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, table_create_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	if (drv::create_table::execute(data_object.session, data_object.table_def)) {
		RETVAL_TRUE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table_create::getSqlQuery() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table_create, getSqlQuery)
{
	DBG_ENTER("mysqlx_table_create::getSqlQuery");

	RETVAL_FALSE;

	zval* object_zv{nullptr};
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, table_create_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<Table_create_data>(object_zv);
	const phputils::string& sql_query = drv::create_table::get_sql_query(data_object.table_def);
	RETVAL_STRINGL(sql_query.c_str(), sql_query.length());

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_table_create_methods[] */
const zend_function_entry table_create_methods[] = {
	PHP_ME(mysqlx_table_create, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_table_create, addColumn, arginfo_table_create_add_column, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, addPrimaryKey, arginfo_table_create_add_primary_key, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, addIndex, arginfo_table_create_add_index, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, addUniqueIndex, arginfo_table_create_add_unique_index, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, addForeignKey, arginfo_table_create_add_foreign_key, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, setInitialAutoIncrement, arginfo_table_create_set_initial_auto_increment, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, setDefaultCharset, arginfo_table_create_set_default_charset, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, setDefaultCollation, arginfo_table_create_set_default_collation, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, setComment, arginfo_table_create_set_comment, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, temporary, arginfo_table_create_temporary, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, as, arginfo_table_create_as, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, like, arginfo_table_create_like, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, execute, arginfo_table_create_execute, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table_create, getSqlQuery, arginfo_table_create_get_sql_query, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


zend_object_handlers table_create_handlers;
HashTable table_create_properties;

const st_mysqlx_property_entry table_create_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_table_create_free_storage */
void mysqlx_table_create_free_storage(zend_object* object)
{
	phputils::free_object<Table_create_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_table_create_object_allocator */
zend_object* php_mysqlx_table_create_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_table_create_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Table_create_data>(
		class_type,
		&table_create_handlers,
		&table_create_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace


/* {{{ mysqlx_register_table_create_class */
void mysqlx_register_table_create_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		table_create_class_entry,
		"TableCreate",
		mysqlx_std_object_handlers,
		table_create_handlers,
		php_mysqlx_table_create_object_allocator,
		mysqlx_table_create_free_storage,
		table_create_methods,
		table_create_properties,
		table_create_property_entries,
		mysqlx_executable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_table_create_class */
void mysqlx_unregister_table_create_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&table_create_properties);
}
/* }}} */


/* {{{ mysqlx_new_table_create */
void mysqlx_new_table_create(
	zval* return_value,
	drv::st_xmysqlnd_node_schema* schema,
	const phputils::string_view& table_name,
	bool replace_existing)
{
	DBG_ENTER("mysqlx_new_table_create");

	const auto schema_name = mnd_str2c(schema->data->schema_name);
	if (drv::is_empty_str(schema->data->schema_name) || table_name.empty()) {
		throw phputils::doc_ref_exception(phputils::doc_ref_exception::Severity::warning, table_create_class_entry);
	}

	auto& data_object = phputils::init_object<Table_create_data>(table_create_class_entry, return_value);
	auto& session = schema->data->session;
	data_object.session = session->m->get_reference(session);
	data_object.table_def.init(schema_name, table_name, replace_existing);

	DBG_VOID_RETURN;
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
