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
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_table.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_schema.h"
#include "mysqlx_session.h"
#include "mysqlx_table__delete.h"
#include "mysqlx_table__insert.h"
#include "mysqlx_table__select.h"
#include "mysqlx_table__update.h"
#include "mysqlx_table.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_table_class_entry;

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__is_view, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
/************************************** INHERITED END   ****************************************/

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, columns)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__insert, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_VARIADIC_INFO(no_pass_by_ref, columns)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__update, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table : public util::custom_allocable
{
	~st_mysqlx_table();
	xmysqlnd_table* table;
};

st_mysqlx_table::~st_mysqlx_table()
{
	xmysqlnd_table_free(table, nullptr, nullptr);
}


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

/************************************** INHERITED START ****************************************/
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, getSession)
{
	DBG_ENTER("mysqlx_table::getSession");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data,
		getThis(),
		"O",
		&object_zv,
		mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	XMYSQLND_SESSION session{ data_object.table->get_schema()->get_session() };
	mysqlx_new_session(return_value, session);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, getName)
{
	DBG_ENTER("mysqlx_table::getName");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_STRINGL(data_object.table->get_name().data(), data_object.table->get_name().length());

	DBG_VOID_RETURN;
}

static const enum_hnd_func_status
mysqlx_table_on_error(
	void * /*context*/,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt* const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_table_on_error");
	const unsigned int UnknownDatabaseCode{1049};
	if (code == UnknownDatabaseCode) {
		DBG_RETURN(HND_PASS);
	} else {
		mysqlx_new_exception(code, sql_state, message);
		DBG_RETURN(HND_PASS_RETURN_FAIL);
	}
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, existsInDatabase)
{
	DBG_ENTER("mysqlx_table::existsInDatabase");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	xmysqlnd_table* table = data_object.table;
	const st_xmysqlnd_session_on_error_bind on_error = { mysqlx_table_on_error, nullptr };
	zval exists;
	ZVAL_UNDEF(&exists);
	if (PASS == table->exists_in_database(on_error, &exists)) {
		ZVAL_COPY_VALUE(return_value, &exists);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, isView)
{
	DBG_ENTER("mysqlx_table::isView");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data,
		getThis(),
		"O",
		&object_zv,
		mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_table>(object_zv);

	RETVAL_FALSE;

	xmysqlnd_table * table = data_object.table;
	const st_xmysqlnd_session_on_error_bind on_error = { mysqlx_table_on_error, nullptr };
	zval exists;
	ZVAL_UNDEF(&exists);
	if (PASS == table->is_view(on_error, &exists)) {
		ZVAL_COPY_VALUE(return_value, &exists);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, count)
{
	DBG_ENTER("mysqlx_table::count");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };
	xmysqlnd_table* table{ data_object.table };
	if (table) {
		const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_table_on_error, nullptr };
		zval counter;
		ZVAL_UNDEF(&counter);
		if (PASS == table->count(on_error, &counter)) {
			ZVAL_COPY_VALUE(return_value, &counter);
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, getSchema)
{
	DBG_ENTER("mysqlx_table::getSchema");

	XMYSQLND_SESSION session;
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(
				execute_data,
				getThis(), "O",
				&object_zv,
				mysqlx_table_class_entry)) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };
	RETVAL_FALSE;

	if( data_object.table->get_schema() ) {
		session = data_object.table->get_schema()->get_session();
	}

	if(session != nullptr) {
		xmysqlnd_schema* schema = session->create_schema_object(
			data_object.table->get_schema()->get_name());
		if (schema) {
			mysqlx_new_schema(return_value, schema);
		} else {
			RAISE_EXCEPTION(10001,"Invalid object of class schema");
		}
	}

	DBG_VOID_RETURN;
}


/************************************** INHERITED END   ****************************************/


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, insert)
{
	DBG_ENTER("mysqlx_table::insert");

	zval* object_zv{nullptr};
	zval* columns{nullptr};
	int num_of_columns{0};
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "O+",
		&object_zv, mysqlx_table_class_entry,
		&columns,
		&num_of_columns))
	{
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_columns ; ++i ) {
		auto column_type{ Z_TYPE(columns[i]) };
		if (column_type != IS_STRING &&
			column_type != IS_OBJECT &&
			column_type != IS_ARRAY) {
			php_error_docref(
				nullptr,
				E_WARNING,
				"Only strings, objects and arrays can be added. Type is %u",
				column_type);
			DBG_VOID_RETURN;
		}
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	if (num_of_columns > 0) {
		mysqlx_new_table__insert(return_value,
									  data_object.table,
									  columns,
									  num_of_columns);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, select)
{
	DBG_ENTER("mysqlx_table::select");

	zval* object_zv{nullptr};
	zval* columns{nullptr};
	int num_of_columns{0};
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "O+",
		&object_zv, mysqlx_table_class_entry,
		&columns,
		&num_of_columns))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	if (columns) {
		DBG_INF_FMT("Num of columns: %d",
					num_of_columns);
		mysqlx_new_table__select(return_value,
						data_object.table,
						columns,
						num_of_columns);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, update)
{
	DBG_ENTER("mysqlx_table::update");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	mysqlx_new_table__update(return_value, data_object.table);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, delete)
{
	DBG_ENTER("mysqlx_table::delete");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_table>(object_zv) };

	RETVAL_FALSE;

	mysqlx_new_table__delete(return_value, data_object.table);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_table_methods[] = {
	PHP_ME(mysqlx_table, __construct,		nullptr,											ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_table, getSession,		arginfo_mysqlx_table__get_session,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, getName,			arginfo_mysqlx_table__get_name,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, isView, arginfo_mysqlx_table__is_view, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, existsInDatabase,	arginfo_mysqlx_table__exists_in_database,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, count,			arginfo_mysqlx_table__count,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_table, getSchema,		arginfo_mysqlx_table__get_schema,			ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/

	PHP_ME(mysqlx_table, insert, arginfo_mysqlx_table__insert, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, select, arginfo_mysqlx_table__select, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, update, arginfo_mysqlx_table__update, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table, delete, arginfo_mysqlx_table__delete, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zval *
mysqlx_table_property__name(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_table* object = (const st_mysqlx_table* ) (obj->ptr);
	DBG_ENTER("mysqlx_table_property__name");
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

static zend_object_handlers mysqlx_object_table_handlers;
static HashTable mysqlx_table_properties;

const st_mysqlx_property_entry mysqlx_table_property_entries[] =
{
	{std::string_view("name"), mysqlx_table_property__name,	nullptr},
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_table_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_table>(object);
}

static zend_object *
php_mysqlx_table_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_table_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table>(
		class_type,
		&mysqlx_object_table_handlers,
		&mysqlx_table_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_table_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_table_class_entry,
		"Table",
		mysqlx_std_object_handlers,
		mysqlx_object_table_handlers,
		php_mysqlx_table_object_allocator,
		mysqlx_table_free_storage,
		mysqlx_table_methods,
		mysqlx_table_properties,
		mysqlx_table_property_entries,
		mysqlx_schema_object_interface_entry);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_table_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table_properties);
}

void
mysqlx_new_table(zval* return_value, xmysqlnd_table* table)
{
	DBG_ENTER("mysqlx_new_table");
	st_mysqlx_table& data_object{
		util::init_object<st_mysqlx_table>(mysqlx_table_class_entry, return_value) };
	data_object.table = table;
	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
