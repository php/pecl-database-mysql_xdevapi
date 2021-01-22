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
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_skippable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__find.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/arguments.h"
#include "util/functions.h"
#include "util/object.h"
#include "util/value.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

zend_class_entry* collection_find_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, projection)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__group_by, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expressions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__having, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, search_condition)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expressions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__offset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__lock_shared, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__lock_exclusive, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find__execute, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, flags, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


bool Collection_find::init(
	xmysqlnd_collection* coll,
	const util::string_view& search_expression)
{
	if (!coll) return false;

	collection = coll->get_reference();
	find_op = xmysqlnd_crud_collection_find__create(
		collection->get_schema()->get_name(),
		collection->get_name());

	if (!find_op) return false;

	if (search_expression.empty()) return true;

	return xmysqlnd_crud_collection_find__set_criteria(
		find_op, search_expression) == PASS;
}

Collection_find::~Collection_find()
{
	if (find_op) {
		xmysqlnd_crud_collection_find__destroy(find_op);
	}

	if (collection) {
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
}

bool Collection_find::fields(util::zvalue& fields)
{
	DBG_ENTER("Collection_find::fields");

	bool is_expression{false};
	switch (fields.type()) {
		case util::zvalue::Type::String:
		case util::zvalue::Type::Array:
			break;
		case util::zvalue::Type::Object:
			if (is_expression_object(fields)) {
				/* get the string */
				fields = get_expression_object(fields);
				is_expression = true;
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_RETURN(false);
	}

	enum_func_status ret{PASS};
	if (fields.is_string()) {
		const util::string_view& field_str = fields.to_string_view();
		ret = xmysqlnd_crud_collection_find__set_fields(find_op, field_str, is_expression, TRUE);
	} else if (fields.is_array()) {
		for (auto it{ fields.vbegin() }; it != fields.vend(); ++it) {
			util::zvalue field(*it);
			is_expression = false;
			if (field.is_object()) {
				if (is_expression_object(field)) {
					/* get the string */
					field = get_expression_object(field);
					is_expression = true;
				}
			}
			/* NO else */
			if (!field.is_string()) {
				RAISE_EXCEPTION(err_msg_wrong_param_1);
				DBG_RETURN(false);
			}
			const util::string_view& field_str = field.to_string_view();
			ret = xmysqlnd_crud_collection_find__set_fields(find_op, field_str, is_expression, TRUE);
			if(ret==FAIL)
				break;
		}
	}
	if (FAIL == ret) {
		RAISE_EXCEPTION(err_msg_add_field);
		DBG_RETURN(false);
	}

	DBG_RETURN(true);
}

bool Collection_find::add_operation(
	Collection_find::Operation operation,
	const util::arg_zvals& sort_expressions)
{
	DBG_ENTER("Collection_find::add_operation");

	for (auto sort_expr : sort_expressions) {
		switch(sort_expr.type()) {
		case util::zvalue::Type::String:
		case util::zvalue::Type::Object:
		case util::zvalue::Type::Array:
			break;
		default:
			php_error_docref(
				nullptr,
				E_WARNING,
				"Only strings, objects and arrays can be added. Type is %u",
				static_cast<unsigned int>(sort_expr.type()));
			DBG_RETURN(false);
		}
	}

	for (auto sort_expr : sort_expressions) {
		switch (sort_expr.type()) {
		case util::zvalue::Type::String:
			{
				const util::string_view& sort_expr_str = sort_expr.to_string_view();
				if (Collection_find::Operation::Sort == operation) {
					if (FAIL == xmysqlnd_crud_collection_find__add_sort(find_op, sort_expr_str)) {
						DBG_RETURN(false);
					}
				} else if (Collection_find::Operation::Group_by == operation) {
					if (FAIL == xmysqlnd_crud_collection_find__add_grouping(find_op, sort_expr_str)) {
						DBG_RETURN(false);
					}
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
					enum_func_status ret{FAIL};
					if (Collection_find::Operation::Sort == operation) {
						ret = xmysqlnd_crud_collection_find__add_sort(find_op, sort_expr_str);
					} else if (Collection_find::Operation::Group_by == operation) {
						ret = xmysqlnd_crud_collection_find__add_grouping(find_op, sort_expr_str);
					}
					if (FAIL == ret) {
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

bool Collection_find::sort(const util::arg_zvals& sort_expressions)
{
	DBG_ENTER("mysqlx_collection__find::sort");
	DBG_RETURN(add_operation(Operation::Sort, sort_expressions));
}

bool Collection_find::group_by(const util::arg_zvals& sort_expressions)
{
	DBG_ENTER("mysqlx_collection__find::group_by");
	DBG_RETURN(add_operation(Operation::Group_by, sort_expressions));
}

bool Collection_find::having(const util::string_view& search_condition)
{
	DBG_ENTER("mysqlx_collection__find::having");
	DBG_RETURN(PASS == xmysqlnd_crud_collection_find__set_having(find_op, search_condition));
}

bool Collection_find::limit(zend_long rows)
{
	DBG_ENTER("mysqlx_collection__find::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(PASS == xmysqlnd_crud_collection_find__set_limit(find_op, rows));
}

bool Collection_find::offset(zend_long position)
{
	DBG_ENTER("mysqlx_collection__find::offset");

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(PASS == xmysqlnd_crud_collection_find__set_offset(find_op, position));
}

bool Collection_find::bind(const util::zvalue& bind_variables)
{
	DBG_ENTER("mysqlx_collection__find::bind");

	for (const auto& [var_name, var_value] : bind_variables) {
		if (!var_name.is_string()) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
		if (FAIL == xmysqlnd_crud_collection_find__bind_value(find_op, var_name.to_string(), var_value)) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
	}

	DBG_RETURN(true);
}

bool Collection_find::lock_shared(int lock_waiting_option)
{
	DBG_ENTER("mysqlx_collection__find::lock_shared");
	DBG_RETURN((xmysqlnd_crud_collection_find__enable_lock_shared(find_op) == PASS)
		&& (xmysqlnd_crud_collection_find_set_lock_waiting_option(find_op, lock_waiting_option) == PASS));
}

bool Collection_find::lock_exclusive(int lock_waiting_option)
{
	DBG_ENTER("mysqlx_collection__find::lock_exclusive");
	DBG_RETURN((xmysqlnd_crud_collection_find__enable_lock_exclusive(find_op) == PASS)
		&& (xmysqlnd_crud_collection_find_set_lock_waiting_option(find_op, lock_waiting_option) == PASS));
}

util::zvalue Collection_find::execute()
{
	return execute(MYSQLX_EXECUTE_FLAG_BUFFERED);
}

util::zvalue Collection_find::execute(zend_long flags)
{
	DBG_ENTER("mysqlx_collection__find::execute");

	xmysqlnd_crud_collection_find_verify_is_initialized(find_op);

	xmysqlnd_stmt* stmt{ collection->find(find_op) };
	util::zvalue resultset;
	if (stmt) {
		util::zvalue stmt_obj = create_stmt(stmt);
		resultset = mysqlx_statement_execute_read_response(
			Z_MYSQLX_P(stmt_obj.ptr()), flags, MYSQLX_RESULT_DOC);
	}

	DBG_RETURN(resultset);
}

Mysqlx::Crud::Find* Collection_find::get_stmt()
{
	if (!find_op
		|| (xmysqlnd_crud_collection_find__finalize_bind(find_op) == FAIL)
		|| !xmysqlnd_crud_collection_find__is_initialized(find_op))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
	}

	st_xmysqlnd_pb_message_shell msg_shell = xmysqlnd_crud_collection_find__get_protobuf_message(find_op);
	Mysqlx::Crud::Find* msg = static_cast<Mysqlx::Crud::Find*>(msg_shell.message);
	return msg;
}

//------------------------------------------------------------------------------


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, fields)
{
	DBG_ENTER("mysqlx_collection__find::fields");

	util::raw_zval* object_zv{nullptr};
	util::raw_zval* raw_fields{nullptr};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oz",
												&object_zv, collection_find_class_entry,
												&raw_fields))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	util::zvalue fields(raw_fields);
	if (coll_find.fields(fields)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

static void
mysqlx_collection__find__add_sort_or_grouping(
	INTERNAL_FUNCTION_PARAMETERS,
	Collection_find::Operation op_type)
{
	DBG_ENTER("mysqlx_collection__find__add_sort_or_grouping");

	util::raw_zval* object_zv{nullptr};
	util::arg_zvals sort_expressions;

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O+",
									&object_zv,
									collection_find_class_entry,
									&sort_expressions.data,
									&sort_expressions.counter))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.add_operation(op_type, sort_expressions)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, sort)
{
	DBG_ENTER("mysqlx_collection__find::sort");
	mysqlx_collection__find__add_sort_or_grouping(
		INTERNAL_FUNCTION_PARAM_PASSTHRU,
		Collection_find::Operation::Sort);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, groupBy)
{
	DBG_ENTER("mysqlx_collection__find::groupBy");
	mysqlx_collection__find__add_sort_or_grouping(
		INTERNAL_FUNCTION_PARAM_PASSTHRU,
		Collection_find::Operation::Group_by);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, having)
{
	DBG_ENTER("mysqlx_collection__find::having");

	util::raw_zval* object_zv{nullptr};
	util::arg_string search_condition;

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, collection_find_class_entry,
												&(search_condition.str), &(search_condition.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.having(search_condition.to_view())) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, limit)
{
	DBG_ENTER("mysqlx_collection__find::limit");

	util::raw_zval* object_zv{nullptr};
	zend_long rows{0};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, collection_find_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.limit(rows)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, offset)
{
	DBG_ENTER("mysqlx_collection__find::offset");

	util::raw_zval* object_zv{nullptr};
	zend_long position{0};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, collection_find_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.offset(position)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, bind)
{
	DBG_ENTER("mysqlx_collection__find::bind");

	util::raw_zval* object_zv{nullptr};
	util::raw_zval* bind_vars{nullptr};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oz",
												&object_zv, collection_find_class_entry,
												&bind_vars))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	util::zvalue bind_variables(bind_vars);
	if (coll_find.bind(bind_variables)) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, lockShared)
{
	DBG_ENTER("mysqlx_collection__find::lockShared");

	util::raw_zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
		&object_zv, collection_find_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.lock_shared(static_cast<int>(lock_waiting_option))) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, lockExclusive)
{
	DBG_ENTER("mysqlx_collection__find::lockExclusive");

	util::raw_zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
		&object_zv, collection_find_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	if (coll_find.lock_exclusive(static_cast<int>(lock_waiting_option))) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__find, execute)
{
	DBG_ENTER("mysqlx_collection__find::execute");

	util::raw_zval* object_zv{nullptr};
	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
												&object_zv, collection_find_class_entry,
												&flags))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.execute(flags).move_to(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_collection__find_methods[] = {
	PHP_ME(mysqlx_collection__find, __construct, arginfo_mysqlx_collection__find__construct, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_collection__find, fields, arginfo_mysqlx_collection__find__fields, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, groupBy, arginfo_mysqlx_collection__find__group_by, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, having, arginfo_mysqlx_collection__find__having, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, bind, arginfo_mysqlx_collection__find__bind, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, sort, arginfo_mysqlx_collection__find__sort, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, limit, arginfo_mysqlx_collection__find__limit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, offset, arginfo_mysqlx_collection__find__offset, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, lockShared, arginfo_mysqlx_collection__find__lock_shared, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, lockExclusive, arginfo_mysqlx_collection__find__lock_exclusive, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__find, execute, arginfo_mysqlx_collection__find__execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers collection_find_handlers;
static HashTable collection_find_properties;

const st_mysqlx_property_entry collection_find_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_collection__find_free_storage(zend_object* object)
{
	util::free_object<Collection_find>(object);
}

static zend_object *
php_mysqlx_collection__find_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__find_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_find>(
		class_type,
		&collection_find_handlers,
		&collection_find_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_collection__find_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_find_class_entry,
		"CollectionFind",
		mysqlx_std_object_handlers,
		collection_find_handlers,
		php_mysqlx_collection__find_object_allocator,
		mysqlx_collection__find_free_storage,
		mysqlx_collection__find_methods,
		collection_find_properties,
		collection_find_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}

void
mysqlx_unregister_collection__find_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_find_properties);
}

util::zvalue
create_collection_find(
	const util::string_view& search_expression,
	drv::xmysqlnd_collection* collection)
{
	DBG_ENTER("create_collection_find");
	util::zvalue coll_find_obj;
	Collection_find& coll_find{ util::init_object<Collection_find>(collection_find_class_entry, coll_find_obj) };
	if (!coll_find.init(collection, search_expression)) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
	}
	DBG_RETURN(coll_find_obj);
}

Mysqlx::Crud::Find* get_stmt_from_collection_find(util::raw_zval* object_zv)
{
	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	return coll_find.get_stmt();
}

} // namespace devapi

} // namespace mysqlx
