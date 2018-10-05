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
#include "mysqlnd_api.h"
extern "C" {
#include <ext/standard/url.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_x_session.h"
#include "mysqlx_schema.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_session.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_x_session_class_entry;

/* {{{ mysqlx_x_session::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_x_session, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
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
mysqlx_register_x_session_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * /*mysqlx_std_object_handlers*/)
{
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "XSession", mysqlx_x_session_methods);
		mysqlx_x_session_class_entry = zend_register_internal_class_ex(
			&tmp_ce, mysqlx_session_class_entry);
	}

	zend_hash_init(&mysqlx_x_session_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_x_session_properties, mysqlx_x_session_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_x_session_class */
void
mysqlx_unregister_x_session_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_x_session_properties);
}
/* }}} */


/* {{{ proto bool mysqlx\\mysql_xdevapi__getXSession( ) */
MYSQL_XDEVAPI_PHP_FUNCTION(mysql_xdevapi__getXSession)
{
	util::string_view uri_string = {nullptr, 0};

	RETVAL_NULL();

	DBG_ENTER("mysql_xdevapi__getXSession");
	if (FAILURE == util::zend::parse_function_parameters(execute_data, "s",
										 &(uri_string.str), &(uri_string.len)))
	{
		DBG_VOID_RETURN;
	}

	if (uri_string.empty()) {
		php_error_docref(nullptr, E_WARNING, "Empty URI string");
		DBG_VOID_RETURN;
	}

	drv::xmysqlnd_new_session_connect(uri_string.str,return_value);

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

