/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
	xmysqlnd_table * table;
};


#define MYSQLX_FETCH_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_table*) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_table::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_table::getSession() */
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
/* }}} */


/* {{{ proto mixed mysqlx_table::getName() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, getName)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::getName");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	if (object->table) {
		RETVAL_STRINGL(object->table->get_name().s, object->table->get_name().l);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_table_on_error */
static const enum_hnd_func_status
mysqlx_table_on_error(
	void * /*context*/,
	XMYSQLND_SESSION session,
	st_xmysqlnd_stmt* const /*stmt*/,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
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
/* }}} */


/* {{{ proto mixed mysqlx_table::existsInDatabase() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, existsInDatabase)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::existsInDatabase");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	xmysqlnd_table * table = object->table;
	if (table) {
		const struct st_xmysqlnd_session_on_error_bind on_error = { mysqlx_table_on_error, nullptr };
		zval exists;
		ZVAL_UNDEF(&exists);
		if (PASS == table->exists_in_database(on_error, &exists)) {
			ZVAL_COPY_VALUE(return_value, &exists);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::isView() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, isView)
{
	zval* object_zv{nullptr};

	RETVAL_FALSE;

	DBG_ENTER("mysqlx_table::isView");
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

	xmysqlnd_table * table = data_object.table;
	if (table) {
		const st_xmysqlnd_session_on_error_bind on_error = { mysqlx_table_on_error, nullptr };
		zval exists;
		ZVAL_UNDEF(&exists);
		if (PASS == table->is_view(on_error, &exists)) {
			ZVAL_COPY_VALUE(return_value, &exists);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::count() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, count)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::count");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	xmysqlnd_table * table = object->table;
	if (table) {
		const struct st_xmysqlnd_session_on_error_bind on_error = { mysqlx_table_on_error, nullptr };
		zval counter;
		ZVAL_UNDEF(&counter);
		if (PASS == table->count(on_error, &counter)) {
			ZVAL_COPY_VALUE(return_value, &counter);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::getSchema() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, getSchema)
{
	st_mysqlx_table* object{nullptr};
	XMYSQLND_SESSION session;
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::getSchema");

	if (FAILURE == util::zend::parse_method_parameters(
				execute_data,
				getThis(), "O",
				&object_zv,
				mysqlx_table_class_entry)) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	if( object->table &&
		object->table->get_schema() ) {
		session = object->table->get_schema()->get_session();
	}

	if(session != nullptr) {
		MYSQLND_STRING schema_name{ object->table->get_schema()->get_name() };
		xmysqlnd_schema * schema = session->create_schema_object(
					mnd_str2c(schema_name));
		if (schema) {
			mysqlx_new_schema(return_value, schema);
		} else {
			RAISE_EXCEPTION(10001,"Invalid object of class schema");
		}
	}

	DBG_VOID_RETURN;
}

/* }}} */
/************************************** INHERITED END   ****************************************/


/* {{{ proto mixed mysqlx_table::insert() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, insert)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};
	zval* columns{nullptr};
	int num_of_columns{0};

	DBG_ENTER("mysqlx_table::insert");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "O+",
		&object_zv, mysqlx_table_class_entry,
		&columns,
		&num_of_columns))
	{
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_columns ; ++i ) {
		if (Z_TYPE(columns[i]) != IS_STRING &&
			Z_TYPE(columns[i]) != IS_OBJECT &&
			Z_TYPE(columns[i]) != IS_ARRAY) {
			php_error_docref(nullptr, E_WARNING, "Only strings, objects and arrays can be added. Type is %u",
							 Z_TYPE(columns[i]));
			DBG_VOID_RETURN;
		}
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->table && num_of_columns > 0) {
		mysqlx_new_table__insert(return_value,
									  object->table,
									  TRUE /* clone */,
									  columns,
									  num_of_columns);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::select() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, select)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};
	zval* columns{nullptr};
	int num_of_columns{0};

	DBG_ENTER("mysqlx_table::select");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "O+",
		&object_zv, mysqlx_table_class_entry,
		&columns,
		&num_of_columns))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->table && columns) {
		DBG_INF_FMT("Num of columns: %d",
					num_of_columns);
		mysqlx_new_table__select(return_value,
						object->table,
						TRUE /* clone */,
						columns,
						num_of_columns);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::update() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, update)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::update");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->table) {
		mysqlx_new_table__update(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table::delete() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table, delete)
{
	st_mysqlx_table* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table::delete");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->table) {
		mysqlx_new_table__delete(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_table_methods[] */
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
/* }}} */


/* {{{ mysqlx_table_property__name */
static zval *
mysqlx_table_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_table* object = (const st_mysqlx_table* ) (obj->ptr);
	DBG_ENTER("mysqlx_table_property__name");
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
/* }}} */


static zend_object_handlers mysqlx_object_table_handlers;
static HashTable mysqlx_table_properties;

const struct st_mysqlx_property_entry mysqlx_table_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_table_property__name,	nullptr},
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_table_free_storage */
static void
mysqlx_table_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_table* inner_obj = (st_mysqlx_table*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_table_free(inner_obj->table, nullptr, nullptr);
			inner_obj->table = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_table_object_allocator */
static zend_object *
php_mysqlx_table_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_table_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table>(
		class_type,
		&mysqlx_object_table_handlers,
		&mysqlx_table_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_table_class */
void
mysqlx_register_table_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_table_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_table_handlers.free_obj = mysqlx_table_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Table", mysqlx_table_methods);
		tmp_ce.create_object = php_mysqlx_table_object_allocator;
		mysqlx_table_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_table_class_entry, 1, mysqlx_schema_object_interface_entry);
	}

	zend_hash_init(&mysqlx_table_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_table_properties, mysqlx_table_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_table_class */
void
mysqlx_unregister_table_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table_properties);
}
/* }}} */


/* {{{ mysqlx_new_table */
void
mysqlx_new_table(zval * return_value, xmysqlnd_table * table, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_table");

	if (SUCCESS == object_init_ex(return_value, mysqlx_table_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_table* const object = (st_mysqlx_table*) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->get_reference() : table;
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

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
