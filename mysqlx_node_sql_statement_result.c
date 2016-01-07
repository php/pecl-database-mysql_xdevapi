/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include <php.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

static zend_class_entry *mysqlx_node_sql_statement_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement_result__has_data, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_sql_statement_result
{
	XMYSQLND_NODE_STMT_RESULT * result;
};


#define MYSQLX_FETCH_NODE_SQL_STATEMENT_RESULT_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_sql_statement_result *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__query, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* {{{ mysqlx_node_sql_statement_result::__construct */
PHP_METHOD(mysqlx_node_sql_statement_result, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement_result::hasData(object session) */
PHP_METHOD(mysqlx_node_sql_statement_result, hasData)
{
	zval * object_zv;
	struct st_mysqlx_node_sql_statement_result * object;

	DBG_ENTER("mysqlx_node_sql_statement_result::hasData");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_BOOL(object && object->result && FALSE == object->result->data->m.eof(object->result));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement_result::fetchOne(object session) */
PHP_METHOD(mysqlx_node_sql_statement_result, fetchOne)
{
	zval * object_zv;
	struct st_mysqlx_node_sql_statement_result * object;

	DBG_ENTER("mysqlx_node_sql_statement_result::fetchOne");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object && object->result && FALSE == object->result->data->m.eof(object->result)) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == object->result->data->m.fetch_current(object->result, &row, NULL, NULL)) {
			ZVAL_COPY_VALUE(return_value, &row);
			object->result->data->m.next(object->result, NULL, NULL);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement_result::fetchAll(object session) */
PHP_METHOD(mysqlx_node_sql_statement_result, fetchAll)
{
	zval * object_zv;
	struct st_mysqlx_node_sql_statement_result * object;

	DBG_ENTER("mysqlx_node_sql_statement_result::fetchAll");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);
	
	RETVAL_FALSE;
	if (object && object->result) {
		zval set;
		ZVAL_UNDEF(&set);
		if (PASS == object->result->data->m.fetch_all(object->result, &set, NULL, NULL)) {
			ZVAL_COPY_VALUE(return_value, &set);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */



/* {{{ mysqlx_node_sql_statement_result_methods[] */
static const zend_function_entry mysqlx_node_sql_statement_result_methods[] = {
	PHP_ME(mysqlx_node_sql_statement_result, __construct,	NULL,	ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_sql_statement_result, hasData, arginfo_mysqlx_node_sql_statement_result__has_data, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement_result, fetchOne, arginfo_mysqlx_node_sql_statement_result__fetch_one, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement_result, fetchAll, arginfo_mysqlx_node_sql_statement_result__fetch_all, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_sql_statement_result_handlers;
static HashTable mysqlx_node_sql_statement_result_properties;

const struct st_mysqlx_property_entry mysqlx_node_sql_statement_result_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_sql_statement_result_free_storage */
static void
mysqlx_node_sql_statement_result_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_sql_statement_result * inner_obj = (struct st_mysqlx_node_sql_statement_result *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_node_stmt_result_free(inner_obj->result, NULL, NULL);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_sql_statement_result_object_allocator */
static zend_object *
php_mysqlx_node_sql_statement_result_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_sql_statement_result * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_sql_statement_result));

	DBG_ENTER("php_mysqlx_node_sql_statement_result_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_sql_statement_result_handlers;
	mysqlx_object->properties = &mysqlx_node_sql_statement_result_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_sql_statement_result_class */
void
mysqlx_register_node_sql_statement_result_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_sql_statement_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_sql_statement_result_handlers.free_obj = mysqlx_node_sql_statement_result_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSqlStatementResult", mysqlx_node_sql_statement_result_methods);
		tmp_ce.create_object = php_mysqlx_node_sql_statement_result_object_allocator;
		mysqlx_node_sql_statement_result_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_sql_statement_result_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_sql_statement_result_properties, mysqlx_node_sql_statement_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_sql_statement_result_class */
void
mysqlx_unregister_node_sql_statement_result_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_sql_statement_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_sql_stmt_result */
void
mysqlx_new_sql_stmt_result(zval * return_value, XMYSQLND_NODE_STMT_RESULT * result)
{
	struct st_mysqlx_node_sql_statement_result * object = NULL;
	DBG_ENTER("mysqlx_new_sql_stmt_result");

	object_init_ex(return_value, mysqlx_node_sql_statement_result_class_entry);
	MYSQLX_FETCH_NODE_SQL_STATEMENT_RESULT_FROM_ZVAL(object, return_value);

	object->result = result;

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
