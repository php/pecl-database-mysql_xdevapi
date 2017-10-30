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
#include "php_api.h"
extern "C" {
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_enum_n_def.h"
#include "ext/mysqlnd/mysqlnd_structs.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_priv.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_driver_class_entry;

/* {{{ mysqlx_node_session::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_driver, __construct)
{
}
/* }}} */


/* {{{ mysqlx_driver_methods[] */
static const zend_function_entry mysqlx_driver_methods[] = {
	PHP_ME(mysqlx_driver, __construct, NULL, ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_register_driver_class */
void
mysqlx_register_driver_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Driver", mysqlx_driver_methods);
	mysqlx_driver_class_entry = zend_register_internal_class(&tmp_ce);
	mysqlx_driver_class_entry->ce_flags |= ZEND_ACC_FINAL; /* Forbid extension of the driver */

	zend_declare_class_constant_stringl(mysqlx_driver_class_entry, "version", sizeof("version") - 1, PHP_MYSQL_XDEVAPI_VERSION, sizeof(PHP_MYSQL_XDEVAPI_VERSION) - 1);
}
/* }}} */


/* {{{ mysqlx_unregister_driver_class */
void
mysqlx_unregister_driver_class(SHUTDOWN_FUNC_ARGS)
{
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
