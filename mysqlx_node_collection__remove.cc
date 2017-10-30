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
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__remove.h"
#include "phputils/allocator.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry* collection_remove_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove__skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__remove__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


/* {{{ Collection_remove::init() */
bool Collection_remove::init(
	zval* obj_zv,
	XMYSQLND_NODE_COLLECTION* coll,
	const phputils::string_view& search_expression)
{
	if (!obj_zv || !coll || search_expression.empty()) return false;
	object_zv = obj_zv;
	collection = coll->data->m.get_reference(coll);
	remove_op = xmysqlnd_crud_collection_remove__create(
		mnd_str2c(collection->data->schema->data->schema_name),
		mnd_str2c(collection->data->collection_name));
	if (!remove_op) return false;

	return xmysqlnd_crud_collection_remove__set_criteria(remove_op, search_expression.to_std_string()) == PASS;
}
/* }}} */


/* {{{ Collection_remove::~Collection_remove() */
Collection_remove::~Collection_remove()
{
	if (remove_op) {
		xmysqlnd_crud_collection_remove__destroy(remove_op);
	}

	if (collection) {
		xmysqlnd_node_collection_free(collection, nullptr, nullptr);
	}
}
/* }}} */


/* {{{ proto mixed Collection_remove::sort() */
void Collection_remove::sort(
	zval* sort_expr,
	int num_of_expr,
	zval* return_value)
{
	DBG_ENTER("Collection_remove::sort");

	RETVAL_FALSE;

	if (!sort_expr) {
		DBG_VOID_RETURN;
	}

	for( int i = 0 ; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i])) {
		case IS_STRING:
			{
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL(sort_expr[i]),
											Z_STRLEN(sort_expr[i]) };
				if (PASS == xmysqlnd_crud_collection_remove__add_sort(remove_op,
													sort_expr_str)) {
					ZVAL_COPY(return_value, object_zv);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval* entry;
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry),
												Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (FAIL == xmysqlnd_crud_collection_remove__add_sort(remove_op,
															sort_expr_str)) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
						DBG_VOID_RETURN;
					}
				} ZEND_HASH_FOREACH_END();
				ZVAL_COPY(return_value, object_zv);
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed Collection_remove::limit() */
void Collection_remove::limit(
	zend_long rows,
	zval* return_value)
{

	DBG_ENTER("Collection_remove::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = phputils::fetch_data_object<Collection_remove>(object_zv);

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_remove__set_limit(remove_op, rows)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed Collection_remove::bind() */
void Collection_remove::bind(
	HashTable* bind_variables,
	zval* return_value)
{
	DBG_ENTER("Collection_remove::bind");

	RETVAL_FALSE;

	zend_string* key;
	zval* val;
	ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
		if (key) {
			const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
			if (FAIL == xmysqlnd_crud_collection_remove__bind_value(remove_op, variable, val)) {
				RAISE_EXCEPTION(err_msg_bind_fail);
				DBG_VOID_RETURN;
			}
		}
	} ZEND_HASH_FOREACH_END();
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed Collection_remove::execute() */
void Collection_remove::execute(zval* return_value)
{
	DBG_ENTER("Collection_remove::execute");

	RETVAL_FALSE;

	DBG_INF_FMT("remove_op=%p collection=%p", remove_op, collection);
	if (remove_op && collection) {
		if (FALSE == xmysqlnd_crud_collection_remove__is_initialized(remove_op)) {
			static const unsigned int errcode = 10002;
			static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
			static const MYSQLND_CSTRING errmsg = { "Remove not completely initialized", sizeof("Remove not completely initialized") - 1 };
			mysqlx_new_exception(errcode, sqlstate, errmsg);
		} else {
			XMYSQLND_NODE_STMT* stmt = collection->data->m.remove(collection, remove_op);
			if (stmt) {
				zval stmt_zv;
				ZVAL_UNDEF(&stmt_zv);
				mysqlx_new_node_stmt(&stmt_zv, stmt);
				if (Z_TYPE(stmt_zv) == IS_NULL) {
					xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
				}
				if (Z_TYPE(stmt_zv) == IS_OBJECT) {
					zval zv;
					ZVAL_UNDEF(&zv);
					zend_long flags = 0;
					mysqlx_node_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv), flags, MYSQLX_RESULT, &zv);

					ZVAL_COPY(return_value, &zv);
					zval_dtor(&zv);
				}
				zval_ptr_dtor(&stmt_zv);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


//------------------------------------------------------------------------------


/* {{{ mysqlx_node_collection__remove::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::sort() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, sort)
{
	DBG_ENTER("mysqlx_node_collection__remove::sort");

	zval* object_zv = nullptr;
	zval* sort_expr = nullptr;
	int num_of_expr = 0;
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
									&object_zv,
									collection_remove_class_entry,
									&sort_expr,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = phputils::fetch_data_object<Collection_remove>(object_zv);
	coll_remove.sort(sort_expr, num_of_expr, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, limit)
{
	DBG_ENTER("mysqlx_node_collection__remove::limit");

	zval* object_zv = nullptr;
	zend_long rows = 0;
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, collection_remove_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = phputils::fetch_data_object<Collection_remove>(object_zv);
	coll_remove.limit(rows, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, bind)
{
	DBG_ENTER("mysqlx_node_collection__remove::bind");

	zval* object_zv = nullptr;
	HashTable* bind_variables = nullptr;
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, collection_remove_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = phputils::fetch_data_object<Collection_remove>(object_zv);
	coll_remove.bind(bind_variables, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, execute)
{
	DBG_ENTER("mysqlx_node_collection__remove::execute");

	zval* object_zv = nullptr;
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, collection_remove_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_remove& coll_remove = phputils::fetch_data_object<Collection_remove>(object_zv);
	coll_remove.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__remove_methods[] */
static const zend_function_entry mysqlx_node_collection__remove_methods[] = {
	PHP_ME(mysqlx_node_collection__remove, __construct,	nullptr,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__remove, bind,	arginfo_mysqlx_node_collection__remove__bind,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, sort,	arginfo_mysqlx_node_collection__remove__sort,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, limit,	arginfo_mysqlx_node_collection__remove__limit,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, execute,	arginfo_mysqlx_node_collection__remove__execute,	ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers collection_remove_handlers;
static HashTable collection_remove_properties;

const st_mysqlx_property_entry collection_remove_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_collection__remove_free_storage */
static void
mysqlx_node_collection__remove_free_storage(zend_object* object)
{
	phputils::free_object<Collection_remove>(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__remove_object_allocator */
static zend_object *
php_mysqlx_node_collection__remove_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__remove_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Collection_remove>(
		class_type,
		&collection_remove_handlers,
		&collection_remove_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__remove_class */
void
mysqlx_register_node_collection__remove_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_remove_class_entry,
		"NodeCollectionRemove",
		mysqlx_std_object_handlers,
		collection_remove_handlers,
		php_mysqlx_node_collection__remove_object_allocator,
		mysqlx_node_collection__remove_free_storage,
		mysqlx_node_collection__remove_methods,
		collection_remove_properties,
		collection_remove_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__remove_class */
void
mysqlx_unregister_node_collection__remove_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_remove_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__remove */
void
mysqlx_new_node_collection__remove(
	zval* return_value,
	const phputils::string_view& search_expression,
	XMYSQLND_NODE_COLLECTION* collection)
{
	DBG_ENTER("mysqlx_new_node_collection__remove");
	if (SUCCESS == object_init_ex(return_value, collection_remove_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		Collection_remove* const coll_remove = static_cast<Collection_remove*>(mysqlx_object->ptr);
		if (!coll_remove ||
			!coll_remove->init(return_value, collection->data->m.get_reference(collection), search_expression))
		{
			DBG_ERR("Error");
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
