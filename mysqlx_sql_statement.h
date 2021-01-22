/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#ifndef MYSQLX_SQL_STATEMENT_H
#define MYSQLX_SQL_STATEMENT_H

#include "util/allocator.h"

namespace mysqlx {

namespace drv {
class xmysqlnd_stmt;
}

namespace devapi {

struct st_mysqlx_object;

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

struct st_mysqlx_statement : public util::custom_allocable
{
	~st_mysqlx_statement();
	drv::xmysqlnd_stmt* stmt;
	drv::XMYSQLND_STMT_OP__EXECUTE* stmt_execute;
	drv::st_xmysqlnd_pb_message_shell* pb_shell;
	zend_long execute_flags;
	enum_func_status send_query_status;
	zend_bool in_execution;
	zend_bool has_more_results;
	zend_bool has_more_rows_in_set;
};

void mysqlx_register_statement_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_statement_class(SHUTDOWN_FUNC_ARGS);

util::zvalue create_stmt(drv::xmysqlnd_stmt* stmt);
util::zvalue mysqlx_statement_execute_read_response(
	const st_mysqlx_object* const mysqlx_object,
	const zend_long flags,
	const mysqlx_result_type result_type);

/**********/

void mysqlx_register_sql_statement_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_sql_statement_class(SHUTDOWN_FUNC_ARGS);

util::zvalue create_sql_stmt(drv::xmysqlnd_stmt* stmt, const std::string_view& namespace_, const util::string_view& query);
bool mysqlx_sql_statement_bind_one_param(util::raw_zval* object_zv, const util::zvalue& param);
util::zvalue mysqlx_sql_statement_execute(const st_mysqlx_object* const mysqlx_object, const zend_long flags);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_SQL_STATEMENT_H */
