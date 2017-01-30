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
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <ext/standard/url.h>
}
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result_meta.h>
#include <xmysqlnd/xmysqlnd_utils.h>
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_base_session.h"
#include "mysqlx_x_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_session.h"
#include "mysqlx_session.h"

static zend_class_entry *mysqlx_x_session_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

/* {{{ mysqlx_x_session::__construct */
static
PHP_METHOD(mysqlx_x_session, __construct)
{
}
/* }}} */


#define MYSQLX_FETCH_X_SESSION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_session *) mysqlx_object->ptr; \
	if (!(_to) && !(_to)->session) { \
		if ((_to)->closed) { \
			php_error_docref(NULL, E_WARNING, "closed session"); \
		} else { \
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		} \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_throw_exception_from_session_if_needed */
static zend_bool
mysqlx_throw_exception_from_session_if_needed(const XMYSQLND_NODE_SESSION_DATA * const session)
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


/* {{{ mysqlx_x_session_methods[] */
static const zend_function_entry mysqlx_x_session_methods[] = {
	PHP_ME(mysqlx_x_session, __construct, 		NULL, ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};
/* }}} */


static HashTable mysqlx_x_session_properties;

const struct st_mysqlx_property_entry mysqlx_x_session_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_register_x_session_class */
void
mysqlx_register_x_session_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "XSession", mysqlx_x_session_methods);
		mysqlx_x_session_class_entry = zend_register_internal_class_ex(
			&tmp_ce, mysqlx_base_session_class_entry);
	}

	zend_hash_init(&mysqlx_x_session_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_x_session_properties, mysqlx_x_session_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_x_session_class */
void
mysqlx_unregister_x_session_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_x_session_properties);
}
/* }}} */


/* {{{ mysqlx_new_x_session */
enum_func_status
mysqlx_new_x_session(zval * return_value)
{
	DBG_ENTER("mysqlx_new_x_session");
	DBG_RETURN(SUCCESS == object_init_ex(return_value, mysqlx_x_session_class_entry)? PASS:FAIL);
}
/* }}} */

/* {{{ create_new_session */
static
enum_func_status create_new_session(php_url * url,
								zval * return_value)
{
	enum_func_status ret = FAIL;
	size_t set_capabilities = 0;
	size_t client_api_flags = 0;
	MYSQLND_CSTRING empty = {NULL, 0};

	if (PASS == mysqlx_new_node_session(return_value)) {
		XMYSQLND_NODE_SESSION * new_session;
		struct st_mysqlx_session * object = (struct st_mysqlx_session *) Z_MYSQLX_P(return_value)->ptr;

		if (!object && !object->session) {
			if (object->closed) {
				php_error_docref(NULL, E_WARNING, "closed session");
			} else {
				php_error_docref(NULL, E_WARNING, "invalid object of class %s",
								 ZSTR_VAL(Z_MYSQLX_P(return_value)->zo.ce->name)); \
			}
		} else {
			const MYSQLND_CSTRING host = make_mysqlnd_cstr(url->host),
						user = make_mysqlnd_cstr(url->user),
						pass = make_mysqlnd_cstr(url->pass),
						path = make_mysqlnd_cstr(url->path);

			new_session = xmysqlnd_node_session_connect(object->session,
										host,
										user,
										pass,
										path,
										empty, //s_or_p
										url->port,
										set_capabilities,
										client_api_flags);
			if (object->session != new_session) {
				mysqlx_throw_exception_from_session_if_needed(object->session->data);

				object->session->m->close(object->session, XMYSQLND_CLOSE_IMPLICIT);
				if (new_session) {
					php_error_docref(NULL, E_WARNING, "Different object returned");
				}
				object->session = new_session;
			}
			ret = PASS;
		}
	} else {
		zval_ptr_dtor(return_value);
		ZVAL_NULL(return_value);
	}
	return ret;
}
/* }}} */


/* {{{ verify_uri_information */
static
enum_func_status verify_uri_information(INTERNAL_FUNCTION_PARAMETERS,
									const php_url * node_url)
{
	DBG_ENTER("verify_uri_information");
	enum_func_status ret = PASS;
	//host is required
	if( !node_url->host ) {
		DBG_ERR_FMT("Missing required host name!");
		ret = FAIL;
	}
	//Username is required
	if( !node_url->user ) {
		DBG_ERR_FMT("Missing required user name!");
		ret = FAIL;
	}
	DBG_RETURN(ret);
	return ret;
}
/* }}} */


/* {{{ proto bool mysqlx\\mysql_xdevapi__getXSession(string uri_string) */
PHP_FUNCTION(mysql_xdevapi__getXSession)
{
	//Setting ret to FAIL will cause the function to throw and exception
	enum_func_status ret = PASS;
	MYSQLND_CSTRING uri_string = {NULL, 0};

	DBG_ENTER("mysql_xdevapi__getXSessionURI");
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s",
										 &(uri_string.s), &(uri_string.l)))
	{
		DBG_VOID_RETURN;
	}

	if (!uri_string.l) {
		php_error_docref(NULL, E_WARNING, "Empty URI string");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}

	DBG_INF_FMT("URI string: %s\n",
			uri_string.s);

	php_url * node_url = php_url_parse(uri_string.s);

	if( node_url && verify_uri_information( INTERNAL_FUNCTION_PARAM_PASSTHRU,
									node_url ) != FAIL ) {
		//Assign default port number if is missing
		if( !node_url->port ) {
			node_url->port = 33060;
		}

		DBG_INF_FMT("host: %s, port: %d,user: %s,pass: %s,path: %s, query: %s\n",
					 node_url->host, node_url->port,
					 node_url->user, node_url->pass,
					 node_url->path, node_url->query);

		ret = create_new_session(node_url,
								return_value);
	} else {
		RAISE_EXCEPTION(err_msg_uri_string_fail);
	}

	if( node_url ) {
		php_url_free( node_url );
	}

	if( ret == FAIL ) {
		RAISE_EXCEPTION(err_msg_new_session_fail);
	}

	DBG_VOID_RETURN;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

