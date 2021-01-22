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
#include "mysqlx_crud_operation_skippable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__modify.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/arguments.h"
#include "util/functions.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/value.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry* collection_modify_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expressions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__set, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__unset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__replace, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__patch, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, document, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__array_insert, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__array_append, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


bool Collection_modify::init(
	xmysqlnd_collection* coll,
	const util::string_view& search_expression)
{
	if (!coll || search_expression.empty()) return false;

	collection = coll->get_reference();
	modify_op = xmysqlnd_crud_collection_modify__create(
		collection->get_schema()->get_name(),
		collection->get_name());

	if (!modify_op) return false;

	return xmysqlnd_crud_collection_modify__set_criteria(
		modify_op, std::string{ search_expression });
}

Collection_modify::~Collection_modify()
{
	if (modify_op) {
		xmysqlnd_crud_collection_modify__destroy(modify_op);
	}

	if (collection) {
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
}

bool Collection_modify::sort(const util::arg_zvals& sort_expressions)
{
	DBG_ENTER("Collection_modify::sort");

	if (sort_expressions.empty()) {
		DBG_RETURN(false);
	}

	for (auto sort_expr : sort_expressions) {
		switch (sort_expr.type()) {
		case util::zvalue::Type::String:
			{
				const util::string_view& sort_expr_str = sort_expr.to_string_view();
				if (!xmysqlnd_crud_collection_modify__add_sort(modify_op,
															sort_expr_str)) {
					RAISE_EXCEPTION(err_msg_add_sort_fail);
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
					const util::string_view& sort_expr_str = sort_expr_entry.to_string_view();
					if (!xmysqlnd_crud_collection_modify__add_sort(modify_op,
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
			break;
		}
	}
	DBG_RETURN(true);
}

bool Collection_modify::limit(zend_long rows)
{
	DBG_ENTER("Collection_modify::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(xmysqlnd_crud_collection_modify__set_limit(modify_op, rows));
}

bool Collection_modify::skip(zend_long position)
{
	DBG_ENTER("Collection_modify::skip");

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(xmysqlnd_crud_collection_modify__set_skip(modify_op, position));
}

bool Collection_modify::bind(const util::zvalue& bind_variables)
{
	DBG_ENTER("Collection_modify::bind");

	for (const auto& [var_name, var_value] : bind_variables) {
		if (!var_name.is_string()) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
		if (!xmysqlnd_crud_collection_modify__bind_value(modify_op, var_name.to_string(), var_value)) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
	}

	DBG_RETURN(true);
}

drv::Modify_value Collection_modify::prepare_value(
	const util::string_view& path,
	util::zvalue value,
	bool validate_array)
{
	DBG_ENTER("Collection_modify::prepare_value");

	bool is_expression{ false };
	bool is_document{ false };

	switch (value.type()) {
		case util::zvalue::Type::Object:
			if (is_expression_object(value)) {
				value = get_expression_object(value);
				is_expression = true;
			} else {
				value = util::json::encode_document(value);
				is_document = true;
			}
			break;

		case util::zvalue::Type::Double:
		case util::zvalue::Type::True:
		case util::zvalue::Type::False:
		case util::zvalue::Type::Long:
		case util::zvalue::Type::Null:
			break;

		case util::zvalue::Type::String:
			if (util::json::can_be_document(value) || util::json::can_be_array(value)) {
				is_document = true;
			} else if (util::json::can_be_binding(value)) {
				is_expression = true;
			}
			break;

		case util::zvalue::Type::Array:
			value = util::json::encode_document(value);
			is_document = true;
			break;

		default:
			throw util::xdevapi_exception(util::xdevapi_exception::Code::invalid_type);
	}

	drv::Modify_value result{ path, value, is_expression, is_document, validate_array };
	DBG_RETURN(result);
}

bool Collection_modify::set(
	const util::string_view& path,
	const util::zvalue& value)
{
	DBG_ENTER("Collection_modify::set");
	DBG_RETURN(xmysqlnd_crud_collection_modify__set(modify_op, prepare_value(path, value)));
}

bool Collection_modify::unset(const util::arg_zvals& variables)
{
	DBG_ENTER("Collection_modify::unset");

	if (variables.empty()) {
		DBG_RETURN(false);
	}

	for (auto variable : variables) {
		switch (variable.type())
		{
		case util::zvalue::Type::String:
			{
				const util::string_view path{ variable.to_string_view() };
				if (!xmysqlnd_crud_collection_modify__unset(modify_op, path)) {
					RAISE_EXCEPTION(err_msg_unset_fail);
					DBG_RETURN(false);
				}
			}
			break;

		case util::zvalue::Type::Array:
			{
				for (auto it{ variable.vbegin() }; it != variable.vend(); ++it) {
					const util::zvalue& variable_entry(*it);
					if (!variable_entry.is_string()) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_RETURN(false);
					}
					const util::string_view path{ variable_entry.to_string_view() };
					if (!xmysqlnd_crud_collection_modify__unset(modify_op, path)) {
						RAISE_EXCEPTION(err_msg_unset_fail);
						DBG_RETURN(false);
					}
				}
			}
			break;

		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			DBG_RETURN(false);
			break;
		}
	}

	DBG_RETURN(true);
}

bool Collection_modify::replace(
	const util::string_view& path,
	const util::zvalue& value)
{
	DBG_ENTER("Collection_modify::replace");
	DBG_RETURN(xmysqlnd_crud_collection_modify__replace(modify_op, prepare_value(path, value)));
}

bool Collection_modify::patch(const util::string_view& document_contents)
{
	DBG_ENTER("Collection_modify::patch");
	const util::string_view Empty_doc_path;
	if (!xmysqlnd_crud_collection_modify__patch(
		modify_op, prepare_value(Empty_doc_path, document_contents))) {
		RAISE_EXCEPTION(err_msg_merge_fail);
		DBG_RETURN(false);
	}
	DBG_RETURN(true);
}

bool Collection_modify::array_insert(
	const util::string_view& path,
	const util::zvalue& value)
{
	DBG_ENTER("Collection_modify::array_insert");
	DBG_RETURN(xmysqlnd_crud_collection_modify__array_insert(modify_op, prepare_value(path, value, true)));
}

bool Collection_modify::array_append(
	const util::string_view& path,
	const util::zvalue& value)
{
	DBG_ENTER("Collection_modify::array_append");
	DBG_RETURN(xmysqlnd_crud_collection_modify__array_append(modify_op, prepare_value(path, value)));
}

util::zvalue Collection_modify::execute()
{
	DBG_ENTER("Collection_modify::execute");
	DBG_INF_FMT("modify_op=%p collection=%p", modify_op, collection);
	util::zvalue resultset;
	if (!xmysqlnd_crud_collection_modify__is_initialized(modify_op)) {
		RAISE_EXCEPTION(err_msg_modify_fail);
	} else {
		xmysqlnd_stmt* stmt = collection->modify(modify_op);
		if (stmt) {
			util::zvalue stmt_obj = create_stmt(stmt);
			zend_long flags{0};
			resultset = mysqlx_statement_execute_read_response(
				Z_MYSQLX_P(stmt_obj.ptr()),
				flags,
				MYSQLX_RESULT);
		}
	}

	DBG_RETURN(resultset);
}

//------------------------------------------------------------------------------


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, sort)
{
	DBG_ENTER("mysqlx_collection__modify::sort");

	util::raw_zval* object_zv{nullptr};
	util::arg_zvals sort_expressions;

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O+",
									&object_zv,
									collection_modify_class_entry,
									&sort_expressions.data,
									&sort_expressions.counter))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.sort(sort_expressions)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, limit)
{
	DBG_ENTER("mysqlx_collection__modify::limit");

	util::raw_zval* object_zv{nullptr};
	zend_long rows{0};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, collection_modify_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.limit(rows)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, skip)
{
	DBG_ENTER("mysqlx_collection__modify::skip");

	util::raw_zval* object_zv{nullptr};
	zend_long position{0};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, collection_modify_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.skip(position)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, bind)
{
	DBG_ENTER("mysqlx_collection__modify::bind");

	util::raw_zval* object_zv{nullptr};
	util::raw_zval* bind_vars{nullptr};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oz",
												&object_zv, collection_modify_class_entry,
												&bind_vars))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	util::zvalue bind_variables(bind_vars);
	if (coll_modify.bind(bind_variables)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, set)
{
	DBG_ENTER("mysqlx_collection__modify::set");

	util::raw_zval* object_zv{nullptr};
	util::arg_string path;
	util::raw_zval* value{nullptr};

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Osz",
		&object_zv, collection_modify_class_entry,
		&path.str, &path.len,
		&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.set(path.to_view(), value)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, replace)
{
	DBG_ENTER("mysqlx_collection__modify::replace");

	util::raw_zval* object_zv{nullptr};
	util::arg_string path;
	util::raw_zval* value{nullptr};

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Osz",
		&object_zv, collection_modify_class_entry,
		&path.str, &path.len,
		&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.replace(path.to_view(), value)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, patch)
{
	DBG_ENTER("mysqlx_collection__modify::patch");

	util::raw_zval* object_zv{nullptr};
	util::arg_string document_contents;

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Os",
		&object_zv, collection_modify_class_entry,
		&(document_contents.str), &(document_contents.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.patch(document_contents.to_view())) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, arrayInsert)
{
	DBG_ENTER("mysqlx_collection__modify::arrayInsert");

	util::raw_zval* object_zv{nullptr};
	util::arg_string path;
	util::raw_zval* value{nullptr};

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Osz",
		&object_zv, collection_modify_class_entry,
		&path.str, &path.len,
		&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.array_insert(path.to_view(), value)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, arrayAppend)
{
	DBG_ENTER("mysqlx_collection__modify::arrayAppend");

	util::raw_zval* object_zv{nullptr};
	util::arg_string path;
	util::raw_zval* value{nullptr};

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Osz",
		&object_zv, collection_modify_class_entry,
		&path.str, &path.len,
		&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.array_append(path.to_view(), value)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, unset)
{
	DBG_ENTER("mysqlx_collection__modify::unset");

	util::raw_zval* object_zv{nullptr};
	util::arg_zvals variables;

	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "O+",
		&object_zv,
		collection_modify_class_entry,
		&variables.data,
		&variables.counter))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.unset(variables)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, execute)
{
	DBG_ENTER("mysqlx_collection__modify::execute");

	util::raw_zval* object_zv{nullptr};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, collection_modify_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.execute().move_to(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_collection__modify_methods[] = {
	PHP_ME(mysqlx_collection__modify, __construct, arginfo_mysqlx_collection__modify__construct, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_collection__modify,	bind,		arginfo_mysqlx_collection__modify__bind,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	sort,		arginfo_mysqlx_collection__modify__sort,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	limit,		arginfo_mysqlx_collection__modify__limit,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	skip,		arginfo_mysqlx_collection__modify__skip,			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection__modify,	set,		arginfo_mysqlx_collection__modify__set,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	unset,		arginfo_mysqlx_collection__modify__unset,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	replace,	arginfo_mysqlx_collection__modify__replace,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	patch,		arginfo_mysqlx_collection__modify__patch,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayInsert,arginfo_mysqlx_collection__modify__array_insert,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayAppend,arginfo_mysqlx_collection__modify__array_append,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection__modify,	execute,	arginfo_mysqlx_collection__modify__execute,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers collection_modify_handlers;
static HashTable collection_modify_properties;

const st_mysqlx_property_entry collection_modify_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_collection__modify_free_storage(zend_object* object)
{
	util::free_object<Collection_modify>(object);
}

static zend_object *
php_mysqlx_collection__modify_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__modify_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_modify>(
		class_type,
		&collection_modify_handlers,
		&collection_modify_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_collection__modify_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_modify_class_entry,
		"CollectionModify",
		mysqlx_std_object_handlers,
		collection_modify_handlers,
		php_mysqlx_collection__modify_object_allocator,
		mysqlx_collection__modify_free_storage,
		mysqlx_collection__modify_methods,
		collection_modify_properties,
		collection_modify_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_skippable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}

void
mysqlx_unregister_collection__modify_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_modify_properties);
}

util::zvalue
create_collection_modify(
	const util::string_view& search_expression,
	xmysqlnd_collection* collection)
{
	DBG_ENTER("create_collection_modify");

	util::zvalue coll_modify_obj;
	Collection_modify& coll_modify{ util::init_object<Collection_modify>(collection_modify_class_entry, coll_modify_obj) };
	if (!coll_modify.init(collection, search_expression)) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::modify_fail);
	}

	DBG_RETURN(coll_modify_obj);
}

} // namespace devapi

} // namespace mysqlx
