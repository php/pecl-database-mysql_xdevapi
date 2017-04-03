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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_SESSION_CONFIG_H
#define MYSQLX_SESSION_CONFIG_H

#include <utility>
#include "phputils/strings.h"
#include "phputils/types.h"

namespace mysqlx {

namespace devapi {

void mysqlx_register_node_session_config_manager_class(INIT_FUNC_ARGS,
					zend_object_handlers * mysqlx_std_object_handlers);

void mysqlx_unregister_node_session_config_manager_class(SHUTDOWN_FUNC_ARGS);

void mysqlx_register_node_session_config_class(INIT_FUNC_ARGS,
					zend_object_handlers * mysqlx_std_object_handlers);

void mysqlx_unregister_node_session_config_class(SHUTDOWN_FUNC_ARGS);

bool istanceof_session_config(zval* object);

PHP_FUNCTION(mysql_xdevapi__sessions);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_SESSION_CONFIG_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
