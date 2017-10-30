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
#include "mysqlx_crud_operation_limitable.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

zend_class_entry * mysqlx_crud_operation_limitable_interface_entry;

ZEND_BEGIN_ARG_INFO_EX(mysqlx_crud_operation_limitable__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

/* {{{ mysqlx_crud_operation_limitable_methods[] */
static const zend_function_entry mysqlx_crud_operation_limitable_methods[] = {
	PHP_ABSTRACT_ME(mysqlx_crud_operation_limitable, limit, mysqlx_crud_operation_limitable__limit)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_register_crud_operation_limitable_interface */
void
mysqlx_register_crud_operation_limitable_interface(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "CrudOperationLimitable", mysqlx_crud_operation_limitable_methods);
	mysqlx_crud_operation_limitable_interface_entry = zend_register_internal_interface(&tmp_ce);
}
/* }}} */


/* {{{ mysqlx_unregister_crud_operation_limitable_interface */
void
mysqlx_unregister_crud_operation_limitable_interface(SHUTDOWN_FUNC_ARGS)
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

