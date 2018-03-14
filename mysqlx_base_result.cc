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
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_rowset.h"
#include "xmysqlnd/xmysqlnd_rowset_buffered.h"
#include "xmysqlnd/xmysqlnd_rowset_fwd.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_base_result_iterator.h"
#include "mysqlx_base_result.h"
#include "mysqlx_field_metadata.h"

namespace mysqlx {

namespace devapi {

zend_class_entry* mysqlx_base_result_interface_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_result__get_warning_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_base_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_base_result_methods[] */
static const zend_function_entry mysqlx_base_result_methods[] = {
	//PHP_ME(mysqlx_base_result, __construct,			nullptr,																ZEND_ACC_PRIVATE)
	PHP_ABSTRACT_ME(mysqlx_base_result, getWarningCount,		arginfo_mysqlx_base_result__get_warning_count)
	PHP_ABSTRACT_ME(mysqlx_base_result, getWarnings,			arginfo_mysqlx_base_result__get_warnings)

	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_register_base_result_interface */
void
mysqlx_register_base_result_interface(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "BaseResult", mysqlx_base_result_methods);
	mysqlx_base_result_interface_entry = zend_register_internal_interface(&tmp_ce);
}
/* }}} */

/* {{{ mysqlx_unregister_node_base_result_interface */
void
mysqlx_unregister_node_base_result_interface(SHUTDOWN_FUNC_ARGS)
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
