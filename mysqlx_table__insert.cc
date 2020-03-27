/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_table__insert_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert__values, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, row_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table__insert : public util::custom_allocable
{
	XMYSQLND_CRUD_TABLE_OP__INSERT * crud_op;
	xmysqlnd_table * table;
};


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, values)
{
	DBG_ENTER("mysqlx_table__insert::values");

	zval* object_zv{nullptr};
	zval* values{nullptr};
	zend_bool op_failed{FALSE};
	int num_of_values{0};
	if (FAILURE == util::zend::parse_method_parameters(
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
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__insert, execute)
{
	DBG_ENTER("mysqlx_table__insert::execute");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
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
				zval stmt_zv;
				ZVAL_UNDEF(&stmt_zv);
				mysqlx_new_stmt(&stmt_zv, stmt);
				if (Z_TYPE(stmt_zv) == IS_NULL) {
					xmysqlnd_stmt_free(stmt, nullptr, nullptr);
				}
				if (Z_TYPE(stmt_zv) == IS_OBJECT) {
					zval zv;
					ZVAL_UNDEF(&zv);
					zend_long flags{0};
					mysqlx_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv), flags, MYSQLX_RESULT, &zv);

					ZVAL_COPY(return_value, &zv);
					zval_dtor(&zv);
				}
				zval_ptr_dtor(&stmt_zv);
			}
		}
	}

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_table__insert_methods[] = {
	PHP_ME(mysqlx_table__insert, __construct,	nullptr,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_table__insert, values,		arginfo_mysqlx_table__insert__values,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__insert, execute,		arginfo_mysqlx_table__insert__execute,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

#if 0
static zval *
mysqlx_table__insert_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_table__insert* object = (const st_mysqlx_table__insert* ) (obj->ptr);
	DBG_ENTER("mysqlx_table__insert_property__name");
	if (object->table && object->table->get_name().s) {
		ZVAL_STRINGL(return_value, object->table->get_name().s, object->table->get_name().l);
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

const struct st_mysqlx_property_entry mysqlx_table__insert_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_table__insert_property__name,	nullptr},
#endif
	{{nullptr,	0}, nullptr, nullptr}
};

static void
mysqlx_table__insert_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_table__insert* inner_obj = (st_mysqlx_table__insert*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_table_free(inner_obj->table, nullptr, nullptr);
			inner_obj->table = nullptr;
		}
		if(inner_obj->crud_op) {
			xmysqlnd_crud_table_insert__destroy(inner_obj->crud_op);
			inner_obj->crud_op = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}

static zend_object *
php_mysqlx_table__insert_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_table__insert_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table__insert>(
		class_type,
		&mysqlx_object_table__insert_handlers,
		&mysqlx_table__insert_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_table__insert_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_table__insert_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_table__insert_handlers.free_obj = mysqlx_table__insert_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "TableInsert", mysqlx_table__insert_methods);
		tmp_ce.create_object = php_mysqlx_table__insert_object_allocator;
		mysqlx_table__insert_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_table__insert_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_table__insert_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_table__insert_properties, mysqlx_table__insert_property_entries);
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

void
mysqlx_new_table__insert(zval * return_value,
					xmysqlnd_table * table,
					const zend_bool clone,
					zval * columns,
					const int num_of_columns)
{
	DBG_ENTER("mysqlx_new_table__insert");

	if (SUCCESS == object_init_ex(return_value, mysqlx_table__insert_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_table__insert* const object = (st_mysqlx_table__insert*) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->get_reference() : table;
			object->crud_op = xmysqlnd_crud_table_insert__create(
				mnd_str2c(object->table->get_schema()->get_name()),
				mnd_str2c(object->table->get_name()),
				columns,
				num_of_columns);
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
