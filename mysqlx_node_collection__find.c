/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#include <php.h>
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include <xmysqlnd/xmysqlnd_crud_collection_commands.h>
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_collection__find.h"

static zend_class_entry *mysqlx_node_collection__find_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(NO_PASS_BY_REF, projection)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__group_by, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(NO_PASS_BY_REF, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(NO_PASS_BY_REF, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, rows, IS_LONG, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__offset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, position, IS_LONG, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, placeholder_values, IS_ARRAY, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_collection__find
{
	XMYSQLND_CRUD_COLLECTION_OP__FIND * crud_op;
	XMYSQLND_NODE_COLLECTION * collection;
};


#define MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_collection__find::__construct */
static
PHP_METHOD(mysqlx_node_collection__find, __construct)
{
}
/* }}} */

/* {{{ mysqlx_node_collection__find::fields */
static
PHP_METHOD(mysqlx_node_collection__find, fields)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;
	const zval * fields;
	zend_bool is_expression = FALSE;

	DBG_ENTER("mysqlx_node_collection__find::fields");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oz",
												&object_zv, mysqlx_node_collection__find_class_entry,
												(zval *) &fields))
	{
		DBG_VOID_RETURN;
	}
	switch (Z_TYPE_P(fields)) {
		case IS_STRING:
		case IS_ARRAY:
			break;
		case IS_OBJECT:
			if (is_a_mysqlx_expression(fields)) {
				/* get the string */
				fields = get_mysqlx_expression(fields);
				is_expression = TRUE;
			}
			/* fall-through */
		default:{
			static const unsigned int errcode = 10007;
			static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
			static const MYSQLND_CSTRING errmsg = { "Invalid value type", sizeof("Invalid value type") - 1 };
			mysqlx_new_exception(errcode, sqlstate, errmsg);
			DBG_VOID_RETURN;
		}			
	}
	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->collection) {
		enum_func_status ret = FAIL;
		if (Z_TYPE_P(fields) == IS_STRING) {
			const MYSQLND_CSTRING field_str = { Z_STRVAL_P(fields), Z_STRLEN_P(fields) };
			ret = xmysqlnd_crud_collection_find__set_fields(object->crud_op, field_str, is_expression, FALSE);
		} else if (Z_TYPE_P(fields) == IS_ARRAY) {
			const zval * entry;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(fields), entry) {
				is_expression = FALSE;
				if (Z_TYPE_P(entry) == IS_OBJECT) {
					if (is_a_mysqlx_expression(entry)) {
						/* get the string */
						entry = get_mysqlx_expression(entry);
						is_expression = TRUE;
					}
				}
				/* NO else */
				if (Z_TYPE_P(entry) != IS_STRING) {
					static const unsigned int errcode = 10003;
					static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
					static const MYSQLND_CSTRING errmsg = { "Parameter must be an array of strings", sizeof("Parameter must be an array of strings") - 1 };
					mysqlx_new_exception(errcode, sqlstate, errmsg);
					break;
				}
				{
					MYSQLND_CSTRING field_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					ret = xmysqlnd_crud_collection_find__set_fields(object->crud_op, field_str, FALSE, TRUE);
				}
			} ZEND_HASH_FOREACH_END();		
		}
		if (FAIL == ret) {
			static const unsigned int errcode = 10004;
			static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
			static const MYSQLND_CSTRING errmsg = { "Error while adding a sort expression", sizeof("Error while adding a sort expression") - 1 };
			mysqlx_new_exception(errcode, sqlstate, errmsg);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */



#define ADD_SORT 1
#define ADD_GROUPING 2

/* {{{ mysqlx_node_collection__find__add_sort_or_grouping */
static void
mysqlx_node_collection__find__add_sort_or_grouping(INTERNAL_FUNCTION_PARAMETERS, const unsigned int op_type)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;
	zval * sort_expr = NULL;

	DBG_ENTER("mysqlx_node_collection__find__add_sort_or_grouping");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oz",
												&object_zv, mysqlx_node_collection__find_class_entry,
												&sort_expr))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && sort_expr) {
		switch (Z_TYPE_P(sort_expr)) {
			case IS_STRING: {
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(sort_expr), Z_STRLEN_P(sort_expr) };
				if (ADD_SORT == op_type) {
					RETVAL_BOOL(PASS == xmysqlnd_crud_collection_find__add_sort(object->crud_op, sort_expr_str));
				} else if (ADD_GROUPING == op_type) {
					RETVAL_BOOL(PASS == xmysqlnd_crud_collection_find__add_grouping(object->crud_op, sort_expr_str));
				}
				break;
			}
			case IS_ARRAY: {
				zval * entry;
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(sort_expr), entry) {
					enum_func_status ret = FAIL;
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						static const unsigned int errcode = 10003;
						static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
						static const MYSQLND_CSTRING errmsg = { "Parameter must be an array of strings", sizeof("Parameter must be an array of strings") - 1 };
						mysqlx_new_exception(errcode, sqlstate, errmsg);
						goto end;
					}
					if (ADD_SORT == op_type) {
						ret = xmysqlnd_crud_collection_find__add_sort(object->crud_op, sort_expr_str);
					} else if (ADD_GROUPING == op_type) {
						ret = xmysqlnd_crud_collection_find__add_grouping(object->crud_op, sort_expr_str);
					}
					if (FAIL == ret) {
						static const unsigned int errcode = 10004;
						static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
						static const MYSQLND_CSTRING errmsg = { "Error while adding a sort expression", sizeof("Error while adding a sort expression") - 1 };
						mysqlx_new_exception(errcode, sqlstate, errmsg);
						goto end;
					}
				} ZEND_HASH_FOREACH_END();
				break;
			}
			/* fall-through */
			default: {
				static const unsigned int errcode = 10005;
				static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
				static const MYSQLND_CSTRING errmsg = { "Parameter must be a string or array of strings", sizeof("Parameter must be a string or array of strings") - 1 };
				mysqlx_new_exception(errcode, sqlstate, errmsg);
			}			
		}
	}
end:
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::sort() */
static
PHP_METHOD(mysqlx_node_collection__find, sort)
{
	DBG_ENTER("mysqlx_node_collection__find::sort");
	mysqlx_node_collection__find__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_SORT);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::groupBy() */
static
PHP_METHOD(mysqlx_node_collection__find, groupBy)
{
	DBG_ENTER("mysqlx_node_collection__find::groupBy");
	mysqlx_node_collection__find__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_GROUPING);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::limit() */
static
PHP_METHOD(mysqlx_node_collection__find, limit)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;
	zend_long rows;

	DBG_ENTER("mysqlx_node_collection__find::limit");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_node_collection__find_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		static const unsigned int errcode = 10006;
		static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
		static const MYSQLND_CSTRING errmsg = { "Parameter must be a non-negative value", sizeof("Parameter must be a non-negative value") - 1 };
		mysqlx_new_exception(errcode, sqlstate, errmsg);	
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->collection) {
		RETVAL_BOOL(PASS == xmysqlnd_crud_collection_find__set_limit(object->crud_op, rows));
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::offset() */
static
PHP_METHOD(mysqlx_node_collection__find, offset)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;
	zend_long position;

	DBG_ENTER("mysqlx_node_collection__find::offset");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_node_collection__find_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		static const unsigned int errcode = 10006;
		static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
		static const MYSQLND_CSTRING errmsg = { "Parameter must be a non-negative value", sizeof("Parameter must be a non-negative value") - 1 };
		mysqlx_new_exception(errcode, sqlstate, errmsg);	
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->collection) {
		RETVAL_BOOL(PASS == xmysqlnd_crud_collection_find__set_offset(object->crud_op, position));
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::bind() */
static
PHP_METHOD(mysqlx_node_collection__find, bind)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;
	HashTable * bind_variables;

	DBG_ENTER("mysqlx_node_collection__find::bind");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, mysqlx_node_collection__find_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->collection) {
		zend_string * key;
		zval * val;
		ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
			if (key) {
				const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
				if (FAIL == xmysqlnd_crud_collection_find__bind_value(object->crud_op, variable, val)) {
					static const unsigned int errcode = 10005;
					static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
					static const MYSQLND_CSTRING errmsg = { "Error while binding a variable", sizeof("Error while binding a variable") - 1 };
					mysqlx_new_exception(errcode, sqlstate, errmsg);
					goto end;
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
end:
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
static
PHP_METHOD(mysqlx_node_collection__find, execute)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_collection__find::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection__find_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->collection) {
		if (FALSE == xmysqlnd_crud_collection_find__is_initialized(object->crud_op)) {
			static const unsigned int errcode = 10008;
			static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 };
			static const MYSQLND_CSTRING errmsg = { "Find not completely initialized", sizeof("Find not completely initialized") - 1 };
			mysqlx_new_exception(errcode, sqlstate, errmsg);
		} else {
			RETVAL_BOOL(PASS == object->collection->data->m.find(object->collection, object->crud_op));
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__find_methods[] */
static const zend_function_entry mysqlx_node_collection__find_methods[] = {
	PHP_ME(mysqlx_node_collection__find, 	__construct,	NULL,										ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__find,	fields,		arginfo_mysqlx_node_collection__find__fields,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	groupBy,	arginfo_mysqlx_node_collection__find__group_by,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	bind,		arginfo_mysqlx_node_collection__find__bind,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	sort,		arginfo_mysqlx_node_collection__find__sort,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	limit,		arginfo_mysqlx_node_collection__find__limit,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	offset,		arginfo_mysqlx_node_collection__find__offset,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find,	execute,	arginfo_mysqlx_node_collection__find__execute,	ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_collection__find_handlers;
static HashTable mysqlx_node_collection__find_properties;

const struct st_mysqlx_property_entry mysqlx_node_collection__find_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection__find_free_storage */
static void
mysqlx_node_collection__find_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_collection__find * inner_obj = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_node_collection_free(inner_obj->collection, NULL, NULL);
			inner_obj->collection = NULL;
		}
		if (inner_obj->crud_op) {
			xmysqlnd_crud_collection_find__destroy(inner_obj->crud_op);
			inner_obj->crud_op = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_collection__find_object_allocator */
static zend_object *
php_mysqlx_node_collection__find_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_collection__find * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_collection__find));

	DBG_ENTER("php_mysqlx_node_collection__find_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_collection__find_handlers;
	mysqlx_object->properties = &mysqlx_node_collection__find_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__find_class */
void
mysqlx_register_node_collection__find_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_collection__find_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_collection__find_handlers.free_obj = mysqlx_node_collection__find_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeCollectionFind", mysqlx_node_collection__find_methods);
		tmp_ce.create_object = php_mysqlx_node_collection__find_object_allocator;
		mysqlx_node_collection__find_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_collection__find_class_entry, 4,
							  mysqlx_executable_interface_entry,
							  mysqlx_crud_operation_bindable_interface_entry,
							  mysqlx_crud_operation_limitable_interface_entry,
							  mysqlx_crud_operation_sortable_interface_entry);
	}

	zend_hash_init(&mysqlx_node_collection__find_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_collection__find_properties, mysqlx_node_collection__find_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__find_class */
void
mysqlx_unregister_node_collection__find_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_collection__find_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__find */
void
mysqlx_new_node_collection__find(zval * return_value,
								 const MYSQLND_CSTRING search_expression,
								 XMYSQLND_NODE_COLLECTION * collection,
								 const zend_bool clone_collection)
{
	DBG_ENTER("mysqlx_new_node_collection__find");
	if (SUCCESS == object_init_ex(return_value, mysqlx_node_collection__find_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_collection__find * const object = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr;
		if (!object) {
			goto err;
		}
		object->collection = clone_collection? collection->data->m.get_reference(collection) : collection;
		object->crud_op = xmysqlnd_crud_collection_find__create(mnd_str2c(object->collection->data->schema->data->schema_name),
																mnd_str2c(object->collection->data->collection_name));
		if (!object->crud_op) {
			goto err;
		}
		if (search_expression.s &&
			search_expression.l &&
			FAIL == xmysqlnd_crud_collection_find__set_criteria(object->crud_op, search_expression))
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


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
