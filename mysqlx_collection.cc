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
#include "xmysqlnd/xmysqlnd_index_collection_commands.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_collection__add.h"
#include "mysqlx_collection__find.h"
#include "mysqlx_collection__modify.h"
#include "mysqlx_collection__remove.h"
#include "mysqlx_collection_index.h"
#include "mysqlx_collection.h"
#include "mysqlx_doc_result.h"
#include "mysqlx_schema.h"
#include "mysqlx_session.h"
#include "util/allocator.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/zend_utils.h"
#include <vector>

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace {

zend_class_entry* mysqlx_collection_class_entry;

} // anonymous namespace

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
/************************************** INHERITED END   ****************************************/

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, json)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__find, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__remove, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


// single doc ops
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection_get_one, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection_replace_one, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, doc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection_add_or_replace_one, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, doc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection_remove_one, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


// index ops
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__create_index, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, index_name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, index_desc_json, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__drop_index, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, index_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()



struct st_mysqlx_collection : public util::custom_allocable
{
	~st_mysqlx_collection()
	{
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
	xmysqlnd_collection* collection;
};


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

/************************************** INHERITED START ****************************************/
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getSession)
{
	DBG_ENTER("mysqlx_collection::getSession");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data,
		getThis(),
		"O",
		&object_zv,
		mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	XMYSQLND_SESSION session{ data_object.collection->get_schema()->get_session()};
	mysqlx_new_session(return_value, session);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getName)
{
	DBG_ENTER("mysqlx_collection::getName");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };

	RETVAL_STRINGL(data_object.collection->get_name().c_str(), data_object.collection->get_name().length());

	DBG_VOID_RETURN;
}

static const enum_hnd_func_status
mysqlx_collection_on_error(void * /*context*/, XMYSQLND_SESSION session,
					xmysqlnd_stmt* const /*stmt*/,
					const unsigned int code,
					const util::string_view& sql_state,
					const util::string_view& message)
{
	DBG_ENTER("mysqlx_collection_on_error");
	const unsigned int UnknownDatabaseCode{1049};
	if (code == UnknownDatabaseCode) {
		DBG_RETURN(HND_PASS);
	} else {
		mysqlx_new_exception(code, sql_state, message);
		DBG_RETURN(HND_PASS_RETURN_FAIL);
	}
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, existsInDatabase)
{
	DBG_ENTER("mysqlx_collection::existsInDatabase");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_collection_on_error, nullptr };
	data_object.collection->exists_in_database( on_error, return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, count)
{
	DBG_ENTER("mysqlx_collection::count");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_collection_on_error, nullptr };
	util::zvalue counter;
	if (PASS == data_object.collection->count(on_error, counter.ptr())) {
		counter.copy_to(return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getSchema)
{
	DBG_ENTER("mysqlx_collection::getSchema");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(
				execute_data,
				getThis(), "O",
				&object_zv,
				mysqlx_collection_class_entry)) {
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	XMYSQLND_SESSION session;
	auto& coll = data_object.collection;
	if (coll->get_schema() ) {
		session = coll->get_schema()->get_session();
	}

	if(session != nullptr) {
		const util::string& schema_name{ coll->get_schema()->get_name() };
		xmysqlnd_schema* schema = session->create_schema_object(schema_name);
		if (schema) {
			mysqlx_new_schema(return_value, schema);
		} else {
			RAISE_EXCEPTION(10001,"Invalid object of class schema");
		}
	}

	DBG_VOID_RETURN;
}

/************************************** INHERITED END   ****************************************/


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, add)
{
	DBG_ENTER("mysqlx_collection::add");
	zval* object_zv{nullptr};
	zval* docs{nullptr};
	int num_of_docs{0};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
												&object_zv,
												mysqlx_collection_class_entry,
												&docs,
												&num_of_docs))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	mysqlx_new_collection__add(return_value, data_object.collection,
									docs,
									num_of_docs);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, find)
{
	DBG_ENTER("mysqlx_collection::find");
	zval* object_zv{nullptr};
	util::param_string search_expr;
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O|s",
												&object_zv, mysqlx_collection_class_entry,
												&(search_expr.str), &(search_expr.len)))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	mysqlx_new_collection__find(return_value, search_expr.to_view(), data_object.collection);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, modify)
{
	DBG_ENTER("mysqlx_collection::modify");
	zval* object_zv{nullptr};
	util::param_string search_expr;
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::modify_fail);
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	mysqlx_new_collection__modify(return_value, search_expr.to_view(), data_object.collection);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, remove)
{
	DBG_ENTER("mysqlx_collection::remove");
	zval* object_zv{nullptr};
	util::param_string search_expr;
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::remove_fail);
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };
	mysqlx_new_collection__remove(return_value, search_expr.to_view(), data_object.collection);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getOne)
{
	DBG_ENTER("mysqlx_collection::getOne");

	zval* object_zv{nullptr};
	util::param_string id;

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&id.str, &id.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);

	Collection_find coll_find;
	const char* Get_one_search_expression = "_id = :id";
	if (!coll_find.init(data_object.collection, Get_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	util::zvalue bind_variables{{"id", id.to_view()}};
	if (coll_find.bind(bind_variables)) {
		coll_find.execute(return_value);
		fetch_one_from_doc_result(return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, replaceOne)
{
	DBG_ENTER("mysqlx_collection::replaceOne");

	zval* object_zv{nullptr};
	util::param_string id;
	zval* doc{nullptr};

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Osz",
		&object_zv, mysqlx_collection_class_entry,
		&id.str, &id.len,
		&doc))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);

	Collection_modify coll_modify;
	const char* Replace_one_search_expression = "$._id = :id";
	if (!coll_modify.init(data_object.collection, Replace_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	util::zvalue bind_variables{{"id", id.to_view()}};
	if (!coll_modify.bind(bind_variables)) {
		DBG_VOID_RETURN;
	}

	const util::string_view Doc_root_path("$");
	util::zvalue doc_with_id(util::json::ensure_doc_id(doc, id.to_view()));
	if (coll_modify.set(Doc_root_path, doc_with_id.ptr())) {
		coll_modify.execute(return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, addOrReplaceOne)
{
	zval* object_zv{nullptr};
	util::param_string id;
	zval* doc{nullptr};

	DBG_ENTER("mysqlx_collection::addOrReplaceOne");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Osz",
		&object_zv, mysqlx_collection_class_entry,
		&id.str, &id.len,
		&doc))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);

	Collection_add coll_add;
	util::zvalue doc_with_id(util::json::ensure_doc_id(doc, id.to_view()));
	if (coll_add.add_docs(data_object.collection, id.to_view(), doc_with_id.ptr())) {
		coll_add.execute(return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, removeOne)
{
	zval* object_zv{nullptr};
	util::param_string id;

	DBG_ENTER("mysqlx_collection::removeOne");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&id.str, &id.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);
	Collection_remove coll_remove;
	const char* Remove_one_search_expression = "_id = :id";
	if (!coll_remove.init(data_object.collection, Remove_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	util::zvalue bind_variables{{"id", id.to_view()}};
	if (coll_remove.bind(bind_variables)) {
		coll_remove.execute(return_value);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, createIndex)
{
	zval* object_zv{nullptr};
	util::param_string index_name;
	util::param_string index_desc_json;

	DBG_ENTER("mysqlx_collection::createIndex");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Oss",
		&object_zv, mysqlx_collection_class_entry,
		&index_name.str, &index_name.len,
		&index_desc_json.str, &index_desc_json.len))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);
	create_collection_index(data_object.collection, index_name.to_view(), index_desc_json.to_view(), return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, dropIndex)
{
	zval* object_zv{nullptr};
	util::param_string index_name;

	DBG_ENTER("mysqlx_collection::dropIndex");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(index_name.str), &(index_name.len)))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);
	drop_collection_index(data_object.collection, index_name.to_view(), return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_collection_methods[] = {
	PHP_ME(mysqlx_collection, __construct,		nullptr,												ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_collection, getSession,		arginfo_mysqlx_collection__get_session,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, getName,			arginfo_mysqlx_collection__get_name,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, existsInDatabase,arginfo_mysqlx_collection__exists_in_database,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, count,			arginfo_mysqlx_collection__count,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection, getSchema,		arginfo_mysqlx_collection__get_schema,			ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/

	PHP_ME(mysqlx_collection, add, 	arginfo_mysqlx_collection__add,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, find, 	arginfo_mysqlx_collection__find,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, modify,	arginfo_mysqlx_collection__modify, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, remove,	arginfo_mysqlx_collection__remove,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection, getOne, arginfo_mysqlx_collection_get_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, replaceOne, arginfo_mysqlx_collection_replace_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, addOrReplaceOne, arginfo_mysqlx_collection_add_or_replace_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, removeOne, arginfo_mysqlx_collection_remove_one, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection, createIndex,	arginfo_mysqlx_collection__create_index,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection, dropIndex,	arginfo_mysqlx_collection__drop_index,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zval *
mysqlx_collection_property__name(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_collection* object = (const st_mysqlx_collection* ) (obj->ptr);
	DBG_ENTER("mysqlx_collection_property__name");
	if (object->collection && !object->collection->get_name().empty()) {
		ZVAL_STRINGL(return_value, object->collection->get_name().data(), object->collection->get_name().length());
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

static zend_object_handlers mysqlx_object_collection_handlers;
static HashTable mysqlx_collection_properties;

const st_mysqlx_property_entry mysqlx_collection_property_entries[] =
{
	{std::string_view("name"), mysqlx_collection_property__name,	nullptr},
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_collection_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_collection>(object);
}

static zend_object *
php_mysqlx_collection_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_collection>(
		class_type,
		&mysqlx_object_collection_handlers,
		&mysqlx_collection_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_collection_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_collection_class_entry,
		"Collection",
		mysqlx_std_object_handlers,
		mysqlx_object_collection_handlers,
		php_mysqlx_collection_object_allocator,
		mysqlx_collection_free_storage,
		mysqlx_collection_methods,
		mysqlx_collection_properties,
		mysqlx_collection_property_entries,
		mysqlx_schema_object_interface_entry);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_collection_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_collection_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_collection_properties);
}

void
mysqlx_new_collection(zval* return_value, xmysqlnd_collection* collection)
{
	DBG_ENTER("mysqlx_new_collection");

	st_mysqlx_collection& data_object{
		util::init_object<st_mysqlx_collection>(mysqlx_collection_class_entry, return_value) };
	data_object.collection = collection;

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
