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
#ifndef MYSQLX_NODE_SQL_STATEMENT_H
#define MYSQLX_NODE_SQL_STATEMENT_H

struct st_mysqlx_object;
struct st_xmysqlnd_node_stmt;

enum mysqlx_execute_flags
{
	MYSQLX_EXECUTE_FLAG_ASYNC = 1 << 0,
	MYSQLX_EXECUTE_FLAG_BUFFERED = 1 << 1,
	MYSQLX_EXECUTE_FLAG_CALLBACKS = 1 << 2,
};

enum mysqlx_result_type
{
	MYSQLX_RESULT = 1 << 0,
	MYSQLX_RESULT_DOC = 1 << 1,
	MYSQLX_RESULT_ROW = 1 << 2,
	MYSQLX_RESULT_SQL = 1 << 3,
};

#define MYSQLX_EXECUTE_ALL_FLAGS	(0 | MYSQLX_EXECUTE_FLAG_ASYNC | MYSQLX_EXECUTE_FLAG_BUFFERED)
#define MYSQLX_EXECUTE_FWD_PREFETCH_COUNT 100

struct st_mysqlx_node_statement
{
	XMYSQLND_NODE_STMT * stmt;
	XMYSQLND_STMT_OP__EXECUTE * stmt_execute;
	struct st_xmysqlnd_pb_message_shell * pb_shell;
	zend_long execute_flags;
	enum_func_status send_query_status;
	zend_bool in_execution;
	zend_bool has_more_results;
	zend_bool has_more_rows_in_set;
};

void mysqlx_register_node_statement_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_node_statement_class(SHUTDOWN_FUNC_ARGS);

void mysqlx_new_node_stmt(zval * return_value, struct st_xmysqlnd_node_stmt * stmt);
void mysqlx_node_statement_execute_read_response(const struct st_mysqlx_object * const mysqlx_object, const zend_long flags, const enum mysqlx_result_type result_type, zval * return_value);

/**********/

void mysqlx_register_node_sql_statement_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_node_sql_statement_class(SHUTDOWN_FUNC_ARGS);

void mysqlx_new_sql_stmt(zval * return_value, struct st_xmysqlnd_node_stmt * stmt, const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING query);
void mysqlx_node_sql_statement_bind_one_param(zval * object_zv, const zval * param_zv, zval * return_value);
void mysqlx_node_sql_statement_execute(const struct st_mysqlx_object * const mysqlx_object, const zend_long flags, zval * return_value);

#endif /* MYSQLX_NODE_SQL_STATEMENT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
