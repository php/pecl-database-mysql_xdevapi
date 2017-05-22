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
#ifndef MYSQLX_NODE_SQL_STATEMENT_RESULT_H
#define MYSQLX_NODE_SQL_STATEMENT_RESULT_H

#include "phputils/allocator.h"

namespace mysqlx {

namespace drv {
struct st_xmysqlnd_node_stmt;
struct st_xmysqlnd_node_stmt_result;
}

namespace devapi {

struct st_mysqlx_node_statement;

struct st_mysqlx_node_sql_statement_result : public phputils::custom_allocable
{
	drv::st_xmysqlnd_node_stmt_result* result;
	drv::st_xmysqlnd_node_stmt* stmt;
	zend_long execute_flags;
	enum_func_status send_query_status;
	zend_bool has_more_results;
	zend_bool has_more_rows_in_set;
};

void mysqlx_new_sql_stmt_result(zval * return_value, drv::st_xmysqlnd_node_stmt_result* result, struct st_mysqlx_node_statement* stmt);
void mysqlx_register_node_sql_statement_result_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_node_sql_statement_result_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_NODE_SQL_STATEMENT_RESULT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
