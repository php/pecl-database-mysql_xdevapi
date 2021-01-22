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
#ifndef MYSQLX_SQL_STATEMENT_RESULT_H
#define MYSQLX_SQL_STATEMENT_RESULT_H

#include "util/allocator.h"
#include "util/value.h"

namespace mysqlx {

namespace drv {
class xmysqlnd_stmt;
struct st_xmysqlnd_stmt_result;
}

namespace devapi {

struct st_mysqlx_statement;

struct st_mysqlx_sql_statement_result : public util::custom_allocable
{
	~st_mysqlx_sql_statement_result();
	drv::st_xmysqlnd_stmt_result* result;
	drv::xmysqlnd_stmt* stmt;
	zend_long execute_flags;
	enum_func_status send_query_status;
	zend_bool has_more_results;
	zend_bool has_more_rows_in_set;
};

util::zvalue create_sql_stmt_result(drv::st_xmysqlnd_stmt_result* result, st_mysqlx_statement* stmt);
void mysqlx_register_sql_statement_result_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_sql_statement_result_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_SQL_STATEMENT_RESULT_H */
