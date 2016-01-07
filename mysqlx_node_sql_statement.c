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
#include "mysqlx_node_session.h"

static zend_class_entry *mysqlx_node_sql_statement_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__has_more_results, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__read_result, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

struct st_mysqlx_node_sql_statement
{
	XMYSQLND_NODE_STMT * stmt;
	zend_long execute_flags;
	enum_func_status send_query_status;
	zend_bool in_execution;
	zend_bool has_more_results;
	zend_bool has_more_rows_in_set;
};


#define MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_sql_statement *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->stmt) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_sql_statement::__construct */
PHP_METHOD(mysqlx_node_sql_statement, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::execute(object session, int flags) */
PHP_METHOD(mysqlx_node_sql_statement, execute)
{
	struct st_mysqlx_node_sql_statement * object;
	zval * object_zv;
	zend_long flags = 0;

	DBG_ENTER("mysqlx_node_sql_statement::execute");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
												&object_zv, mysqlx_node_sql_statement_class_entry,
												&flags))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if ((flags | MYSQLX_EXECUTE_ALL_FLAGS) != MYSQLX_EXECUTE_ALL_FLAGS) {
		php_error_docref(NULL, E_WARNING, "Invalid flags. Unknown %lu", flags - (flags | MYSQLX_EXECUTE_ALL_FLAGS));
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("flags=%lu", flags);
	DBG_INF_FMT("%sSYNC", (flags & MYSQLX_EXECUTE_FLAG_ASYNC)? "A":"");

	if (TRUE == object->in_execution) {
		php_error_docref(NULL, E_WARNING, "Statement in execution. Please fetch all data first.");
	} else {
		XMYSQLND_NODE_STMT * stmt = object->stmt;
		object->execute_flags = flags;
		object->has_more_rows_in_set = FALSE;
		object->has_more_results = FALSE;
		object->send_query_status = stmt->data->m.send_query(stmt, NULL, NULL);

		if (PASS == object->send_query_status) {
			if (object->execute_flags & MYSQLX_EXECUTE_FLAG_ASYNC) {
				DBG_INF("ASYNC");
				RETVAL_BOOL(PASS == object->send_query_status);
			} else {
				XMYSQLND_NODE_STMT_RESULT * result = object->stmt->data->m.read_one_result(stmt, &object->has_more_results, NULL, NULL);

				DBG_INF_FMT("has_more_results=%s   has_more_rows_in_set=%s",
							object->has_more_results? "TRUE":"FALSE",
							object->has_more_rows_in_set? "TRUE":"FALSE");

				if (result) {
					mysqlx_new_sql_stmt_result(return_value, result);
				} else {
					/* Or we should close the connection, rendering it unusable at this point ?*/
					object->send_query_status = FAIL;
				}
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::hasMoreResults(object session) */
PHP_METHOD(mysqlx_node_sql_statement, hasMoreResults)
{
	struct st_mysqlx_node_sql_statement * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_sql_statement::hasMoreResults");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);

	RETVAL_BOOL(object->stmt->data->m.has_more_results(object->stmt));

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::readResult(object session) */
PHP_METHOD(mysqlx_node_sql_statement, readResult)
{
	struct st_mysqlx_node_sql_statement * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_sql_statement::readResult");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (PASS == object->send_query_status) {
		XMYSQLND_NODE_STMT * stmt = object->stmt;
		XMYSQLND_NODE_STMT_RESULT * result = stmt->data->m.read_one_result(stmt, &object->has_more_results, NULL, NULL);

		DBG_INF_FMT("has_more_results=%s", object->has_more_results? "TRUE":"FALSE");
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
	PHP_ME(mysqlx_node_sql_statement, hasMoreResults, arginfo_mysqlx_node_sql_statement__has_more_results, ZEND_ACC_PUBLIC)
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
	struct st_mysqlx_object * mysqlx_object;
	struct st_mysqlx_node_sql_statement * object = NULL;
	DBG_ENTER("mysqlx_new_sql_stmt");

	object_init_ex(return_value, mysqlx_node_sql_statement_class_entry);

	mysqlx_object = Z_MYSQLX_P(return_value);
	object = (struct st_mysqlx_node_sql_statement *) mysqlx_object->ptr;
	if (!object) {
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN;
	}

	object->stmt = stmt;
	object->execute_flags = 0;
	object->send_query_status = FAIL;
	object->in_execution = FALSE;
	object->has_more_results = FALSE;
	object->has_more_rows_in_set = FALSE;

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
