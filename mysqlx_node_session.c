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
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_sql_statement.h"


static zend_class_entry *mysqlx_node_session_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__connect, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_TYPE_INFO(0, hostname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, username, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__query, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__create_statement, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

struct st_mysqlx_node_session
{
	XMYSQLND_NODE_SESSION * session;
};


#define MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_session *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s\n", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ proto mixed mysqlx_node_session::query(object session, string query) */
PHP_METHOD(mysqlx_node_session, query)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::query");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;

	{
		MYSQLND_STATS * stats = session->data->stats;
		MYSQLND_ERROR_INFO * error_info = session->data->error_info;
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_IMPLICIT);
		if (stmt) {
			if (PASS == stmt->data->m.send_query(stmt, stats, error_info)) {
				zend_bool has_more = FALSE;
				do {
					XMYSQLND_NODE_STMT_RESULT * result = stmt->data->m.read_one_result(stmt, &has_more, stats, error_info);
					xmysqlnd_node_stmt_result_free(result, stats, error_info);
					DBG_INF_FMT("has_more=%s", has_more? "TRUE":"FALSE");
				} while (has_more == TRUE);
			}
			xmysqlnd_node_stmt_free(stmt, stats, error_info);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::query_and_discard(object session, string query) */
PHP_METHOD(mysqlx_node_session, query_and_discard)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::query_and_discard");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;
	{
		MYSQLND_STATS * stats = session->data->stats;
		MYSQLND_ERROR_INFO * error_info = session->data->error_info;
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_IMPLICIT);
		if (stmt) {
			if (PASS == stmt->data->m.send_query(stmt, stats, error_info)) {
				stmt->data->m.skip_all_results(stmt, stats, error_info);
			}
			xmysqlnd_node_stmt_free(stmt, stats, error_info);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::createStatement(object session, string query) */
PHP_METHOD(mysqlx_node_session, createStatement)
{
	zval * object_zv;
	struct st_mysqlx_node_session * object;
	XMYSQLND_NODE_SESSION * session;
	MYSQLND_CSTRING query = {NULL, 0};

	DBG_ENTER("mysqlx_node_session::createStatement");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry, &(query.s), &(query.l)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	session = object->session;

	{
		XMYSQLND_NODE_STMT * stmt = session->data->m->create_statement(session->data, query, MYSQLND_SEND_QUERY_EXPLICIT);
		if (stmt) {
			mysqlx_new_sql_stmt(return_value, stmt);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::__construct */
PHP_METHOD(mysqlx_node_session, __construct)
{
}
/* }}} */


/* {{{ mysqlx_node_session_methods[] */
static const zend_function_entry mysqlx_node_session_methods[] = {
	PHP_ME(mysqlx_node_session, __construct, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_session, query, arginfo_mysqlx_node_session__query, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, query_and_discard, arginfo_mysqlx_node_session__query, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, createStatement, arginfo_mysqlx_node_session__create_statement, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_session_handlers;
static HashTable mysqlx_node_session_properties;

const struct st_mysqlx_property_entry mysqlx_node_session_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_session_free_storage */
static void
mysqlx_node_session_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_session * inner_obj = (struct st_mysqlx_node_session *) mysqlx_object->ptr;

	if (inner_obj) {
		XMYSQLND_NODE_SESSION * session = (XMYSQLND_NODE_SESSION *) inner_obj->session;

		if (session) {
			session->m->close(session, XMYSQLND_CLOSE_EXPLICIT);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_session_object_allocator */
static zend_object *
php_mysqlx_node_session_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_session * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_session));

	DBG_ENTER("php_mysqlx_node_session_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	if (!(object->session = xmysqlnd_node_session_init(0, FALSE, NULL))) {
		mnd_efree(object);
		mnd_efree(mysqlx_object);
		DBG_RETURN(NULL);
	}

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_session_handlers;
	mysqlx_object->properties = &mysqlx_node_session_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_session_class */
void
mysqlx_register_node_session_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_session_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_session_handlers.free_obj = mysqlx_node_session_free_storage;

	{
		zend_class_entry tmp_ce;
//		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_node_session", mysqlx_node_session_methods);
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSession", mysqlx_node_session_methods);
		tmp_ce.create_object = php_mysqlx_node_session_object_allocator;
		mysqlx_node_session_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_session_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_session_properties, mysqlx_node_session_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_session_class */
void
mysqlx_unregister_node_session_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_session_properties);
}
/* }}} */


/* {{{ mysqlx_new_sql_stmt */
enum_func_status
mysqlx_new_node_session(zval * return_value)
{
	DBG_ENTER("mysqlx_new_sql_stmt");
	DBG_RETURN(SUCCESS == object_init_ex(return_value, mysqlx_node_session_class_entry)? PASS:FAIL);
}
/* }}} */


/* {{{ proto bool getNodeSession(string hostname, string username, string password, int port)
   Bind variables to a prepared statement as parameters */
PHP_FUNCTION(mysqlx__getNodeSession)
{
	MYSQLND_CSTRING hostname = {NULL, 0};
	MYSQLND_CSTRING username = {NULL, 0};
	MYSQLND_CSTRING password = {NULL, 0};
	MYSQLND_CSTRING empty = {NULL, 0};
	zend_long port = 0;
	size_t set_capabilities = 0;
	size_t client_api_flags = 0;

	DBG_ENTER("mysqlx_node_session::connect");
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "sss|l",
										 &(hostname.s), &(hostname.l), 
										 &(username.s), &(username.l),
										 &(password.s), &(password.l),
										 &port))
	{
		DBG_VOID_RETURN;
	}

	if (!hostname.l) {
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	if (!port) {
		port = 33060;
	}

	RETVAL_FALSE;

	if (PASS == mysqlx_new_node_session(return_value)) {
		struct st_mysqlx_node_session * object;
		XMYSQLND_NODE_SESSION * new_session;
		MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, return_value);

		new_session = xmysqlnd_node_session_connect(object->session, hostname, username, password,
													empty /*db*/, empty /*s_or_p*/, port, set_capabilities, client_api_flags);

		if (object->session != new_session) {
			php_error_docref(NULL, E_WARNING, "Different object returned");
			if (object->session) {
				object->session->m->close(object->session, XMYSQLND_CLOSE_IMPLICIT);
			}
			object->session = new_session;
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

