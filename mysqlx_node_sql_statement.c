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

#define MAX_STMT_PARAMS 1000

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__bind, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, param_no, IS_LONG, DONT_ALLOW_NULL)
	ZEND_ARG_INFO(NO_PASS_BY_REF, param)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__execute, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, flags, IS_LONG, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__has_more_results, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_sql_statement__get_result, 0, ZEND_RETURN_VALUE, 0)
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
static
PHP_METHOD(mysqlx_node_sql_statement, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::bind(object statement, int param_no, mixed value) */
static
PHP_METHOD(mysqlx_node_sql_statement, bind)
{
	struct st_mysqlx_node_sql_statement * object;
	zval * object_zv;
	zend_long param_no;
	zval * param_zv;

	DBG_ENTER("mysqlx_node_sql_statement::bind");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Olz",
												&object_zv, mysqlx_node_sql_statement_class_entry,
												&param_no, &param_zv))
	{
		DBG_VOID_RETURN;
	}
	if (param_no >= MAX_STMT_PARAMS) {
		php_error_docref(NULL, E_WARNING, "param_no too big. Allowed are %", MAX_STMT_PARAMS);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);
	RETVAL_TRUE;
	if (TRUE == object->in_execution) {
		php_error_docref(NULL, E_WARNING, "Statement in execution. Please fetch all data first.");
	} else if (object->stmt) {
		object->stmt->data->m.bind_one_param(object->stmt, param_no, param_zv);
	}
//	Z_TRY_ADDREF_P(object_zv);
//	RETVAL_ZVAL(object_zv, 1 /*copy*/, 0 /*dtor*/);

	DBG_VOID_RETURN;
}
/* }}} */


#define MYSQLX_EXECUTE_FWD_PREFETCH_COUNT 3

/* {{{ proto mixed mysqlx_node_sql_statement::execute(object statement, int flags) */
static
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
	DBG_INF_FMT("%s", (flags & MYSQLX_EXECUTE_FLAG_BUFFERED)? "BUFFERED":"FWD");

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
				XMYSQLND_NODE_STMT_RESULT * result;
				if (object->execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
					result = object->stmt->data->m.get_buffered_result(stmt, &object->has_more_results, NULL, NULL);
				} else {
					result = object->stmt->data->m.get_fwd_result(stmt, MYSQLX_EXECUTE_FWD_PREFETCH_COUNT, &object->has_more_rows_in_set, &object->has_more_results, NULL, NULL);
				}

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


/* {{{ proto mixed mysqlx_node_sql_statement::hasMoreResults(object statement) */
static
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
	DBG_INF_FMT("%s", Z_TYPE_P(return_value) == IS_TRUE? "YES":"NO");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_sql_statement_read_result */
static void mysqlx_node_sql_statement_read_result(INTERNAL_FUNCTION_PARAMETERS)
{
	struct st_mysqlx_node_sql_statement * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_sql_statement::getResult");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_sql_statement_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SQL_STATEMENT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (PASS == object->send_query_status) {
		XMYSQLND_NODE_STMT * stmt = object->stmt;
		XMYSQLND_NODE_STMT_RESULT * result;

		if (object->execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
			result = object->stmt->data->m.get_buffered_result(stmt, &object->has_more_results, NULL, NULL);
		} else {
			result = object->stmt->data->m.get_fwd_result(stmt, MYSQLX_EXECUTE_FWD_PREFETCH_COUNT, &object->has_more_rows_in_set, &object->has_more_results, NULL, NULL);
		}

		DBG_INF_FMT("result=%p  has_more_results=%s", result, object->has_more_results? "TRUE":"FALSE");
		if (result) {
			mysqlx_new_sql_stmt_result(return_value, result);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ proto mixed mysqlx_node_sql_statement::readResult(object session) */
static
PHP_METHOD(mysqlx_node_sql_statement, getResult)
{
	mysqlx_node_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_sql_statement::readResult(object session) */
static
PHP_METHOD(mysqlx_node_sql_statement, getNextResult)
{
	mysqlx_node_sql_statement_read_result(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */



/* {{{ mysqlx_node_sql_statement_methods[] */
static const zend_function_entry mysqlx_node_sql_statement_methods[] = {
	PHP_ME(mysqlx_node_sql_statement, __construct,		NULL,													ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_sql_statement, bind,				arginfo_mysqlx_node_sql_statement__bind,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement, execute,			arginfo_mysqlx_node_sql_statement__execute,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement, hasMoreResults,	arginfo_mysqlx_node_sql_statement__has_more_results,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement, getResult,		arginfo_mysqlx_node_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_sql_statement, getNextResult,	arginfo_mysqlx_node_sql_statement__get_result, 			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_node_sql_statement_property__statement */
static zval *
mysqlx_node_sql_statement_property__statement(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_sql_statement * object = (const struct st_mysqlx_node_sql_statement *) (obj->ptr);
	DBG_ENTER("mysqlx_node_sql_statement_property__statement");
	if (object->stmt && object->stmt->data->query.s) {
		ZVAL_STRINGL(return_value, object->stmt->data->query.s, object->stmt->data->query.l);
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


static zend_object_handlers mysqlx_object_node_sql_statement_handlers;
static HashTable mysqlx_node_sql_statement_properties;

const struct st_mysqlx_property_entry mysqlx_node_sql_statement_property_entries[] =
{
	{{"statement",	sizeof("statement") - 1}, mysqlx_node_sql_statement_property__statement,	NULL},
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

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_sql_statement_class_entry, "statement",	sizeof("statement") - 1, ZEND_ACC_PUBLIC);

	zend_declare_class_constant_long(mysqlx_node_sql_statement_class_entry, "EXECUTE_ASYNC", sizeof("EXECUTE_ASYNC") - 1, MYSQLX_EXECUTE_FLAG_ASYNC);
	zend_declare_class_constant_long(mysqlx_node_sql_statement_class_entry, "BUFFERED", sizeof("BUFFERED") - 1, MYSQLX_EXECUTE_FLAG_BUFFERED);
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
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
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
