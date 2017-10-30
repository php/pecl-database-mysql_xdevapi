/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_index_collection_commands.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_node_collection__add.h"
#include "mysqlx_node_collection__find.h"
#include "mysqlx_node_collection__modify.h"
#include "mysqlx_node_collection__remove.h"
#include "mysqlx_node_collection__create_index.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_doc_result.h"
#include "mysqlx_node_schema.h"
#include "phputils/allocator.h"
#include "phputils/hash_table.h"
#include "phputils/json_utils.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace {

zend_class_entry* mysqlx_node_collection_class_entry;

} // anonymous namespace

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
/************************************** INHERITED END   ****************************************/

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, json)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__modify, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, search_condition, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


// single doc ops
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection_get_one, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection_replace_one, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, doc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection_add_or_replace_one, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, doc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection_remove_one, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, id, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


// index ops
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__create_index, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, index_name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, is_unique, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__drop_index, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, index_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()



struct st_mysqlx_node_collection : public phputils::custom_allocable
{
	XMYSQLND_NODE_COLLECTION * collection;
};


#define MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_collection *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_collection::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, __construct)
{
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_node_collection::getSession() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, getSession)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;

	DBG_ENTER("mysqlx_node_collection::getSession");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::not_implemented);

	if (object->collection) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::getName() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, getName)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;

	DBG_ENTER("mysqlx_node_collection::getName");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	if (object->collection) {
		RETVAL_STRINGL(object->collection->data->collection_name.s, object->collection->data->collection_name.l);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection_on_error */
static const enum_hnd_func_status
mysqlx_node_collection_on_error(void * context, XMYSQLND_NODE_SESSION * session,
					struct st_xmysqlnd_node_stmt * const stmt,
					const unsigned int code,
					const MYSQLND_CSTRING sql_state,
					const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_collection_on_error");
	const unsigned int UnknownDatabaseCode = 1049;
	if (code == UnknownDatabaseCode) {
		DBG_RETURN(HND_PASS);
	} else {
		mysqlx_new_exception(code, sql_state, message);
		DBG_RETURN(HND_PASS_RETURN_FAIL);
	}
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::existsInDatabase() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, existsInDatabase)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;

	DBG_ENTER("mysqlx_node_collection::existsInDatabase");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	XMYSQLND_NODE_COLLECTION * collection = object->collection;
	if (collection) {
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_node_collection_on_error, NULL };
		zval exists;
		ZVAL_UNDEF(&exists);
		if (PASS == collection->data->m.exists_in_database(collection, on_error, &exists)) {
			ZVAL_COPY_VALUE(return_value, &exists);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::count() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, count)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;

	DBG_ENTER("mysqlx_node_collection::count");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	XMYSQLND_NODE_COLLECTION * collection = object->collection;
	if (collection) {
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_node_collection_on_error, NULL };
		zval counter;
		ZVAL_UNDEF(&counter);
		if (PASS == collection->data->m.count(collection, on_error, &counter)) {
			ZVAL_COPY_VALUE(return_value, &counter);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::getSchema() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, getSchema)
{
	st_mysqlx_node_collection* object = nullptr;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING schema_name = {nullptr, 0};
	zval* object_zv = nullptr;

	DBG_ENTER("mysqlx_node_collection::getSchema");

	if (FAILURE == zend_parse_method_parameters(
				ZEND_NUM_ARGS(),
				getThis(), "Os",
				&object_zv,
				mysqlx_node_collection_class_entry,
				&(schema_name.s), &(schema_name.l))) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	if( object->collection &&
		object->collection->data &&
		object->collection->data->schema &&
		object->collection->data->schema->data ) {
		session = object->collection->data->schema->data->session;
	}

	if(session != NULL) {
		XMYSQLND_NODE_SCHEMA * schema = session->m->create_schema_object(
					session, schema_name);
		if (schema) {
			mysqlx_new_node_schema(return_value, schema);
		} else {
			RAISE_EXCEPTION(10001,"Invalid object of class schema");
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */
/************************************** INHERITED END   ****************************************/


/* {{{ proto mixed mysqlx_node_collection::add() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, add)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;
	zval* docs = nullptr;
	int num_of_docs = 0;

	DBG_ENTER("mysqlx_node_collection::add");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
												&object_zv,
												mysqlx_node_collection_class_entry,
												&docs,
												&num_of_docs))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_node_collection__add(return_value, object->collection,
										docs,
										num_of_docs);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::find() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, find)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;
	phputils::string_input_param search_expr;

	DBG_ENTER("mysqlx_node_collection::find");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|s",
												&object_zv, mysqlx_node_collection_class_entry,
												&(search_expr.str), &(search_expr.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_node_collection__find(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::modify() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, modify)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;
	phputils::string_input_param search_expr;

	DBG_ENTER("mysqlx_node_collection::modify");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_node_collection__modify(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::remove() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, remove)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;
	phputils::string_input_param search_expr;

	DBG_ENTER("mysqlx_node_collection::remove");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_collection_class_entry,
		&(search_expr.str), &(search_expr.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_node_collection__remove(return_value, search_expr, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::getOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, getOne)
{
	DBG_ENTER("mysqlx_node_collection::getOne");

	zval* object_zv = nullptr;
	phputils::string_input_param id;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_collection_class_entry,
		&id.str, &id.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<st_mysqlx_node_collection>(object_zv);

	Collection_find coll_find;
	const char* Get_one_search_expression = "_id = :id";
	if (!coll_find.init(object_zv, data_object.collection, Get_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	phputils::Hash_table bind_variables;
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


/* {{{ proto mixed mysqlx_node_collection::replaceOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, replaceOne)
{
	DBG_ENTER("mysqlx_node_collection::replaceOne");

	zval* object_zv = nullptr;
	phputils::string_input_param id;
	zval* doc = nullptr;

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Osz",
		&object_zv, mysqlx_node_collection_class_entry,
		&id.str, &id.len,
		&doc))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<st_mysqlx_node_collection>(object_zv);

	Collection_modify coll_modify;
	const char* Replace_one_search_expression = "$._id = :id";
	if (!coll_modify.init(object_zv, data_object.collection, Replace_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	phputils::Hash_table bind_variables;
	bind_variables.insert("id", id);
	coll_modify.bind(bind_variables.ptr(), return_value);
	if (Z_TYPE_P(return_value) == IS_FALSE) {
		DBG_VOID_RETURN;
	}

	const phputils::string_input_param Doc_root_path("$");
	zval doc_with_id;
	phputils::json::ensure_doc_id(doc, id, &doc_with_id);
	coll_modify.set(Doc_root_path, true, &doc_with_id, return_value);

	coll_modify.execute(return_value);
	zval_ptr_dtor(&doc_with_id);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::addOrReplaceOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, addOrReplaceOne)
{
	zval* object_zv = nullptr;
	phputils::string_input_param id;
	zval* doc = nullptr;

	DBG_ENTER("mysqlx_node_collection::addOrReplaceOne");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Osz",
		&object_zv, mysqlx_node_collection_class_entry,
		&id.str, &id.len,
		&doc))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<st_mysqlx_node_collection>(object_zv);

	Collection_add coll_add;
	zval doc_with_id;
	phputils::json::ensure_doc_id(doc, id, &doc_with_id);
	if (!coll_add.init(object_zv, data_object.collection, id, &doc_with_id)) {
		DBG_VOID_RETURN;
	}

	coll_add.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::removeOne() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, removeOne)
{
	zval* object_zv = nullptr;
	phputils::string_input_param id;

	DBG_ENTER("mysqlx_node_collection::removeOne");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_collection_class_entry,
		&id.str, &id.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<st_mysqlx_node_collection>(object_zv);
	Collection_remove coll_remove;
	const char* Remove_one_search_expression = "_id = :id";
	if (!coll_remove.init(object_zv, data_object.collection, Remove_one_search_expression)) {
		DBG_VOID_RETURN;
	}

	phputils::Hash_table bind_variables;
	bind_variables.insert("id", id);
	coll_remove.bind(bind_variables.ptr(), return_value);
	if (Z_TYPE_P(return_value) == IS_FALSE) {
		DBG_VOID_RETURN;
	}

	coll_remove.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::createIndex() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, createIndex)
{
	st_mysqlx_node_collection* object = nullptr;
	zval* object_zv = nullptr;
	MYSQLND_CSTRING index_name = {nullptr, 0};
	zend_bool is_unique;

	DBG_ENTER("mysqlx_node_collection::createIndex");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Osb",
		&object_zv, mysqlx_node_collection_class_entry,
		&(index_name.s), &(index_name.l),
		&is_unique))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->collection) {
		mysqlx_new_node_collection__create_index(return_value, index_name, is_unique, object->collection);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ collection_drop_index_on_error */
static const enum_hnd_func_status
collection_drop_index_on_error(
	void * context,
	XMYSQLND_NODE_SESSION * session,
	st_xmysqlnd_node_stmt * const stmt,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("collection_drop_index_on_error");
	throw phputils::xdevapi_exception(code, phputils::string(sql_state.s, sql_state.l), phputils::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection::dropIndex() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection, dropIndex)
{
	zval* object_zv = nullptr;
	phputils::string_input_param index_name;

	DBG_ENTER("mysqlx_node_collection::dropIndex");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_collection_class_entry,
		&(index_name.str), &(index_name.len)))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<st_mysqlx_node_collection>(object_zv);

	try {
		const st_xmysqlnd_node_session_on_error_bind on_error = { collection_drop_index_on_error, nullptr };
		RETVAL_BOOL(drv::collection_drop_index(data_object.collection, index_name, on_error));
	} catch (std::exception& e) {
		phputils::log_warning(e.what());
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection_methods[] */
static const zend_function_entry mysqlx_node_collection_methods[] = {
	PHP_ME(mysqlx_node_collection, __construct,		NULL,												ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_node_collection, getSession,		arginfo_mysqlx_node_collection__get_session,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, getName,			arginfo_mysqlx_node_collection__get_name,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, existsInDatabase,arginfo_mysqlx_node_collection__exists_in_database,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, count,			arginfo_mysqlx_node_collection__count,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_collection, getSchema,		arginfo_mysqlx_node_collection__get_schema,			ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/

	PHP_ME(mysqlx_node_collection, add, 	arginfo_mysqlx_node_collection__add,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, find, 	arginfo_mysqlx_node_collection__find,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, modify,	arginfo_mysqlx_node_collection__modify, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, remove,	arginfo_mysqlx_node_collection__remove,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_collection, getOne, arginfo_mysqlx_node_collection_get_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, replaceOne, arginfo_mysqlx_node_collection_replace_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, addOrReplaceOne, arginfo_mysqlx_node_collection_add_or_replace_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, removeOne, arginfo_mysqlx_node_collection_remove_one, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_collection, createIndex,	arginfo_mysqlx_node_collection__create_index,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection, dropIndex,	arginfo_mysqlx_node_collection__drop_index,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_node_collection_property__name */
static zval *
mysqlx_node_collection_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_collection * object = (const struct st_mysqlx_node_collection *) (obj->ptr);
	DBG_ENTER("mysqlx_node_collection_property__name");
	if (object->collection && object->collection->data->collection_name.s) {
		ZVAL_STRINGL(return_value, object->collection->data->collection_name.s, object->collection->data->collection_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


static zend_object_handlers mysqlx_object_node_collection_handlers;
static HashTable mysqlx_node_collection_properties;

const struct st_mysqlx_property_entry mysqlx_node_collection_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_node_collection_property__name,	NULL},
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection_free_storage */
static void
mysqlx_node_collection_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_collection * inner_obj = (struct st_mysqlx_node_collection *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_node_collection_free(inner_obj->collection, NULL, NULL);
			inner_obj->collection = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection_object_allocator */
static zend_object *
php_mysqlx_node_collection_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_collection_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<st_mysqlx_node_collection>(
		class_type,
		&mysqlx_object_node_collection_handlers,
		&mysqlx_node_collection_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection_class */
void
mysqlx_register_node_collection_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_collection_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_collection_handlers.free_obj = mysqlx_node_collection_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeCollection", mysqlx_node_collection_methods);
		tmp_ce.create_object = php_mysqlx_node_collection_object_allocator;
		mysqlx_node_collection_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_collection_class_entry, 1, mysqlx_schema_object_interface_entry);
	}

	zend_hash_init(&mysqlx_node_collection_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_collection_properties, mysqlx_node_collection_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_collection_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection_class */
void
mysqlx_unregister_node_collection_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_collection_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection */
void
mysqlx_new_node_collection(zval * return_value, XMYSQLND_NODE_COLLECTION * collection, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_node_collection");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_collection_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_collection * const object = (struct st_mysqlx_node_collection *) mysqlx_object->ptr;
		if (object) {
			object->collection = clone? collection->data->m.get_reference(collection) : collection;
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
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
