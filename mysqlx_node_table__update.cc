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
extern "C" {
#include <php.h>
#undef ERROR
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_table.h>
#include <xmysqlnd/xmysqlnd_crud_table_commands.h>
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_table__update.h"
#include <phputils/allocator.h>
#include <phputils/object.h>

static zend_class_entry *mysqlx_node_table__update_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__set, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, table_field, IS_STRING, DONT_ALLOW_NULL)
	ZEND_ARG_INFO(NO_PASS_BY_REF, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__where, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(NO_PASS_BY_REF, where_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__orderby, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(NO_PASS_BY_REF, orderby_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, rows, IS_LONG, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, placeholder_values, IS_ARRAY, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_table__update : public mysqlx::phputils::custom_allocable
{
	XMYSQLND_CRUD_TABLE_OP__UPDATE * crud_op;
	XMYSQLND_NODE_TABLE * table;
};


#define MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_table__update *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_table__update::__construct */
static
PHP_METHOD(mysqlx_node_table__update, __construct)
{
}
/* }}} */


#define TWO_PARAM_OP__SET 1
#define TWO_PARAM_OP__ARRAY_INSERT 2
#define TWO_PARAM_OP__ARRAY_APPEND 3

/* {{{ mysqlx_node_table__update__2_param_op */
static void
mysqlx_node_table__update__2_param_op(INTERNAL_FUNCTION_PARAMETERS, const unsigned int op_type)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;
	const zval * value;
	MYSQLND_CSTRING table_field = {NULL, 0};
	zend_bool is_expression = FALSE;
	const zend_bool is_document = FALSE;

	DBG_ENTER("mysqlx_node_table__update__2_param_op");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Osz",
												&object_zv, mysqlx_node_table__update_class_entry,
												&(table_field.s), &(table_field.l),
												(zval *) &value))
	{
		DBG_VOID_RETURN;
	}
	switch (Z_TYPE_P(value)) {
		case IS_OBJECT:
			if (op_type == TWO_PARAM_OP__SET) {
				if (is_a_mysqlx_expression(value)) {
					/* get the string */
					value = get_mysqlx_expression(value);
					is_expression = TRUE;
				}
				break;
			}
			/* fall-through */
		case IS_STRING:
		case IS_DOUBLE:
		case IS_TRUE:
		case IS_FALSE:
		case IS_LONG:
		case IS_NULL:
			break;
		default:{
			RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_VOID_RETURN;
		}

	}
	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op) {
		enum_func_status ret = FAIL;
		switch (op_type) {
			case TWO_PARAM_OP__SET:
				ret = xmysqlnd_crud_table_update__set(object->crud_op, table_field, value, is_expression, is_document);
				break;
		}

		if (PASS == ret) {
			ZVAL_COPY(return_value, object_zv);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table__update::set() */
static
PHP_METHOD(mysqlx_node_table__update, set)
{
	mysqlx_node_table__update__2_param_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, TWO_PARAM_OP__SET);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table__update::where() */
static
PHP_METHOD(mysqlx_node_table__update, where)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;
	MYSQLND_CSTRING where_expr = {NULL, 0};

	DBG_ENTER("mysqlx_node_table__update::where");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_table__update_class_entry,
												&(where_expr.s), &(where_expr.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && where_expr.s && where_expr.l)
	{
		if (PASS == xmysqlnd_crud_table_update__set_criteria(object->crud_op, where_expr))
		{
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table__update::orderby() */
static
PHP_METHOD(mysqlx_node_table__update, orderby)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;
	zval * orderby_expr = NULL;
	int    num_of_expr = 0;
	int    i = 0;

	DBG_ENTER("mysqlx_node_table__update::orderby");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
												&object_zv,
												mysqlx_node_table__update_class_entry,
												&orderby_expr,
												&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (!( object->crud_op && orderby_expr ) ) {
		DBG_VOID_RETURN;
	}

	for( i = 0 ; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(orderby_expr[i])) {
		case IS_STRING:
			{
				const MYSQLND_CSTRING orderby_expr_str = { Z_STRVAL(orderby_expr[i]),
												Z_STRLEN(orderby_expr[i]) };
				if (PASS == xmysqlnd_crud_table_update__add_orderby(object->crud_op, orderby_expr_str)) {
					ZVAL_COPY(return_value, object_zv);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval * entry;
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(orderby_expr[i]), entry) {
					const MYSQLND_CSTRING orderby_expr_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
					}
					if (FAIL == xmysqlnd_crud_table_update__add_orderby(object->crud_op, orderby_expr_str)) {
						RAISE_EXCEPTION(err_msg_add_orderby_fail);
					}
				} ZEND_HASH_FOREACH_END();
				ZVAL_COPY(return_value, object_zv);
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			break;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table__update::limit() */
static
PHP_METHOD(mysqlx_node_table__update, limit)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;
	zend_long rows;

	DBG_ENTER("mysqlx_node_table__update::limit");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_node_table__update_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op) {
		if (PASS == xmysqlnd_crud_table_update__set_limit(object->crud_op, rows)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ proto mixed mysqlx_node_table__update::bind() */
static
PHP_METHOD(mysqlx_node_table__update, bind)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;
	HashTable * bind_variables;

	DBG_ENTER("mysqlx_node_table__update::bind");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, mysqlx_node_table__update_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op) {
		zend_string * key;
		zval * val;
		zend_bool op_success = TRUE;
		ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
			if (key) {
				const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
				if (FAIL == xmysqlnd_crud_table_update__bind_value(object->crud_op, variable, val)) {
					RAISE_EXCEPTION(err_msg_bind_fail);
					op_success = FALSE;
					break;
				}
			}
		} ZEND_HASH_FOREACH_END();
		if( op_success ) {
			ZVAL_COPY(return_value, object_zv);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ proto mixed mysqlx_node_table__update::execute() */
static
PHP_METHOD(mysqlx_node_table__update, execute)
{
	struct st_mysqlx_node_table__update * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table__update::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table__update_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	DBG_INF_FMT("crud_op=%p table=%p", object->crud_op, object->table);
	if (object->crud_op && object->table) {
		if (FALSE == xmysqlnd_crud_table_update__is_initialized(object->crud_op)) {
			RAISE_EXCEPTION(err_msg_update_fail);
		} else {
			XMYSQLND_NODE_STMT * stmt = object->table->data->m.update(object->table, object->crud_op);
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


/* {{{ mysqlx_node_table__update_methods[] */
static const zend_function_entry mysqlx_node_table__update_methods[] = {
	PHP_ME(mysqlx_node_table__update, __construct,	NULL,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_table__update, set,		arginfo_mysqlx_node_table__update__set,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table__update, where,	arginfo_mysqlx_node_table__update__where,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table__update, orderby,	arginfo_mysqlx_node_table__update__orderby,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table__update, limit,	arginfo_mysqlx_node_table__update__limit,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table__update, bind,		arginfo_mysqlx_node_table__update__bind,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_table__update, execute,	arginfo_mysqlx_node_table__update__execute,	ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

#if 0
/* {{{ mysqlx_node_table__update_property__name */
static zval *
mysqlx_node_table__update_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_table__update * object = (const struct st_mysqlx_node_table__update *) (obj->ptr);
	DBG_ENTER("mysqlx_node_table__update_property__name");
	if (object->table && object->table->data->table_name.s) {
		ZVAL_STRINGL(return_value, object->table->data->table_name.s, object->table->data->table_name.l);
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
#endif

static zend_object_handlers mysqlx_object_node_table__update_handlers;
static HashTable mysqlx_node_table__update_properties;

const struct st_mysqlx_property_entry mysqlx_node_table__update_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_node_table__update_property__name,	NULL},
#endif
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_table__update_free_storage */
static void
mysqlx_node_table__update_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_table__update * inner_obj = (struct st_mysqlx_node_table__update *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_node_table_free(inner_obj->table, NULL, NULL);
			inner_obj->table = NULL;
		}
		if(inner_obj->crud_op) {
			xmysqlnd_crud_table_update__destroy(inner_obj->crud_op);
			inner_obj->crud_op = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_table__update_object_allocator */
static zend_object *
php_mysqlx_node_table__update_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_table__update_object_allocator");
	st_mysqlx_object* mysqlx_object = mysqlx::phputils::alloc_object<st_mysqlx_node_table__update>(
		class_type,
		&mysqlx_object_node_table__update_handlers,
		&mysqlx_node_table__update_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_table__update_class */
void
mysqlx_register_node_table__update_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_table__update_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_table__update_handlers.free_obj = mysqlx_node_table__update_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeTableUpdate", mysqlx_node_table__update_methods);
		tmp_ce.create_object = php_mysqlx_node_table__update_object_allocator;
		mysqlx_node_table__update_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_table__update_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_node_table__update_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_table__update_properties, mysqlx_node_table__update_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_table__update_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_node_table__update_class */
void
mysqlx_unregister_node_table__update_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_table__update_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_table__update */
void
mysqlx_new_node_table__update(zval * return_value, XMYSQLND_NODE_TABLE * table, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_node_table__update");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_table__update_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_table__update * const object = (struct st_mysqlx_node_table__update *) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->data->m.get_reference(table) : table;
			object->crud_op = xmysqlnd_crud_table_update__create(
				mnd_str2c(object->table->data->schema->data->schema_name),
				mnd_str2c(object->table->data->table_name));
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

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
