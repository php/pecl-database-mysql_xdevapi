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
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_table.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_crud_table_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_table__select.h"
#include "util/allocator.h"
#include "util/functions.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

zend_class_entry* mysqlx_table__select_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__where, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, where_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__group_by, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__having, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__orderby, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__offset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__lock_shared, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__lock_exclusive, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table__select : public util::custom_allocable
{
	~st_mysqlx_table__select();
	XMYSQLND_CRUD_TABLE_OP__SELECT* crud_op;
	xmysqlnd_table* table;
};

st_mysqlx_table__select::~st_mysqlx_table__select()
{
	xmysqlnd_table_free(table, nullptr, nullptr);
	xmysqlnd_crud_table_select__destroy(crud_op);
}


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, where)
{
	DBG_ENTER("mysqlx_table__select::where");

	util::raw_zval* object_zv{nullptr};
	util::arg_string where_expr;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, mysqlx_table__select_class_entry,
												&where_expr.str, &where_expr.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	if (!where_expr.empty()) {
		if (PASS == xmysqlnd_crud_table_select__set_criteria(data_object.crud_op, where_expr.to_view())) {
			util::zvalue::copy_from_to(object_zv, return_value);
		}
	}
}

const unsigned int ADD_SORT = 1;
const unsigned int ADD_GROUPING = 2;

static void
mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAMETERS, const unsigned int op_type)
{
	DBG_ENTER("mysqlx_table__select__add_sort_or_grouping");

	util::raw_zval* object_zv{nullptr};
	zval* sort_expr{nullptr};
	int num_of_expr{0};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O+",
												&object_zv,
												mysqlx_table__select_class_entry,
												&sort_expr,
												&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_expr ; ++i ) {
		auto sort_expr_type{ Z_TYPE(sort_expr[i]) };
		if (sort_expr_type != IS_STRING &&
			sort_expr_type != IS_OBJECT &&
			sort_expr_type != IS_ARRAY) {
			php_error_docref(
				nullptr,
				E_WARNING,
				"Only strings, objects and arrays can be added. Type is %u",
				sort_expr_type);
			DBG_VOID_RETURN;
		}
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	if (!( data_object.crud_op && sort_expr ) ) {
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i]))
		{
		case IS_STRING:
			{
				const util::string_view sort_expr_str{ Z_STRVAL(sort_expr[i]), Z_STRLEN(sort_expr[i]) };
				if (ADD_SORT == op_type) {
					if (PASS == xmysqlnd_crud_table_select__add_orderby(data_object.crud_op, sort_expr_str)) {
						util::zvalue::copy_from_to(object_zv, return_value);
					}
				} else if (ADD_GROUPING == op_type) {
					if (PASS == xmysqlnd_crud_table_select__add_grouping(data_object.crud_op, sort_expr_str)) {
						util::zvalue::copy_from_to(object_zv, return_value);
					}
				}
				break;
			}
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				enum_func_status ret{FAIL};
				MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					ret = FAIL;
					const util::string_view sort_expr_str{ Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (ADD_SORT == op_type) {
						ret = xmysqlnd_crud_table_select__add_orderby(data_object.crud_op, sort_expr_str);
					} else if (ADD_GROUPING == op_type) {
						ret = xmysqlnd_crud_table_select__add_grouping(data_object.crud_op, sort_expr_str);
					}
					if (FAIL == ret) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
						DBG_VOID_RETURN;
					}
				} ZEND_HASH_FOREACH_END();
				if( FAIL != ret ) {
					util::zvalue::copy_from_to(object_zv, return_value);
				}
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			break;
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, orderby)
{
	DBG_ENTER("mysqlx_table__select::orderby");
	mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_SORT);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, groupBy)
{
	DBG_ENTER("mysqlx_table__select::groupBy");
	mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_GROUPING);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, having)
{
	DBG_ENTER("mysqlx_table__select::having");

	util::raw_zval* object_zv{nullptr};
	util::arg_string search_condition;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, mysqlx_table__select_class_entry,
												&search_condition.str, &search_condition.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	if (data_object.crud_op && data_object.table) {
		if (PASS == xmysqlnd_crud_table_select__set_having(data_object.crud_op, search_condition.to_view())) {
			util::zvalue::copy_from_to(object_zv, return_value);
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, limit)
{
	DBG_ENTER("mysqlx_table__select::limit");

	util::raw_zval* object_zv{nullptr};
	zend_long rows;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, mysqlx_table__select_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	if (data_object.crud_op && data_object.table) {
		if (PASS == xmysqlnd_crud_table_select__set_limit(data_object.crud_op, rows)) {
			util::zvalue::copy_from_to(object_zv, return_value);
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, offset)
{
	DBG_ENTER("mysqlx_table__select::offset");

	util::raw_zval* object_zv{nullptr};
	zend_long position;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Ol",
												&object_zv, mysqlx_table__select_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	if (data_object.crud_op && data_object.table) {
		if (PASS == xmysqlnd_crud_table_select__set_offset(data_object.crud_op, position)) {
			util::zvalue::copy_from_to(object_zv, return_value);
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, bind)
{
	DBG_ENTER("mysqlx_table__select::bind");

	util::raw_zval* object_zv{nullptr};
	HashTable* bind_variables_ht;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Oh",
												&object_zv, mysqlx_table__select_class_entry,
												&bind_variables_ht))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_NULL();

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };
	if (data_object.crud_op && data_object.table) {
		util::zvalue bind_variables(bind_variables_ht);
		for (const auto& [key, value] : bind_variables) {
			if (key.is_string()) {
				if (FAIL == xmysqlnd_crud_table_select__bind_value(data_object.crud_op, key.to_string_view(), value)) {
					RAISE_EXCEPTION(err_msg_bind_fail);
					DBG_VOID_RETURN;
				}
			}
		}
		util::zvalue::copy_from_to(object_zv, return_value);
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, lockShared)
{
	DBG_ENTER("mysqlx_table__select::lockShared");

	util::raw_zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
		&object_zv, mysqlx_table__select_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	auto crud_op = data_object.crud_op;
	int waiting_option = static_cast<int>(lock_waiting_option);
	if ((xmysqlnd_crud_table_select__enable_lock_shared(crud_op) == PASS)
		&& (xmysqlnd_crud_table_select_set_lock_waiting_option(crud_op, waiting_option) == PASS))
	{
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, lockExclusive)
{
	DBG_ENTER("mysqlx_table__select::lockExclusive");

	util::raw_zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O|l",
		&object_zv, mysqlx_table__select_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	auto crud_op = data_object.crud_op;
	int waiting_option = static_cast<int>(lock_waiting_option);
	if ((xmysqlnd_crud_table_select__enable_lock_exclusive(crud_op) == PASS)
		&& (xmysqlnd_crud_table_select_set_lock_waiting_option(crud_op, waiting_option) == PASS))
	{
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, execute)
{
	DBG_ENTER("mysqlx_table__select::execute");

	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};
	util::raw_zval* object_zv{nullptr};

	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_table__select_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__select>(object_zv) };

	RETVAL_FALSE;

	xmysqlnd_crud_table_select_verify_is_initialized(data_object.crud_op);

	xmysqlnd_stmt* stmt{ data_object.table->select(data_object.crud_op) };
	if (stmt) {
		util::zvalue stmt_obj = create_stmt(stmt);
		mysqlx_statement_execute_read_response(Z_MYSQLX_P(stmt_obj.ptr()), flags, MYSQLX_RESULT_ROW).move_to(return_value);
	}

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_table__select_methods[] = {
	PHP_ME(mysqlx_table__select, __construct, arginfo_mysqlx_table__select__construct, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_table__select, where, arginfo_mysqlx_table__select__where, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, groupBy, arginfo_mysqlx_table__select__group_by, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, having, arginfo_mysqlx_table__select__having, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, bind, arginfo_mysqlx_table__select__bind, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, orderby, arginfo_mysqlx_table__select__orderby, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, limit, arginfo_mysqlx_table__select__limit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, offset, arginfo_mysqlx_table__select__offset, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, lockShared, arginfo_mysqlx_table__select__lock_shared, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, lockExclusive, arginfo_mysqlx_table__select__lock_exclusive, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, execute, arginfo_mysqlx_table__select__execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

#if 0
static util::raw_zval*
mysqlx_table__select_property__name(const st_mysqlx_object* obj, util::raw_zval* return_value)
{
	const st_mysqlx_table__select* object = (const st_mysqlx_table__select* ) (obj->ptr);
	DBG_ENTER("mysqlx_table__select_property__name");
	if (object->table && !object->table->get_name().empty()) {
		ZVAL_STRINGL(return_value, object->table->get_name().data(), object->table->get_name().length());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}

#endif

static zend_object_handlers mysqlx_object_table__select_handlers;
static HashTable mysqlx_table__select_properties;

const st_mysqlx_property_entry mysqlx_table__select_property_entries[] =
{
#if 0
	{std::string_view("name"), mysqlx_table__select_property__name,	nullptr},
#endif
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_table__select_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_table__select>(object);
}

static zend_object *
php_mysqlx_table__select_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_table__select_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table__select>(
		class_type,
		&mysqlx_object_table__select_handlers,
		&mysqlx_table__select_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_table__select_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_table__select_class_entry,
		"TableSelect",
		mysqlx_std_object_handlers,
		mysqlx_object_table__select_handlers,
		php_mysqlx_table__select_object_allocator,
		mysqlx_table__select_free_storage,
		mysqlx_table__select_methods,
		mysqlx_table__select_properties,
		mysqlx_table__select_property_entries,
		mysqlx_executable_interface_entry);

#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table__select_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}

void
mysqlx_unregister_table__select_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table__select_properties);
}

util::zvalue
create_table_select(
	xmysqlnd_table* table,
	zval* columns,
	const int num_of_columns)
{
	DBG_ENTER("create_table_select");
	util::zvalue table_select_obj;
	st_mysqlx_table__select& data_object{
		util::init_object<st_mysqlx_table__select>(mysqlx_table__select_class_entry, table_select_obj) };
	data_object.table = table->get_reference();
	data_object.crud_op = xmysqlnd_crud_table_select__create(
		data_object.table->get_schema()->get_name(),
		data_object.table->get_name(),
		columns,
		num_of_columns);
	DBG_RETURN(table_select_obj);
}

Mysqlx::Crud::Find* get_stmt_from_table_select(zval* object_zv)
{
	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	XMYSQLND_CRUD_TABLE_OP__SELECT* select_op = data_object.crud_op;
	if (!select_op
		|| (xmysqlnd_crud_table_select__finalize_bind(select_op) == FAIL)
		|| !xmysqlnd_crud_table_select__is_initialized(select_op))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
	}

	st_xmysqlnd_pb_message_shell msg_shell = xmysqlnd_crud_table_select__get_protobuf_message(select_op);
	Mysqlx::Crud::Find* msg = static_cast<Mysqlx::Crud::Find*>(msg_shell.message);
	return msg;
}

} // namespace devapi

} // namespace mysqlx
