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
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_table.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_database_object.h"
#include "mysqlx_exception.h"
#include "mysqlx_collection.h"
#include "mysqlx_table.h"
#include "mysqlx_schema.h"
#include "mysqlx_session.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/functions.h"

namespace mysqlx {

namespace devapi {


namespace {

using namespace drv;

static zend_class_entry *mysqlx_schema_class_entry;

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/************************************** INHERITED END   ****************************************/


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__create_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, options, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema_modify_collection, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, options, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__drop_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_collections, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_table, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, table_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_tables, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_schema__get_collection_as_table, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


struct st_mysqlx_schema : public util::custom_allocable
{
	~st_mysqlx_schema();
	xmysqlnd_schema* schema;
};

st_mysqlx_schema::~st_mysqlx_schema()
{
	xmysqlnd_schema_free(schema, nullptr, nullptr);
	if (schema->get_counter() == 0) {
		mnd_efree(schema);
	}
}


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

/************************************** INHERITED START ****************************************/
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getSession)
{
	DBG_ENTER("mysqlx_schema::getSession");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(
		execute_data,
		getThis(),
		"O",
		&object_zv,
		mysqlx_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	XMYSQLND_SESSION session{ data_object.schema->get_session() };
	create_session(session).move_to(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getName)
{
	DBG_ENTER("mysqlx_schema::getName");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	const auto& schema_name{ data_object.schema->get_name() };
	RETVAL_STRINGL(schema_name.data(), schema_name.length());

	DBG_VOID_RETURN;
}

static const enum_hnd_func_status
mysqlx_scheme_on_error(
	void* /*context*/,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt* const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_scheme_on_error");
	create_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, existsInDatabase)
{
	DBG_ENTER("mysqlx_schema::existsInDatabase");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	xmysqlnd_schema* schema = data_object.schema;
	const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_scheme_on_error, nullptr };
	schema->exists_in_database(on_error, return_value);

	DBG_VOID_RETURN;
}

/************************************** INHERITED END   ****************************************/

static const enum_hnd_func_status
mysqlx_schema_on_error(
	void* /*context*/,
	const xmysqlnd_schema* const /*schema*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_schema_on_error");
	create_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

const enum_hnd_func_status on_drop_db_object_error(
	void* /*context*/,
	const xmysqlnd_schema * const /*schema*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	throw util::xdevapi_exception(code, sql_state, message);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, createCollection)
{
	DBG_ENTER("mysqlx_schema::createCollection");

	util::raw_zval* object_zv{nullptr};
	util::arg_string collection_name;
	const util::arg_string Empty_collection_options = "{}";
	util::arg_string collection_options(Empty_collection_options);
	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Os|s",
		&object_zv, mysqlx_schema_class_entry,
		&(collection_name.str), &(collection_name.len),
		&collection_options.str, &collection_options.len))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_NULL();

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	if (!collection_name.empty()) {
		const st_xmysqlnd_schema_on_error_bind on_error{ mysqlx_schema_on_error, nullptr };
		xmysqlnd_collection* const collection{
			data_object.schema->create_collection(
				collection_name.to_view(),
				collection_options.to_view(),
				on_error) };
		DBG_INF_FMT("collection=%p", collection);
		if (collection) {
			create_collection(collection).move_to(return_value);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, modifyCollection)
{
	DBG_ENTER("mysqlx_schema::modifyCollection");

	util::raw_zval* object_zv{nullptr};
	util::arg_string collection_name;
	util::arg_string collection_options;
	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Oss",
		&object_zv, mysqlx_schema_class_entry,
		&collection_name.str, &collection_name.len,
		&collection_options.str, &collection_options.len))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	if (!collection_name.empty()) {
		const st_xmysqlnd_schema_on_error_bind on_error{ mysqlx_schema_on_error, nullptr };
		if (data_object.schema->modify_collection(
			collection_name.to_view(),
			collection_options.to_view(),
			on_error)) {
			RETVAL_TRUE;
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, dropCollection)
{
	DBG_ENTER("mysqlx_schema::dropCollection");

	util::raw_zval* object_zv{nullptr};
	util::arg_string collection_name;
	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_schema_class_entry,
		&collection_name.str, &collection_name.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_schema>(object_zv);

	try {
		const st_xmysqlnd_schema_on_error_bind on_error{ on_drop_db_object_error, nullptr };
		RETVAL_BOOL(PASS == data_object.schema->drop_collection(collection_name.to_view(), on_error));
	} catch(std::exception& e) {
		util::log_warning(e.what());
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getCollection)
{
	DBG_ENTER("mysqlx_schema::getCollection");

	util::raw_zval* object_zv{nullptr};
	util::arg_string collection_name;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, mysqlx_schema_class_entry,
												&(collection_name.str), &(collection_name.len)))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	RETVAL_NULL();
	if ( !collection_name.empty() && data_object.schema) {
		xmysqlnd_collection* const collection = data_object.schema->create_collection_object(collection_name.to_view());
		if (collection) {
			create_collection(collection).move_to(return_value);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getTable)
{
	DBG_ENTER("mysqlx_schema::getTable");

	util::raw_zval* object_zv{nullptr};
	util::arg_string table_name;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, mysqlx_schema_class_entry,
												&(table_name.str), &(table_name.len)))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	RETVAL_NULL();
	if ( !table_name.empty() && data_object.schema) {
		xmysqlnd_table* const table = data_object.schema->create_table_object(table_name.to_view());
		create_table(table).move_to(return_value);
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getCollectionAsTable)
{
	DBG_ENTER("mysqlx_schema::getCollectionAsTable");

	util::raw_zval* object_zv{nullptr};
	util::arg_string collection_name;
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "Os",
												&object_zv, mysqlx_schema_class_entry,
												&(collection_name.str), &(collection_name.len)))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	RETVAL_NULL();
	if (!collection_name.empty() && data_object.schema) {
		xmysqlnd_table* const table = data_object.schema->create_table_object(collection_name.to_view());
		create_table(table).move_to(return_value);
	}
	DBG_VOID_RETURN;
}

struct st_mysqlx_on_db_object_ctx
{
	util::zvalue* list;
};

static void
mysqlx_on_db_object(void* context, xmysqlnd_schema* const schema, const util::string_view& object_name, const util::string_view& object_type)
{
	DBG_ENTER("mysqlx_on_db_object");

	st_mysqlx_on_db_object_ctx* ctx = static_cast<st_mysqlx_on_db_object_ctx*>(context);
	util::zvalue db_obj;

	if ((object_type[0] == 'T') || (object_type[0] == 'V')) {
		xmysqlnd_table* const table = schema->create_table_object(object_name);
		if (table) {
			db_obj = create_table(table);
			ctx->list->insert(object_name, db_obj);
		}
	} else if (object_type[0] == 'C') {
		xmysqlnd_collection* const collection = schema->create_collection_object(object_name);
		if (collection) {
			db_obj = create_collection(collection);
			ctx->list->insert(object_name, db_obj);
		}
	}

	DBG_VOID_RETURN;
}

static util::zvalue
mysqlx_get_database_objects(
	xmysqlnd_schema* schema,
	const db_object_type_filter object_type_filter)
{
	DBG_ENTER("mysqlx_get_database_objects");
	util::zvalue list = util::zvalue::create_array();
	if (schema) {
		st_mysqlx_on_db_object_ctx context{ &list };
		const st_xmysqlnd_schema_on_database_object_bind on_object{ mysqlx_on_db_object, &context };
		const st_xmysqlnd_schema_on_error_bind handler_on_error{ mysqlx_schema_on_error, nullptr };

		if (PASS != schema->get_db_objects(schema->get_name(), object_type_filter, on_object, handler_on_error)) {
			list.clear();
		}
	}
	DBG_RETURN(list);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getTables)
{
	DBG_ENTER("mysqlx_schema::getTables");

	util::raw_zval* object_zv{nullptr};
	if (util::get_method_arguments(execute_data, getThis(), "O", &object_zv, mysqlx_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	mysqlx_get_database_objects(data_object.schema, drv::db_object_type_filter::table_or_view).move_to(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_schema, getCollections)
{
	DBG_ENTER("mysqlx_schema::getCollections");

	util::raw_zval* object_zv{nullptr};
	if (util::get_method_arguments(execute_data, getThis(), "O", &object_zv, mysqlx_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_schema>(object_zv) };
	mysqlx_get_database_objects(data_object.schema, drv::db_object_type_filter::collection).move_to(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_schema_methods[] = {
	PHP_ME(mysqlx_schema, __construct, arginfo_mysqlx_schema__construct, ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_schema, getSession, arginfo_mysqlx_schema__get_session, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, getName, arginfo_mysqlx_schema__get_name, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, existsInDatabase, arginfo_mysqlx_schema__exists_in_database, ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/
	PHP_ME(mysqlx_schema, createCollection, arginfo_mysqlx_schema__create_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, modifyCollection, arginfo_mysqlx_schema_modify_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, dropCollection, arginfo_mysqlx_schema__drop_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, getCollection, arginfo_mysqlx_schema__get_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, getCollections, arginfo_mysqlx_schema__get_collections, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_schema, getTable, arginfo_mysqlx_schema__get_table, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, getTables, arginfo_mysqlx_schema__get_tables, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_schema, getCollectionAsTable,arginfo_mysqlx_schema__get_collection_as_table, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static util::raw_zval*
mysqlx_schema_property__name(const st_mysqlx_object* obj, util::raw_zval* return_value)
{
	const st_mysqlx_schema* object = static_cast<const st_mysqlx_schema*>(obj->ptr);
	DBG_ENTER("mysqlx_schema_property__name");
	if (object->schema && !object->schema->get_name().empty()) {
		ZVAL_STRINGL(return_value, object->schema->get_name().data(), object->schema->get_name().length());
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

static zend_object_handlers mysqlx_object_schema_handlers;
static HashTable mysqlx_schema_properties;

const st_mysqlx_property_entry mysqlx_schema_property_entries[] =
{
	{std::string_view("name"), mysqlx_schema_property__name,	nullptr},
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_schema_free_storage(zend_object* object)
{
	util::free_object<st_mysqlx_schema>(object);
}

static zend_object *
php_mysqlx_schema_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_schema_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_schema>(
		class_type,
		&mysqlx_object_schema_handlers,
		&mysqlx_schema_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

} // anonymous namespace

void
mysqlx_register_schema_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_schema_class_entry,
		"Schema",
		mysqlx_std_object_handlers,
		mysqlx_object_schema_handlers,
		php_mysqlx_schema_object_allocator,
		mysqlx_schema_free_storage,
		mysqlx_schema_methods,
		mysqlx_schema_properties,
		mysqlx_schema_property_entries,
		mysqlx_database_object_interface_entry);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_schema_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_schema_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_schema_properties);
}

util::zvalue
create_schema(xmysqlnd_schema* schema)
{
	DBG_ENTER("create_schema");

	util::zvalue schema_obj;
	st_mysqlx_schema& data_object{
		util::init_object<st_mysqlx_schema>(mysqlx_schema_class_entry, schema_obj) };
	data_object.schema = schema;

	DBG_RETURN(schema_obj);
}

} // namespace devapi

} // namespace mysqlx
