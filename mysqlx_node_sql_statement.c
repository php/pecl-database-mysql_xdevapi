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
#include "mysqlx_node_sql_statement_result.h"
#include "mysqlx_node_sql_statement.h"

static zend_class_entry *mysqlx_node_sql_statement_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__read_result, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

struct st_mysqlx_node_sql_statement
{
	XMYSQLND_NODE_STMT * stmt;
	enum_func_status send_query_status;
	zend_bool has_more;
};


#define MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_sql_statement *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s\n", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_sql_statement::__construct */
PHP_METHOD(mysqlx_node_sql_statement, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::execute(object session) */
PHP_METHOD(mysqlx_node_sql_statement, execute)
{
	zval * object_zv;
	struct st_mysqlx_node_sql_statement * object;

	DBG_ENTER("mysqlx_node_sql_statement::execute");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);
	if (object && object->stmt) {
		object->has_more = FALSE;
		RETVAL_BOOL(PASS == (object->stmt->data->m.send_query(object->stmt, NULL, NULL)));
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::readResult(object session, string query) */
PHP_METHOD(mysqlx_node_sql_statement, readResult)
{
	zval * object_zv;
	struct st_mysqlx_node_sql_statement * object;

	DBG_ENTER("mysqlx_node_sql_statement::readResult");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object && object->stmt) {
		XMYSQLND_NODE_STMT * stmt = object->stmt;
		XMYSQLND_NODE_STMT_RESULT * result = stmt->data->m.read_one_result(stmt, &object->has_more, NULL, NULL);

		DBG_INF_FMT("has_more=%s", object->has_more? "TRUE":"FALSE");
		if (result) {
			mysqlx_new_sql_stmt_result(return_value, result);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */



/* {{{ mysqlx_node_sql_statement_methods[] */
static const zend_function_entry mysqlx_node_sql_statement_methods[] = {
	PHP_ME(mysqlx_node_sql_statement, __construct,	NULL,	ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_sql_statement, execute, arginfo_mysqlx_node_sql_statement__execute, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement, readResult, arginfo_mysqlx_node_sql_statement__read_result, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_sql_statement_handlers;
static HashTable mysqlx_node_sql_statement_properties;

const struct st_mysqlx_property_entry mysqlx_node_sql_statement_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_sql_statement_free_storage */
static void
mysqlx_node_sql_statement_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_sql_statement * inner_obj = (struct st_mysqlx_node_sql_statement *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->stmt) {
			xmysqlnd_node_stmt_free(inner_obj->stmt, NULL, NULL);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_sql_statement_object_allocator */
static zend_object *
php_mysqlx_node_sql_statement_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_sql_statement * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_sql_statement));

	DBG_ENTER("php_mysqlx_node_sql_statement_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_sql_statement_handlers;
	mysqlx_object->properties = &mysqlx_node_sql_statement_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_sql_statement_class */
void
mysqlx_register_node_sql_statement_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_sql_statement_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_sql_statement_handlers.free_obj = mysqlx_node_sql_statement_free_storage;

	{
		zend_class_entry tmp_ce;
//		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_node_sql_statement", mysqlx_node_sql_statement_methods);
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSqlStatement", mysqlx_node_sql_statement_methods);
		tmp_ce.create_object = php_mysqlx_node_sql_statement_object_allocator;
		mysqlx_node_sql_statement_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_sql_statement_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_sql_statement_properties, mysqlx_node_sql_statement_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_sql_statement_class */
void
mysqlx_unregister_node_sql_statement_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_sql_statement_properties);
}
/* }}} */


/* {{{ mysqlx_new_sql_stmt */
void
mysqlx_new_sql_stmt(zval * return_value, XMYSQLND_NODE_STMT * stmt)
{
	struct st_mysqlx_node_sql_statement * object = NULL;
	DBG_ENTER("mysqlx_new_sql_stmt");

	object_init_ex(return_value, mysqlx_node_sql_statement_class_entry);
	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, return_value);

	object->stmt = stmt;
	object->send_query_status = FAIL;
	object->has_more = FALSE;

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
