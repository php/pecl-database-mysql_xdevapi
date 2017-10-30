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
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_ddl_view_commands.h"
#include "xmysqlnd/xmysqlnd_view.h"
#include "xmysqlnd/xmysqlnd_utils.h"

#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_collection__find.h"
#include "mysqlx_node_table__select.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_view_create.h"
#include "mysqlx_object.h"
#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

static zend_class_entry* view_create_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_definer, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, definer, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_algorithm, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, algorithm, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_security, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, security, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_with_check_option, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, check_option, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_columns, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, columns)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_defined_as, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, defined_as, IS_OBJECT, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_create_execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct view_create_data : public phputils::custom_allocable
{
	~view_create_data()
	{
		xmysqlnd_node_session_free(session);
	}

	void add_column(zval* column_zv)
	{
		if (Z_TYPE_P(column_zv) != IS_STRING) {
			throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_columns);
		}

		MYSQLND_CSTRING column = { Z_STRVAL_P(column_zv), Z_STRLEN_P(column_zv) };
		command.add_column(column);
	}

	drv::Create_view_cmd command;
	st_xmysqlnd_node_session* session = nullptr;
};

/* {{{ mysqlx_view_create::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::definer(string definer) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, definer)
{
	DBG_ENTER("mysqlx_view_create::definer");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	MYSQLND_CSTRING definer = { nullptr, 0 };
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, view_create_class_entry,
		&definer.s, &definer.l))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	data_object.command.set_definer(definer);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::algorithm(string algorithm) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, algorithm)
{
	DBG_ENTER("mysqlx_view_create::algorithm");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	MYSQLND_CSTRING algorithm = { nullptr, 0 };
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, view_create_class_entry,
		&algorithm.s, &algorithm.l))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	data_object.command.set_algorithm(algorithm);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::security(string security) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, security)
{
	DBG_ENTER("mysqlx_view_create::security");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	MYSQLND_CSTRING security = { nullptr, 0 };
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, view_create_class_entry,
		&security.s, &security.l))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	data_object.command.set_security(security);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::withCheckOption(string check_option) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, withCheckOption)
{
	DBG_ENTER("mysqlx_view_create::withCheckOption");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	MYSQLND_CSTRING check_option = { nullptr, 0 };
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, view_create_class_entry,
		&check_option.s, &check_option.l))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	data_object.command.set_check_option(check_option);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::columns(string/array columns) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, columns)
{
	DBG_ENTER("mysqlx_view_create::columns");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	zval* columns_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oz",
		&object_zv, view_create_class_entry,
		&columns_zv))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);

	if (Z_TYPE_P(columns_zv) == IS_STRING) {
		data_object.add_column(columns_zv);
	} else if (Z_TYPE_P(columns_zv) == IS_ARRAY) {
		zval* column_zv = nullptr;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(columns_zv), column_zv) {
			data_object.add_column(column_zv);
		} ZEND_HASH_FOREACH_END();
	} else {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_columns);
	}

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::definedAs(findCollection/selectTable) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, definedAs)
{
	DBG_ENTER("mysqlx_view_create::definedAs");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	zval* defined_as_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Oz",
		&object_zv, view_create_class_entry,
		&defined_as_zv))
	{
		DBG_VOID_RETURN;
	}

	if (Z_TYPE_P(defined_as_zv) != IS_OBJECT) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_defined_as);
	}

	zend_class_entry* defined_as_ce = Z_OBJCE_P(defined_as_zv);
	Mysqlx::Crud::Find* stmt = nullptr;
	if (defined_as_ce == mysqlx_node_table__select_class_entry) {
		stmt = get_stmt_from_table_select(defined_as_zv);
	} else if (defined_as_ce == collection_find_class_entry) {
		stmt = get_stmt_from_collection_find(defined_as_zv);
	} else {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::find_fail);
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	data_object.command.set_stmt(*stmt);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_create::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_create, execute)
{
	DBG_ENTER("mysqlx_view_create::execute");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, view_create_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_create_data>(object_zv);
	const st_xmysqlnd_pb_message_shell pb_msg = data_object.command.get_message();
	st_xmysqlnd_node_stmt* stmt = drv::View::create(data_object.session, pb_msg);
	zend_long flags = 0;
	execute_new_statement_read_response(
		stmt,
		flags,
		MYSQLX_RESULT,
		return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_view_create_methods[] */
static const zend_function_entry view_create_methods[] = {
	PHP_ME(mysqlx_view_create, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_view_create, definer, arginfo_view_create_definer, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_create, algorithm, arginfo_view_create_algorithm, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_create, security, arginfo_view_create_security, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_create, withCheckOption, arginfo_view_create_with_check_option, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_create, columns, arginfo_view_create_columns, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_create, definedAs, arginfo_view_create_defined_as, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_view_create, execute, arginfo_view_create_execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers view_create_handlers;
static HashTable view_create_properties;

const st_mysqlx_property_entry view_create_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_view_create_free_storage */
static void
mysqlx_view_create_free_storage(zend_object* object)
{
	phputils::free_object<view_create_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_view_create_object_allocator */
static zend_object *
php_mysqlx_view_create_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_view_create_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<view_create_data>(
		class_type,
		&view_create_handlers,
		&view_create_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace


/* {{{ mysqlx_register_view_create_class */
void
mysqlx_register_view_create_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		view_create_class_entry,
		"ViewCreate",
		mysqlx_std_object_handlers,
		view_create_handlers,
		php_mysqlx_view_create_object_allocator,
		mysqlx_view_create_free_storage,
		view_create_methods,
		view_create_properties,
		view_create_property_entries,
		mysqlx_executable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_view_create_class */
void
mysqlx_unregister_view_create_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&view_create_properties);
}
/* }}} */


/* {{{ mysqlx_new_view_create */
void
mysqlx_new_view_create(
	zval* return_value,
	drv::st_xmysqlnd_node_schema* schema,
	const MYSQLND_CSTRING& view_name,
	bool replace_existing)
{
	DBG_ENTER("mysqlx_new_view_create");

	const auto schema_name = mnd_str2c(schema->data->schema_name);
	if (is_empty_str(schema->data->schema_name) || is_empty_str(view_name)) {
		throw phputils::doc_ref_exception(phputils::doc_ref_exception::Severity::warning, view_create_class_entry);
	}

	auto& data_object = phputils::init_object<view_create_data>(view_create_class_entry, return_value);
	auto& session = schema->data->session;
	data_object.session = session->m->get_reference(session);
	data_object.command.set_view_name(schema_name, view_name);
	data_object.command.set_replace_existing(replace_existing);

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
