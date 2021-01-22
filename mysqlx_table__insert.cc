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
#include "json_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_table.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_table__insert.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/functions.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_table__insert_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert__values, 0, ZEND_RETURN_VALUE, 1)
	MYSQL_XDEVAPI_ARG_VARIADIC_TYPE_INFO(no_pass_by_ref, row_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table__insert : public util::custom_allocable
{
	~st_mysqlx_table__insert();
	XMYSQLND_CRUD_TABLE_OP__INSERT* crud_op;
	xmysqlnd_table* table;
};

st_mysqlx_table__insert::~st_mysqlx_table__insert()
{
	xmysqlnd_table_free(table, nullptr, nullptr);
	xmysqlnd_crud_table_insert__destroy(crud_op);
}


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, values)
{
	DBG_ENTER("mysqlx_table__insert::values");

	util::raw_zval* object_zv{nullptr};
	zval* values{nullptr};
	zend_bool op_failed{FALSE};
	int num_of_values{0};
	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "O+",
		&object_zv,
		mysqlx_table__insert_class_entry,
		&values,
		&num_of_values))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__insert>(object_zv) };

	RETVAL_FALSE;

	DBG_INF_FMT("Num of values: %d",
				num_of_values);

	for(int i{0}; i < num_of_values ; ++i ) {
		if (FAIL == xmysqlnd_crud_table_insert__add_row(data_object.crud_op,
												&values[i])) {
			op_failed = TRUE;
			break;
		}
	}

	if( op_failed == FALSE ) {
		util::zvalue::copy_from_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, execute)
{
	DBG_ENTER("mysqlx_table__insert::execute");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_table__insert_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table__insert>(object_zv) };

	RETVAL_FALSE;

	DBG_INF_FMT("crud_op=%p table=%p", data_object.crud_op, data_object.table);
	if (data_object.crud_op && data_object.table) {
		if (FALSE == xmysqlnd_crud_table_insert__is_initialized(data_object.crud_op)) {
			RAISE_EXCEPTION(err_msg_insert_fail);
		} else {
			xmysqlnd_stmt* stmt = data_object.table->insert(data_object.crud_op);
			if (stmt) {
				util::zvalue stmt_obj = create_stmt(stmt);
				zend_long flags{0};
				mysqlx_statement_execute_read_response(Z_MYSQLX_P(stmt_obj.ptr()), flags, MYSQLX_RESULT).move_to(return_value);
			}
		}
	}

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_table__insert_methods[] = {
	PHP_ME(mysqlx_table__insert, __construct, arginfo_mysqlx_table__insert__construct, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_table__insert, values,		arginfo_mysqlx_table__insert__values,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__insert, execute,		arginfo_mysqlx_table__insert__execute,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

#if 0
static util::raw_zval*
mysqlx_table__insert_property__name(const st_mysqlx_object* obj, util::raw_zval* return_value)
{
	const st_mysqlx_table__insert* object = (const st_mysqlx_table__insert* ) (obj->ptr);
	DBG_ENTER("mysqlx_table__insert_property__name");
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

static zend_object_handlers mysqlx_object_table__insert_handlers;
static HashTable mysqlx_table__insert_properties;

const st_mysqlx_property_entry mysqlx_table__insert_property_entries[] =
{
#if 0
	{std::string_view("name"), mysqlx_table__insert_property__name,	nullptr},
#endif
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_table__insert_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_table__insert>(object);
}

static zend_object *
php_mysqlx_table__insert_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_table__insert_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table__insert>(
		class_type,
		&mysqlx_object_table__insert_handlers,
		&mysqlx_table__insert_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_table__insert_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_table__insert_class_entry,
		"TableInsert",
		mysqlx_std_object_handlers,
		mysqlx_object_table__insert_handlers,
		php_mysqlx_table__insert_object_allocator,
		mysqlx_table__insert_free_storage,
		mysqlx_table__insert_methods,
		mysqlx_table__insert_properties,
		mysqlx_table__insert_property_entries,
		mysqlx_executable_interface_entry);

#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table__insert_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}

void
mysqlx_unregister_table__insert_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table__insert_properties);
}

util::zvalue
create_table_insert(
	xmysqlnd_table* table,
	zval* columns,
	const int num_of_columns)
{
	DBG_ENTER("create_table_insert");
	util::zvalue table_insert_obj;
	st_mysqlx_table__insert& data_object{
		util::init_object<st_mysqlx_table__insert>(mysqlx_table__insert_class_entry, table_insert_obj) };
	data_object.table = table->get_reference();
	data_object.crud_op = xmysqlnd_crud_table_insert__create(
		data_object.table->get_schema()->get_name(),
		data_object.table->get_name(),
		columns,
		num_of_columns);
	DBG_RETURN(table_insert_obj);
}

} // namespace devapi

} // namespace mysqlx
