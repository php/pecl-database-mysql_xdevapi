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
#include "xmysqlnd/xmysqlnd.h"
#include <ext/standard/url.h>
}
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
#include "mysqlx_x_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_session.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_x_session_class_entry;

/* {{{ mysqlx_x_session::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_x_session, __construct)
{
}
/* }}} */


#define MYSQLX_FETCH_X_SESSION_FROM_ZVAL(_to, _from) \
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


/* {{{ mysqlx_x_session_methods[] */
static const zend_function_entry mysqlx_x_session_methods[] = {
	PHP_ME(mysqlx_x_session, __construct, 		nullptr, ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static HashTable mysqlx_x_session_properties;

const struct st_mysqlx_property_entry mysqlx_x_session_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
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

	zend_hash_init(&mysqlx_x_session_properties, 0, nullptr, mysqlx_free_property_cb, 1);

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


/* {{{ proto bool mysqlx\\mysql_xdevapi__getXSession( ) */
MYSQL_XDEVAPI_PHP_FUNCTION(mysql_xdevapi__getXSession)
{
	util::string_view uri_string = {nullptr, 0};

	RETVAL_NULL();

	DBG_ENTER("mysql_xdevapi__getXSession");
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s",
										 &(uri_string.str), &(uri_string.len)))
	{
		DBG_VOID_RETURN;
	}

	if (uri_string.empty()) {
		php_error_docref(nullptr, E_WARNING, "Empty URI string");
		DBG_VOID_RETURN;
	}

	drv::xmysqlnd_node_new_session_connect(uri_string.str,return_value);

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

