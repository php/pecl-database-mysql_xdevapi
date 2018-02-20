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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_table.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_database_object.h"
#include "mysqlx_exception.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_table.h"
#include "mysqlx_node_schema.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace devapi {


namespace {

using namespace drv;

static zend_class_entry *mysqlx_node_schema_class_entry;

/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/************************************** INHERITED END   ****************************************/


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__create_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__drop_collection, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, schema_name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collections, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_table, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_tables, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collection_as_table, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_schema : public util::custom_allocable
{
	XMYSQLND_NODE_SCHEMA* schema;
};


#define MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_node_schema*) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->schema) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_schema::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, __construct)
{
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_node_schema::getSession() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getSession)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_node_schema::getSession");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	throw util::xdevapi_exception(util::xdevapi_exception::Code::not_implemented);

	if (object->schema) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getName() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getName)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_node_schema::getName");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->schema) {
		RETVAL_STRINGL(object->schema->data->schema_name.s,
				object->schema->data->schema_name.l);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_scheme_on_error */
static const enum_hnd_func_status
mysqlx_node_scheme_on_error(void* context, XMYSQLND_NODE_SESSION session, st_xmysqlnd_node_stmt* const stmt, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_scheme_on_error");
	mysqlx_new_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::existsInDatabase() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, existsInDatabase)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_node_schema::existsInDatabase");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	XMYSQLND_NODE_SCHEMA* schema = object->schema;
	if (schema) {
		const struct st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_node_scheme_on_error, nullptr };
		zval exists;
		ZVAL_UNDEF(&exists);
		if (PASS == schema->data->m.exists_in_database(schema, on_error, &exists)) {
			ZVAL_COPY_VALUE(return_value, &exists);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::drop() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, drop)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_node_schema::drop");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_schema_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->schema) {
		auto session = object->schema->data->session;
		const MYSQLND_CSTRING schema_name = mnd_str2c(object->schema->data->schema_name);

		RETVAL_BOOL(session && PASS == session->m->drop_db(session, schema_name));
	}

	DBG_VOID_RETURN;
}
/* }}} */
/************************************** INHERITED END   ****************************************/

/* {{{ mysqlx_node_schema_on_error */
static const enum_hnd_func_status
mysqlx_node_schema_on_error(void* context, const XMYSQLND_NODE_SCHEMA* const schema, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_schema_on_error");
	mysqlx_new_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ on_drop_db_object_error */
const enum_hnd_func_status on_drop_db_object_error(
	void* context,
	const XMYSQLND_NODE_SCHEMA * const schema,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("on_drop_db_object_error");
	throw util::xdevapi_exception(code, util::string(sql_state.s, sql_state.l), util::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::createCollection(string name) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, createCollection)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view collection_name;

	DBG_ENTER("mysqlx_node_schema::createCollection");
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_schema_class_entry,
		&(collection_name.str), &(collection_name.len)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	 if (!collection_name.empty() && object->schema) {
		const struct st_xmysqlnd_node_schema_on_error_bind on_error = { mysqlx_node_schema_on_error, nullptr };

		st_xmysqlnd_node_collection* const collection = object->schema->data->m.create_collection(object->schema, collection_name, on_error);
		DBG_INF_FMT("collection=%p", collection);
		if (collection) {
			mysqlx_new_node_collection(return_value, collection, FALSE);
			DBG_INF_FMT("type=%d", Z_TYPE_P(return_value));
			if (Z_TYPE_P(return_value) != IS_OBJECT) {
				DBG_ERR("Something is wrong");
				xmysqlnd_node_collection_free(collection, nullptr, nullptr);
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_schema::dropCollection(string collection_name) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, dropCollection)
{
	zval* object_zv{nullptr};
	util::string_view collection_name;

	DBG_ENTER("mysqlx_node_schema::dropCollection");
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os",
		&object_zv, mysqlx_node_schema_class_entry,
		&collection_name.str, &collection_name.len))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = util::fetch_data_object<st_mysqlx_node_schema>(object_zv);

	try {
		const st_xmysqlnd_node_schema_on_error_bind on_error = { on_drop_db_object_error, nullptr };
		RETVAL_BOOL(PASS == data_object.schema->data->m.drop_collection(data_object.schema, collection_name, on_error));
	} catch(std::exception& e) {
		util::log_warning(e.what());
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getCollection(string name) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getCollection)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view collection_name;

	DBG_ENTER("mysqlx_node_schema::getCollection");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(collection_name.str), &(collection_name.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ( !collection_name.empty() && object->schema) {
		st_xmysqlnd_node_collection* const collection = object->schema->data->m.create_collection_object(object->schema, collection_name.to_nd_cstr());
		if (collection) {
			mysqlx_new_node_collection(return_value, collection, FALSE);
			if (Z_TYPE_P(return_value) != IS_OBJECT) {
				xmysqlnd_node_collection_free(collection, nullptr, nullptr);
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getTable(string name) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getTable)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};
	util::string_view table_name;

	DBG_ENTER("mysqlx_node_schema::getTable");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(table_name.str), &(table_name.len)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if ( !table_name.empty() && object->schema) {
		st_xmysqlnd_node_table* const table = object->schema->data->m.create_table_object(object->schema, table_name.to_nd_cstr());
		mysqlx_new_node_table(return_value, table, FALSE /* no clone */);
		if (Z_TYPE_P(return_value) != IS_OBJECT) {
			xmysqlnd_node_table_free(table, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getCollectionAsTable(string name) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getCollectionAsTable)
{
	st_mysqlx_node_schema* object{nullptr};
	zval* object_zv{nullptr};
	MYSQLND_CSTRING collection_name = { nullptr, 0 };

	DBG_ENTER("mysqlx_node_schema::getCollectionAsTable");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(collection_name.s), &(collection_name.l)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if (collection_name.s && collection_name.l && object->schema) {
		st_xmysqlnd_node_table* const table = object->schema->data->m.create_table_object(object->schema, collection_name);
		mysqlx_new_node_table(return_value, table, FALSE /* no clone */);
		if (Z_TYPE_P(return_value) != IS_OBJECT) {
			xmysqlnd_node_table_free(table, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


struct st_mysqlx_on_db_object_ctx
{
	zval* list;
};

/* {{{ mysqlx_on_db_object */
static void
mysqlx_on_db_object(void* context, XMYSQLND_NODE_SCHEMA* const schema, const MYSQLND_CSTRING object_name, const MYSQLND_CSTRING object_type)
{
	st_mysqlx_on_db_object_ctx* ctx = static_cast<st_mysqlx_on_db_object_ctx*>(context);
	zval zv;

	DBG_ENTER("mysqlx_on_db_object");

	ZVAL_UNDEF(&zv);

	if ((object_type.s[0] == 'T') || (object_type.s[0] == 'V')) {
		XMYSQLND_NODE_TABLE* const table = schema->data->m.create_table_object(schema, object_name);
		if (table) {
			mysqlx_new_node_table(&zv, table, FALSE);
			if (Z_TYPE(zv) == IS_OBJECT) {
				add_assoc_zval_ex(ctx->list, object_name.s, object_name.l, &zv);
			} else {
				xmysqlnd_node_table_free(table, nullptr, nullptr);
				zval_dtor(&zv);
			}
		}
	} else if (object_type.s[0] == 'C') {
		XMYSQLND_NODE_COLLECTION* const collection = schema->data->m.create_collection_object(schema, object_name);
		if (collection) {
			mysqlx_new_node_collection(&zv, collection, FALSE);
			if (Z_TYPE(zv) == IS_OBJECT) {
				add_assoc_zval_ex(ctx->list, object_name.s, object_name.l, &zv);
			} else {
				xmysqlnd_node_collection_free(collection, nullptr, nullptr);
				zval_dtor(&zv);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_get_database_objects */
static void
mysqlx_get_database_objects(
	XMYSQLND_NODE_SCHEMA* schema,
	const db_object_type_filter object_type_filter,
	zval* return_value)
{
	DBG_ENTER("mysqlx_get_database_objects");
	if (schema){
		zval list;
		st_mysqlx_on_db_object_ctx context{ &list };
		const st_xmysqlnd_node_schema_on_database_object_bind on_object{ mysqlx_on_db_object, &context };
		const st_xmysqlnd_node_schema_on_error_bind handler_on_error{ mysqlx_node_schema_on_error, nullptr };

		ZVAL_UNDEF(&list);
		array_init(&list);

		if (PASS == schema->data->m.get_db_objects(schema, mnd_str2c(schema->data->schema_name), object_type_filter, on_object, handler_on_error)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlx_node_session::getTables() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getTables)
{
	zval* object_zv{nullptr};
	st_mysqlx_node_schema* object{nullptr};

	DBG_ENTER("mysqlx_node_schema::getTables");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	mysqlx_get_database_objects(object->schema, drv::db_object_type_filter::table_or_view, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::getCollections() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_schema, getCollections)
{
	zval* object_zv{nullptr};
	st_mysqlx_node_schema* object{nullptr};

	DBG_ENTER("mysqlx_node_schema::getCollections");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object_zv, mysqlx_node_schema_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;

	mysqlx_get_database_objects(object->schema, drv::db_object_type_filter::collection, return_value);

	DBG_VOID_RETURN;
}
/* }}} */




/* {{{ mysqlx_node_schema_methods[] */
static const zend_function_entry mysqlx_node_schema_methods[] = {
	PHP_ME(mysqlx_node_schema, __construct, nullptr, ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_node_schema, getSession, arginfo_mysqlx_node_schema__get_session, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getName, arginfo_mysqlx_node_schema__get_name, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, existsInDatabase, arginfo_mysqlx_node_schema__exists_in_database, ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/
	PHP_ME(mysqlx_node_schema, createCollection, arginfo_mysqlx_node_schema__create_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, dropCollection, arginfo_mysqlx_node_schema__drop_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getCollection, arginfo_mysqlx_node_schema__get_collection, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getCollections, arginfo_mysqlx_node_schema__get_collections, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_schema, getTable, arginfo_mysqlx_node_schema__get_table, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getTables, arginfo_mysqlx_node_schema__get_tables, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_schema, getCollectionAsTable,arginfo_mysqlx_node_schema__get_collection_as_table, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_node_schema_property__name */
static zval *
mysqlx_node_schema_property__name(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_node_schema* object = static_cast<const st_mysqlx_node_schema*>(obj->ptr);
	DBG_ENTER("mysqlx_node_schema_property__name");
	if (object->schema && object->schema->data->schema_name.s) {
		ZVAL_STRINGL(return_value, object->schema->data->schema_name.s, object->schema->data->schema_name.l);
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


static zend_object_handlers mysqlx_object_node_schema_handlers;
static HashTable mysqlx_node_schema_properties;

const struct st_mysqlx_property_entry mysqlx_node_schema_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_node_schema_property__name,	nullptr},
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_schema_free_storage */
static void
mysqlx_node_schema_free_storage(zend_object* object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_node_schema* inner_obj = static_cast<st_mysqlx_node_schema*>(mysqlx_object->ptr);

	if (inner_obj) {
		if (inner_obj->schema) {
			xmysqlnd_node_schema_free(inner_obj->schema, nullptr, nullptr);
			inner_obj->schema = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_schema_object_allocator */
static zend_object *
php_mysqlx_node_schema_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_node_schema_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_node_schema>(
		class_type,
		&mysqlx_object_node_schema_handlers,
		&mysqlx_node_schema_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace

/* {{{ mysqlx_register_node_schema_class */
void
mysqlx_register_node_schema_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	mysqlx_object_node_schema_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_schema_handlers.free_obj = mysqlx_node_schema_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeSchema", mysqlx_node_schema_methods);
		tmp_ce.create_object = php_mysqlx_node_schema_object_allocator;
		mysqlx_node_schema_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_schema_class_entry, 1, mysqlx_database_object_interface_entry);
	}

	zend_hash_init(&mysqlx_node_schema_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_schema_properties, mysqlx_node_schema_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_schema_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_node_schema_class */
void
mysqlx_unregister_node_schema_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_schema_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_schema */
void
mysqlx_new_node_schema(zval* return_value, XMYSQLND_NODE_SCHEMA* schema)
{
	DBG_ENTER("mysqlx_new_node_schema");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_schema_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_node_schema* const object = static_cast<st_mysqlx_node_schema*>(mysqlx_object->ptr);
		if (object) {
			object->schema = schema;
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
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
