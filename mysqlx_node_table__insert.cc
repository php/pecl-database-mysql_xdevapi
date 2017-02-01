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
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/json/php_json.h>
#include <zend_smart_str.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_table.h>
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_table__insert.h"
#include <phputils/allocator.h>
#include <phputils/object.h>

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_node_table__insert_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__insert__values, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, row_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__insert__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_table__insert : public phputils::custom_allocable
{
	XMYSQLND_CRUD_TABLE_OP__INSERT * crud_op;
	XMYSQLND_NODE_TABLE * table;
};


#define MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_table__insert *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_table__insert::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_table__insert, __construct)
{
}
/* }}} */



/* {{{ proto mixed mysqlx_node_table__insert::values() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_table__insert, values)
{
	struct st_mysqlx_node_table__insert * object;
	zval * object_zv;
	zval * values = NULL;
	zend_bool op_failed = FALSE;
	int    num_of_values = 0, i = 0;

	DBG_ENTER("mysqlx_node_table__insert::values");

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "O+",
		&object_zv,
		mysqlx_node_table__insert_class_entry,
		&values,
		&num_of_values))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	DBG_INF_FMT("Num of values: %d",
				num_of_values);

	for( i = 0 ; i < num_of_values ; ++i ) {
		if (FAIL == xmysqlnd_crud_table_insert__add_row(object->crud_op,
												&values[i])) {
			op_failed = TRUE;
			break;
		}
	}

	if( op_failed == FALSE ) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table__insert::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_table__insert, execute)
{
	struct st_mysqlx_node_table__insert * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table__insert::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table__insert_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	DBG_INF_FMT("crud_op=%p table=%p", object->crud_op, object->table);
	if (object->crud_op && object->table) {
		if (FALSE == xmysqlnd_crud_table_insert__is_initialized(object->crud_op)) {
			RAISE_EXCEPTION(err_msg_insert_fail);
		} else {
			XMYSQLND_NODE_STMT * stmt = object->table->data->m.insert(object->table, object->crud_op);
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


/* {{{ mysqlx_node_table__insert_methods[] */
static const zend_function_entry mysqlx_node_table__insert_methods[] = {
	PHP_ME(mysqlx_node_table__insert, __construct,	NULL,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_table__insert, values,		arginfo_mysqlx_node_table__insert__values,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table__insert, execute,		arginfo_mysqlx_node_table__insert__execute,		ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

#if 0
/* {{{ mysqlx_node_table__insert_property__name */
static zval *
mysqlx_node_table__insert_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_table__insert * object = (const struct st_mysqlx_node_table__insert *) (obj->ptr);
	DBG_ENTER("mysqlx_node_table__insert_property__name");
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

static zend_object_handlers mysqlx_object_node_table__insert_handlers;
static HashTable mysqlx_node_table__insert_properties;

const struct st_mysqlx_property_entry mysqlx_node_table__insert_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_node_table__insert_property__name,	NULL},
#endif
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_table__insert_free_storage */
static void
mysqlx_node_table__insert_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_table__insert * inner_obj = (struct st_mysqlx_node_table__insert *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_node_table_free(inner_obj->table, NULL, NULL);
			inner_obj->table = NULL;
		}
		if(inner_obj->crud_op) {
			xmysqlnd_crud_table_insert__destroy(inner_obj->crud_op);
			inner_obj->crud_op = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_table__insert_object_allocator */
static zend_object *
php_mysqlx_node_table__insert_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_table__insert_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<st_mysqlx_node_table__insert>(
		class_type,
		&mysqlx_object_node_table__insert_handlers,
		&mysqlx_node_table__insert_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_table__insert_class */
void
mysqlx_register_node_table__insert_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_table__insert_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_table__insert_handlers.free_obj = mysqlx_node_table__insert_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeTableInsert", mysqlx_node_table__insert_methods);
		tmp_ce.create_object = php_mysqlx_node_table__insert_object_allocator;
		mysqlx_node_table__insert_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_table__insert_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_node_table__insert_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_table__insert_properties, mysqlx_node_table__insert_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_table__insert_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_node_table__insert_class */
void
mysqlx_unregister_node_table__insert_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_table__insert_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_table__insert */
void
mysqlx_new_node_table__insert(zval * return_value,
					XMYSQLND_NODE_TABLE * table,
					const zend_bool clone,
					zval * columns,
					const int num_of_columns)
{
	DBG_ENTER("mysqlx_new_node_table__insert");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_table__insert_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_table__insert * const object = (struct st_mysqlx_node_table__insert *) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->data->m.get_reference(table) : table;
			object->crud_op = xmysqlnd_crud_table_insert__create(
				mnd_str2c(object->table->data->schema->data->schema_name),
				mnd_str2c(object->table->data->table_name),
				columns,
				num_of_columns);
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
