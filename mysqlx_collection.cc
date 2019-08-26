/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "util/hash_table.h"
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
	xmysqlnd_collection * collection;
};


#define MYSQLX_FETCH_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_collection*) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_collection::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_collection::getSession() */
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

	auto& data_object{ util::fetch_data_object<st_mysqlx_collection>(object_zv) };

	RETVAL_FALSE;

	XMYSQLND_SESSION session{ data_object.collection->get_schema()->get_session()};
	mysqlx_new_session(return_value, session);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::getName() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getName)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_collection::getName");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	if (object->collection) {
		RETVAL_STRINGL(object->collection->get_name().s, object->collection->get_name().l);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_collection_on_error */
static const enum_hnd_func_status
mysqlx_collection_on_error(void * /*context*/, XMYSQLND_SESSION session,
					xmysqlnd_stmt* const /*stmt*/,
					const unsigned int code,
					const MYSQLND_CSTRING sql_state,
					const MYSQLND_CSTRING message)
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
/* }}} */


/* {{{ proto mixed mysqlx_collection::existsInDatabase() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, existsInDatabase)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_collection::existsInDatabase");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	xmysqlnd_collection * collection = object->collection;
	if (collection) {
		const struct st_xmysqlnd_session_on_error_bind on_error = { mysqlx_collection_on_error, nullptr };
		zval exists;
		ZVAL_UNDEF(&exists);
		if (PASS == collection->exists_in_database( on_error, &exists)) {
			ZVAL_COPY_VALUE(return_value, &exists);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::count() */
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
	xmysqlnd_collection* collection{ data_object.collection };
	if (collection) {
		const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_collection_on_error, nullptr };
		zval counter;
		ZVAL_UNDEF(&counter);
		if (PASS == collection->count(on_error, &counter)) {
			ZVAL_COPY_VALUE(return_value, &counter);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::getSchema() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getSchema)
{
	st_mysqlx_collection* object{nullptr};
	XMYSQLND_SESSION session;
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_collection::getSchema");

	if (FAILURE == util::zend::parse_method_parameters(
				execute_data,
				getThis(), "O",
				&object_zv,
				mysqlx_collection_class_entry)) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	if( object->collection &&
		object->collection->get_schema() ) {
		session = object->collection->get_schema()->get_session();
	}

	if(session != nullptr) {
		MYSQLND_STRING schema_name{ object->collection->get_schema()->get_name() };
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


/* {{{ proto mixed mysqlx_collection::add() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, add)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};
	zval* docs{nullptr};
	int num_of_docs{0};

	DBG_ENTER("mysqlx_collection::add");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
												&object_zv,
												mysqlx_collection_class_entry,
												&docs,
												&num_of_docs))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_collection__add(return_value, object->collection,
										docs,
										num_of_docs);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::find() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, find)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view search_expr;

	DBG_ENTER("mysqlx_collection::find");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O|s",
												&object_zv, mysqlx_collection_class_entry,
												&(search_expr.str), &(search_expr.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_collection__find(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::modify() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, modify)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view search_expr;

	DBG_ENTER("mysqlx_collection::modify");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::modify_fail);
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_collection__modify(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::remove() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, remove)
{
	st_mysqlx_collection* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view search_expr;

	DBG_ENTER("mysqlx_collection::remove");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::remove_fail);
	}

	MYSQLX_FETCH_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_collection__remove(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::getOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, getOne)
{
	DBG_ENTER("mysqlx_collection::getOne");

	zval* object_zv{nullptr};
	util::string_view id;

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
	if (!coll_find.init(object_zv, data_object.collection, Get_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	util::Hash_table bind_variables;
	bind_variables.insert("id", id);
	coll_find.bind(bind_variables.ptr(), return_value);
	if (Z_TYPE_P(return_value) == IS_FALSE) {
		DBG_VOID_RETURN;
	}

	coll_find.execute(return_value);

	fetch_one_from_doc_result(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::replaceOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, replaceOne)
{
	DBG_ENTER("mysqlx_collection::replaceOne");

	zval* object_zv{nullptr};
	util::string_view id;
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
	if (!coll_modify.init(object_zv, data_object.collection, Replace_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	util::Hash_table bind_variables;
	bind_variables.insert("id", id);
	coll_modify.bind(bind_variables.ptr(), return_value);
	if (Z_TYPE_P(return_value) == IS_FALSE) {
		DBG_VOID_RETURN;
	}

	const util::string_view Doc_root_path("$");
	zval doc_with_id;
	util::json::ensure_doc_id(doc, id, &doc_with_id);
	coll_modify.set(Doc_root_path, true, &doc_with_id, return_value);

	coll_modify.execute(return_value);
	zval_ptr_dtor(&doc_with_id);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::addOrReplaceOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, addOrReplaceOne)
{
	zval* object_zv{nullptr};
	util::string_view id;
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
	zval doc_with_id;
	util::json::ensure_doc_id(doc, id, &doc_with_id);
	if (!coll_add.add_docs(data_object.collection, id, &doc_with_id)) {
		DBG_VOID_RETURN;
	}

	coll_add.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::removeOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, removeOne)
{
	zval* object_zv{nullptr};
	util::string_view id;

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

	util::zvalue bind_variables(util::zvalue::create_array(1));
	bind_variables.insert("id", id);
	if (coll_remove.bind(bind_variables.ptr())) {
		coll_remove.execute(return_value);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::createIndex() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, createIndex)
{
	zval* object_zv{nullptr};
	util::string_view index_name;
	util::string_view index_desc_json;

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
	create_collection_index(data_object.collection, index_name, index_desc_json, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection::dropIndex() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection, dropIndex)
{
	zval* object_zv{nullptr};
	util::string_view index_name;

	DBG_ENTER("mysqlx_collection::dropIndex");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_collection_class_entry,
		&(index_name.str), &(index_name.len)))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_collection>(object_zv);
	drop_collection_index(data_object.collection, index_name, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_collection_methods[] */
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
/* }}} */


/* {{{ mysqlx_collection_property__name */
static zval *
mysqlx_collection_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_collection* object = (const st_mysqlx_collection* ) (obj->ptr);
	DBG_ENTER("mysqlx_collection_property__name");
	if (object->collection && object->collection->get_name().s) {
		ZVAL_STRINGL(return_value, object->collection->get_name().s, object->collection->get_name().l);
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


static zend_object_handlers mysqlx_object_collection_handlers;
static HashTable mysqlx_collection_properties;

const struct st_mysqlx_property_entry mysqlx_collection_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_collection_property__name,	nullptr},
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_collection_free_storage */
static void
mysqlx_collection_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_collection* inner_obj = (st_mysqlx_collection*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_collection_free(inner_obj->collection, nullptr, nullptr);
			inner_obj->collection = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_collection_object_allocator */
static zend_object *
php_mysqlx_collection_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_collection_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_collection>(
		class_type,
		&mysqlx_object_collection_handlers,
		&mysqlx_collection_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_collection_class */
void
mysqlx_register_collection_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_collection_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_collection_handlers.free_obj = mysqlx_collection_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Collection", mysqlx_collection_methods);
		tmp_ce.create_object = php_mysqlx_collection_object_allocator;
		mysqlx_collection_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_collection_class_entry, 1, mysqlx_schema_object_interface_entry);
	}

	zend_hash_init(&mysqlx_collection_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_collection_properties, mysqlx_collection_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_collection_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_collection_class */
void
mysqlx_unregister_collection_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_collection_properties);
}
/* }}} */


/* {{{ mysqlx_new_collection */
void
mysqlx_new_collection(zval * return_value, xmysqlnd_collection * collection, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_collection");

	if (SUCCESS == object_init_ex(return_value, mysqlx_collection_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_collection* const object = (st_mysqlx_collection*) mysqlx_object->ptr;
		if (object) {
			object->collection = clone? collection->get_reference() : collection;
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
