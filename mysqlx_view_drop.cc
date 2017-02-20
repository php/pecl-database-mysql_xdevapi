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
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_ddl_view_commands.h"
#include "xmysqlnd/xmysqlnd_view.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_object.h"
#include "mysqlx_view_drop.h"
#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

static zend_class_entry* view_drop_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_drop_if_exists, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, if_exists, _IS_BOOL, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_view_drop_execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct view_drop_data : public phputils::custom_allocable
{
	~view_drop_data()
	{
		xmysqlnd_node_session_free(session);
	}

	drv::Drop_view_cmd command;
	st_xmysqlnd_node_session* session = nullptr;
};

/* {{{ mysqlx_view_drop::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_drop, __construct)
{
}
/* }}} */


/* {{{ mysqlx_view_drop::ifExists */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_drop, ifExists)
{
	DBG_ENTER("mysqlx_view_drop::ifExists");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, view_drop_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_drop_data>(object_zv);
	data_object.command.set_if_exists(true);

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_view_drop::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_view_drop, execute)
{
	DBG_ENTER("mysqlx_view_drop::execute");

	RETVAL_FALSE;

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O",
		&object_zv, view_drop_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<view_drop_data>(object_zv);
	const st_xmysqlnd_pb_message_shell pb_msg = data_object.command.get_message();
	st_xmysqlnd_node_stmt* stmt = drv::View::drop(data_object.session, pb_msg);
	zend_long flags = 0;
	execute_new_statement_read_response(
		stmt,
		flags,
		MYSQLX_RESULT,
		return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_view_drop_methods[] */
static const zend_function_entry view_drop_methods[] = {
	PHP_ME(mysqlx_view_drop, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_view_drop, ifExists, arginfo_view_drop_if_exists, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_view_drop, execute, arginfo_view_drop_execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers view_drop_handlers;
static HashTable view_drop_properties;

const st_mysqlx_property_entry view_drop_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_view_drop_free_storage */
static void
mysqlx_view_drop_free_storage(zend_object * object)
{
	phputils::free_object<view_drop_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_view_drop_object_allocator */
static zend_object *
php_mysqlx_view_drop_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_view_drop_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<view_drop_data>(
		class_type,
		&view_drop_handlers,
		&view_drop_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace

/* {{{ mysqlx_register_view_drop_class */
void
mysqlx_register_view_drop_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		view_drop_class_entry,
		"ViewDrop",
		mysqlx_std_object_handlers,
		view_drop_handlers,
		php_mysqlx_view_drop_object_allocator,
		mysqlx_view_drop_free_storage,
		view_drop_methods,
		view_drop_properties,
		view_drop_property_entries,
		mysqlx_executable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_view_drop_class */
void
mysqlx_unregister_view_drop_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&view_drop_properties);
}
/* }}} */


/* {{{ mysqlx_new_view_drop */
void
mysqlx_new_view_drop(
	zval* return_value,
	drv::st_xmysqlnd_node_schema* schema,
	const MYSQLND_CSTRING& view_name)
{
	DBG_ENTER("mysqlx_new_view_drop");

	const auto schema_name = mnd_str2c(schema->data->schema_name);
	if (is_empty_str(schema_name) || is_empty_str(view_name)) {
		throw phputils::doc_ref_exception(phputils::doc_ref_exception::Severity::warning, view_drop_class_entry);
	}

	auto& data_object = phputils::init_object<view_drop_data>(view_drop_class_entry, return_value);
	auto session = schema->data->session;
	data_object.session = session->m->get_reference(session);
	data_object.command.set_view_name(schema_name, view_name);

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
