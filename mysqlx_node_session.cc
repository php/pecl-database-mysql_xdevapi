/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#include <ext/standard/url.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_base_session.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_session.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_node_session_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__sql, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, query, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_session__quote_name, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


#define MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_session*) mysqlx_object->ptr; \
        if (!(_to) && !(_to)->session) { \
		if ((_to)->closed) { \
			php_error_docref(nullptr, E_WARNING, "closed session"); \
		} else { \
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		} \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_throw_exception_from_session_if_needed */
static zend_bool
mysqlx_throw_exception_from_session_if_needed(const XMYSQLND_SESSION_DATA session)
{
	const unsigned int error_num = session->m->get_error_no(session);
	DBG_ENTER("mysqlx_throw_exception_from_session_if_needed");
	if (error_num) {
		MYSQLND_CSTRING sqlstate = { session->m->get_sqlstate(session) , 0 };
		MYSQLND_CSTRING errmsg = { session->m->get_error_str(session) , 0 };
		sqlstate.l = strlen(sqlstate.s);
		errmsg.l = strlen(errmsg.s);
		mysqlx_new_exception(error_num, sqlstate, errmsg);
		DBG_RETURN(TRUE);
	}
	DBG_RETURN(FALSE);
}
/* }}} */



/* {{{ mysqlx_execute_node_session_query */
static void
mysqlx_execute_node_session_query(XMYSQLND_SESSION  session,
								  const MYSQLND_CSTRING namespace_,
								  const MYSQLND_CSTRING query,
								  const zend_long flags,
								  zval * const return_value,
								  const unsigned int argc,
								  const zval * args)
{
	XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
	DBG_ENTER("mysqlx_execute_node_session_query");

	if (stmt) {
		zval stmt_zv;
		ZVAL_UNDEF(&stmt_zv);
		mysqlx_new_sql_stmt(&stmt_zv, stmt, namespace_, query);
		if (Z_TYPE(stmt_zv) == IS_NULL) {
			xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
		}
		if (Z_TYPE(stmt_zv) == IS_OBJECT) {
			zval zv;
			ZVAL_UNDEF(&zv);

			bool found{ false };
			for (unsigned int i{0}; i < argc; ++i) {
				ZVAL_UNDEF(&zv);
				mysqlx_node_sql_statement_bind_one_param(&stmt_zv, &args[i], &zv);
				if (Z_TYPE(zv) == IS_FALSE) {
					found = true;
					break;
				}
				zval_dtor(&zv);
			}
			if( false == found ) {
				ZVAL_UNDEF(&zv);

				mysqlx_node_sql_statement_execute(Z_MYSQLX_P(&stmt_zv), flags, &zv);

				ZVAL_COPY(return_value, &zv);
				zval_dtor(&zv);
			}
		}
		zval_ptr_dtor(&stmt_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::executeSql(string query [[, mixed param]]) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_session, executeSql)
{
	zval* object_zv{nullptr};
	st_mysqlx_session* object{nullptr};
	MYSQLND_CSTRING query = {nullptr, 0};
	zval* args{nullptr};
	int argc{0};

	DBG_ENTER("mysqlx_node_session::executeSql");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os*", &object_zv, mysqlx_node_session_class_entry,
																	   &(query.s), &(query.l),
																	   &args, &argc) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;
	if (!query.l) {
		php_error_docref(nullptr, E_WARNING, "Empty query");
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	if (object->session) {
		mysqlx_execute_node_session_query(object->session, namespace_sql, query, MYSQLX_EXECUTE_FLAG_BUFFERED, return_value, argc, args);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::sql(string query) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_session, sql)
{
	zval* object_zv{nullptr};
	st_mysqlx_session* object{nullptr};
	XMYSQLND_SESSION session;
	MYSQLND_CSTRING query = {nullptr, 0};

	DBG_ENTER("mysqlx_node_session::sql");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_base_session_class_entry,
																	   &(query.s), &(query.l)) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	if (!query.l) {
		php_error_docref(nullptr, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		XMYSQLND_NODE_STMT * const stmt = session->m->create_statement_object(session);
		if (stmt) {
			mysqlx_new_sql_stmt(return_value, stmt, namespace_sql, query);
			if (Z_TYPE_P(return_value) == IS_NULL) {
				xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
				mysqlx_throw_exception_from_session_if_needed(session->data);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_session::quoteName(string query) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_session, quoteName)
{
	zval* object_zv{nullptr};
	st_mysqlx_session* object{nullptr};
	XMYSQLND_SESSION session;
	MYSQLND_CSTRING name = {nullptr, 0};

	DBG_ENTER("mysqlx_node_session::quoteName");
	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os", &object_zv, mysqlx_node_session_class_entry,
																	   &(name.s), &(name.l)) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_SESSION_FROM_ZVAL(object, object_zv);

	if ((session = object->session)) {
		MYSQLND_STRING quoted_name = session->data->m->quote_name(session->data, name);
		RETVAL_STRINGL(quoted_name.s, quoted_name.l);
		if (quoted_name.s) {
			mnd_efree(quoted_name.s);
		}
		mysqlx_throw_exception_from_session_if_needed(session->data);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_session, __construct)
{
}
/* }}} */


/* {{{ mysqlx_node_session_methods[] */
static const zend_function_entry mysqlx_node_session_methods[] = {
	PHP_ME(mysqlx_node_session, __construct, 	nullptr, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_session, executeSql,		nullptr, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, sql,			arginfo_mysqlx_node_session__sql, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_session, quoteName,		arginfo_mysqlx_node_session__quote_name, ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static HashTable mysqlx_node_session_properties;

const struct st_mysqlx_property_entry mysqlx_node_session_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};


/* {{{ mysqlx_register_node_session_class */
void
mysqlx_register_node_session_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Session", mysqlx_node_session_methods);
		mysqlx_node_session_class_entry = zend_register_internal_class_ex(
			&tmp_ce, mysqlx_base_session_class_entry);
	}

	zend_hash_init(&mysqlx_node_session_properties, 0, nullptr, mysqlx_free_property_cb, 1);

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


/* {{{ mysqlx_new_node_session */
enum_func_status
mysqlx_new_node_session(zval * return_value)
{
	DBG_ENTER("mysqlx_new_node_session");
	DBG_RETURN(SUCCESS == object_init_ex(return_value, mysqlx_node_session_class_entry)? PASS:FAIL);
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

