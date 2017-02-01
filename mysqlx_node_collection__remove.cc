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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include <xmysqlnd/xmysqlnd_crud_collection_commands.h>
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__remove.h"
#include <phputils/allocator.h>
#include <phputils/object.h>

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_node_collection__remove_class_entry;

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


struct st_mysqlx_node_collection__remove : public phputils::custom_allocable
{
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE * crud_op;
	XMYSQLND_NODE_COLLECTION * collection;
};


#define MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_collection__remove *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_collection__remove::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::sort() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, sort)
{
	struct st_mysqlx_node_collection__remove * object;
	zval * object_zv;
	zval * sort_expr = NULL;
	int    num_of_expr = 0;
	int    i = 0;

	DBG_ENTER("mysqlx_node_collection__remove::sort");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
									&object_zv,
									mysqlx_node_collection__remove_class_entry,
									&sort_expr,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (!( object->crud_op && sort_expr ) ) {
		DBG_VOID_RETURN;
	}

	for( i = 0 ; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i])) {
		case IS_STRING:
			{
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL(sort_expr[i]),
											Z_STRLEN(sort_expr[i]) };
				if (PASS == xmysqlnd_crud_collection_remove__add_sort(object->crud_op,
													sort_expr_str)) {
					ZVAL_COPY(return_value, object_zv);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval * entry;
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry),
												Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
					}
					if (FAIL == xmysqlnd_crud_collection_remove__add_sort(object->crud_op,
															sort_expr_str)) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
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


/* {{{ proto mixed mysqlx_node_collection__remove::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, limit)
{
	struct st_mysqlx_node_collection__remove * object;
	zval * object_zv;
	zend_long rows;

	DBG_ENTER("mysqlx_node_collection__remove::limit");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_node_collection__remove_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op) {
		if (PASS == xmysqlnd_crud_collection_remove__set_limit(object->crud_op, rows)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, bind)
{
	struct st_mysqlx_node_collection__remove * object;
	zval * object_zv;
	HashTable * bind_variables;

	DBG_ENTER("mysqlx_node_collection__remove::bind");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, mysqlx_node_collection__remove_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op) {
		zend_string * key;
		zval * val;
		ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
			if (key) {
				const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
				if (FAIL == xmysqlnd_crud_collection_remove__bind_value(object->crud_op, variable, val)) {
					RAISE_EXCEPTION(err_msg_bind_fail);
				}
			}
		} ZEND_HASH_FOREACH_END();
		ZVAL_COPY(return_value, object_zv);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__remove::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__remove, execute)
{
	struct st_mysqlx_node_collection__remove * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_collection__remove::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection__remove_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	DBG_INF_FMT("crud_op=%p collection=%p", object->crud_op, object->collection);
	if (object->crud_op && object->collection) {
		if (FALSE == xmysqlnd_crud_collection_remove__is_initialized(object->crud_op)) {
			static const unsigned int errcode = 10002;
			static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
			static const MYSQLND_CSTRING errmsg = { "Remove not completely initialized", sizeof("Remove not completely initialized") - 1 };
			mysqlx_new_exception(errcode, sqlstate, errmsg);
		} else {
			XMYSQLND_NODE_STMT * stmt = object->collection->data->m.remove(object->collection, object->crud_op);
			if (stmt) {
				zval stmt_zv;
				ZVAL_UNDEF(&stmt_zv);
				mysqlx_new_node_stmt(&stmt_zv, stmt);
				if (Z_TYPE(stmt_zv) == IS_NULL) {
					xmysqlnd_node_stmt_free(stmt, NULL, NULL);
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


/* {{{ mysqlx_node_collection__remove_methods[] */
static const zend_function_entry mysqlx_node_collection__remove_methods[] = {
	PHP_ME(mysqlx_node_collection__remove, __construct,	NULL,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__remove, bind,	arginfo_mysqlx_node_collection__remove__bind,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, sort,	arginfo_mysqlx_node_collection__remove__sort,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, limit,	arginfo_mysqlx_node_collection__remove__limit,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__remove, execute,	arginfo_mysqlx_node_collection__remove__execute,	ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_collection__remove_handlers;
static HashTable mysqlx_node_collection__remove_properties;

const struct st_mysqlx_property_entry mysqlx_node_collection__remove_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection__remove_free_storage */
static void
mysqlx_node_collection__remove_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_collection__remove * inner_obj = (struct st_mysqlx_node_collection__remove *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_node_collection_free(inner_obj->collection, NULL, NULL);
			inner_obj->collection = NULL;
		}
		if (inner_obj->crud_op) {
			xmysqlnd_crud_collection_remove__destroy(inner_obj->crud_op);
			inner_obj->crud_op = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__remove_object_allocator */
static zend_object *
php_mysqlx_node_collection__remove_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_collection__remove_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<st_mysqlx_node_collection__remove>(
		class_type,
		&mysqlx_object_node_collection__remove_handlers,
		&mysqlx_node_collection__remove_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__remove_class */
void
mysqlx_register_node_collection__remove_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_collection__remove_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_collection__remove_handlers.free_obj = mysqlx_node_collection__remove_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeCollectionRemove", mysqlx_node_collection__remove_methods);
		tmp_ce.create_object = php_mysqlx_node_collection__remove_object_allocator;
		mysqlx_node_collection__remove_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_collection__remove_class_entry, 4,
							  mysqlx_executable_interface_entry,
							  mysqlx_crud_operation_bindable_interface_entry,
							  mysqlx_crud_operation_limitable_interface_entry,
							  mysqlx_crud_operation_sortable_interface_entry);
	}

	zend_hash_init(&mysqlx_node_collection__remove_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_collection__remove_properties, mysqlx_node_collection__remove_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__remove_class */
void
mysqlx_unregister_node_collection__remove_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_collection__remove_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__remove */
void
mysqlx_new_node_collection__remove(zval * return_value,
								   const MYSQLND_CSTRING search_expression,
								   XMYSQLND_NODE_COLLECTION * collection,
								   const zend_bool clone_collection)
{
	DBG_ENTER("mysqlx_new_node_collection__remove");
	if (SUCCESS == object_init_ex(return_value, mysqlx_node_collection__remove_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_collection__remove * const object = (struct st_mysqlx_node_collection__remove *) mysqlx_object->ptr;
		if (!object) {
			goto err;
		}
		object->collection = clone_collection? collection->data->m.get_reference(collection) : collection;
		object->crud_op = xmysqlnd_crud_collection_remove__create(mnd_str2c(object->collection->data->schema->data->schema_name),
																  mnd_str2c(object->collection->data->collection_name));
		if (!object->crud_op) {
			goto err;
		}
		if (search_expression.s &&
			search_expression.l &&
			FAIL == xmysqlnd_crud_collection_remove__set_criteria(object->crud_op, search_expression))
		{
			goto err;
		}
		goto end;
err:
		DBG_ERR("Error");
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		if (object->collection && clone_collection) {
			object->collection->data->m.free_reference(object->collection, NULL, NULL);
		}
		zval_ptr_dtor(return_value);
		ZVAL_NULL(return_value);
	}
end:
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
