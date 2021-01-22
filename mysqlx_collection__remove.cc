/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__remove.h"
#include "util/allocator.h"
#include "util/arguments.h"
#include "util/functions.h"
#include "util/object.h"
#include "util/value.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry* collection_remove_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expressions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------

bool Collection_remove::init(
	xmysqlnd_collection* coll,
	const util::string_view& search_expression)
{
	if (!coll || search_expression.empty()) return false;
	collection = coll->get_reference();
	remove_op = xmysqlnd_crud_collection_remove__create(
		collection->get_schema()->get_name(),
		collection->get_name());
	if (!remove_op) return false;

	return xmysqlnd_crud_collection_remove__set_criteria(
		remove_op, std::string{ search_expression }) == PASS;
}

Collection_remove::~Collection_remove()
{
	if (remove_op) {
		xmysqlnd_crud_collection_remove__destroy(remove_op);
	}

	if (collection) {
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
}

bool Collection_remove::sort(const util::arg_zvals& sort_expressions)
{
	DBG_ENTER("Collection_remove::sort");

	if (sort_expressions.empty()) {
		DBG_RETURN(false);
	}

	for (auto sort_expr : sort_expressions) {
		switch (sort_expr.type()) {
		case util::zvalue::Type::String:
			{
				const util::string_view sort_expr_str = sort_expr.to_string_view();
				if (FAIL == xmysqlnd_crud_collection_remove__add_sort(remove_op,
													sort_expr_str)) {
					DBG_RETURN(false);
				}
			}
			break;
		case util::zvalue::Type::Array:
			{
				for (auto it{ sort_expr.vbegin() }; it != sort_expr.vend(); ++it) {
					const util::zvalue& sort_expr_entry(*it);
					if (!sort_expr_entry.is_string()) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_RETURN(false);
					}
					const util::string_view sort_expr_str = sort_expr_entry.to_string_view();
					if (FAIL == xmysqlnd_crud_collection_remove__add_sort(remove_op,
															sort_expr_str)) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
						DBG_RETURN(false);
					}
				}
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			DBG_RETURN(false);
		}
	}
	DBG_RETURN(true);
}

bool Collection_remove::limit(zend_long rows)
{
	DBG_ENTER("Collection_remove::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(PASS == xmysqlnd_crud_collection_remove__set_limit(remove_op, static_cast<std::size_t>(rows)));
}

bool Collection_remove::bind(const util::zvalue& bind_variables)
{
	DBG_ENTER("Collection_remove::bind");

	for (const auto& [var_name, var_value] : bind_variables) {
		if (!var_name.is_string()) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
		if (FAIL == xmysqlnd_crud_collection_remove__bind_value(
			remove_op, var_name.to_string(), var_value)) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
	}

	DBG_RETURN(true);
}

util::zvalue Collection_remove::execute()
{
	DBG_ENTER("Collection_remove::execute");

	DBG_INF_FMT("remove_op=%p collection=%p", remove_op, collection);
	util::zvalue resultset;
	if (remove_op && collection) {
		if (FALSE == xmysqlnd_crud_collection_remove__is_initialized(remove_op)) {
			const int errcode{10002};
			constexpr util::string_view sqlstate = "HY000";
			constexpr util::string_view errmsg = "Remove not completely initialized";
			create_exception(errcode, sqlstate, errmsg);
		} else {
			xmysqlnd_stmt* stmt{ collection->remove(remove_op) };
			if (stmt) {
				util::zvalue stmt_obj = create_stmt(stmt);
				zend_long flags{0};
				resultset = mysqlx_statement_execute_read_response(
					Z_MYSQLX_P(stmt_obj.ptr()), flags, MYSQLX_RESULT);
			}
		}
	}

	DBG_RETURN(resultset);
}

//------------------------------------------------------------------------------


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__remove, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__remove, sort)
{
	DBG_ENTER("mysqlx_collection__remove::sort");

	util::raw_zval* object_zv{nullptr};
	util::arg_zvals sort_expressions;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O+",
									&object_zv,
									collection_remove_class_entry,
									&sort_expressions.data,
									&sort_expressions.counter))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = util::fetch_data_object<Collection_remove>(object_zv);
	if (coll_remove.sort(sort_expressions)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__remove, limit)
{
	DBG_ENTER("mysqlx_collection__remove::limit");

	util::raw_zval* object_zv{nullptr};
	zend_long rows{0};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, collection_remove_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = util::fetch_data_object<Collection_remove>(object_zv);
	if (coll_remove.limit(rows)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__remove, bind)
{
	DBG_ENTER("mysqlx_collection__remove::bind");

	util::raw_zval* object_zv{nullptr};
	util::raw_zval* bind_vars{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oz",
												&object_zv, collection_remove_class_entry,
												&bind_vars))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = util::fetch_data_object<Collection_remove>(object_zv);
	util::zvalue bind_variables(bind_vars);
	if (coll_remove.bind(bind_variables)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__remove, execute)
{
	DBG_ENTER("mysqlx_collection__remove::execute");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, collection_remove_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = util::fetch_data_object<Collection_remove>(object_zv);
	coll_remove.execute().move_to(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_collection__remove_methods[] = {
	PHP_ME(mysqlx_collection__remove, __construct, arginfo_mysqlx_collection__remove__construct, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_collection__remove, bind,	arginfo_mysqlx_collection__remove__bind,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__remove, sort,	arginfo_mysqlx_collection__remove__sort,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__remove, limit,	arginfo_mysqlx_collection__remove__limit,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__remove, execute,	arginfo_mysqlx_collection__remove__execute,	ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers collection_remove_handlers;
static HashTable collection_remove_properties;

const st_mysqlx_property_entry collection_remove_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_collection__remove_free_storage(zend_object* object)
{
	util::free_object<Collection_remove>(object);
}

static zend_object *
php_mysqlx_collection__remove_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__remove_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_remove>(
		class_type,
		&collection_remove_handlers,
		&collection_remove_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_collection__remove_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_remove_class_entry,
		"CollectionRemove",
		mysqlx_std_object_handlers,
		collection_remove_handlers,
		php_mysqlx_collection__remove_object_allocator,
		mysqlx_collection__remove_free_storage,
		mysqlx_collection__remove_methods,
		collection_remove_properties,
		collection_remove_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}

void
mysqlx_unregister_collection__remove_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_remove_properties);
}

util::zvalue
create_collection_remove(
	const util::string_view& search_expression,
	xmysqlnd_collection* collection)
{
	DBG_ENTER("create_collection_remove");
	util::zvalue coll_remove_obj;
	Collection_remove& coll_remove{ util::init_object<Collection_remove>(collection_remove_class_entry, coll_remove_obj) };
	if (!coll_remove.init(collection, search_expression)) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::remove_fail);
	}
	DBG_RETURN(coll_remove_obj);
}

} // namespace devapi

} // namespace mysqlx
