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
#ifndef XMYSQLND_STMT_H
#define XMYSQLND_STMT_H

#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_msg__sql_stmt_execute */
#include "util/allocator.h"

namespace mysqlx {

namespace drv {

struct xmysqlnd_session;
typedef std::shared_ptr<xmysqlnd_session> XMYSQLND_SESSION;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_stmt_execution_state;
struct st_xmysqlnd_warning_list;
struct st_xmysqlnd_rowset;

struct st_xmysqlnd_stmt_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt);
	void * ctx;
};

struct st_xmysqlnd_stmt_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const struct st_xmysqlnd_stmt_result_meta * const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_statement_ok_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const struct st_xmysqlnd_stmt_execution_state * const exec_state);
	void * ctx;
};

typedef struct st_xmysqlnd_rowset *                                    (*func_xmysqlnd_stmt__create_rowset)(void * ctx);

struct st_xmysqlnd_stmt_bind_ctx
{
	xmysqlnd_stmt* stmt;
	MYSQLND_STATS* stats;
	MYSQLND_ERROR_INFO* error_info;
	func_xmysqlnd_stmt__create_rowset create_rowset;
	size_t fwd_prefetch_count;
	size_t prefetch_counter;
	zval* current_row;
	st_xmysqlnd_rowset* rowset;
	st_xmysqlnd_stmt_result_meta* meta;
	st_xmysqlnd_stmt_result* result;
	st_xmysqlnd_warning_list* warnings;
	st_xmysqlnd_stmt_execution_state* exec_state;

	st_xmysqlnd_stmt_on_row_bind on_row;
	st_xmysqlnd_stmt_on_warning_bind on_warning;
	st_xmysqlnd_stmt_on_error_bind on_error;
	st_xmysqlnd_stmt_on_result_end_bind on_resultset_end;
	st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok;
};

class xmysqlnd_stmt : public util::custom_allocable
{
public:
	xmysqlnd_stmt() = default;
	xmysqlnd_stmt(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const obj_factory,
						 XMYSQLND_SESSION cur_session);
	enum_func_status	send_raw_message(xmysqlnd_stmt * const stmt,
										 const struct st_xmysqlnd_pb_message_shell message_shell,
										 MYSQLND_STATS * const stats,
										 MYSQLND_ERROR_INFO * const error_info);


	enum_func_status	read_one_result(xmysqlnd_stmt * const stmt,
										const struct st_xmysqlnd_stmt_on_row_bind on_row,
										const struct st_xmysqlnd_stmt_on_warning_bind on_warning,
										const struct st_xmysqlnd_stmt_on_error_bind on_error,
										const struct st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
										const struct st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
										zend_bool * const has_more_results,
										MYSQLND_STATS * const stats,
										MYSQLND_ERROR_INFO * const error_info);

	enum_func_status	read_all_results(xmysqlnd_stmt * const stmt,
										 const struct st_xmysqlnd_stmt_on_row_bind on_row,
										const struct st_xmysqlnd_stmt_on_warning_bind on_warning,
										 const struct st_xmysqlnd_stmt_on_error_bind on_error,
										 const struct st_xmysqlnd_stmt_on_result_start_bind on_result_start,
										 const struct st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
										 const struct st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
										 MYSQLND_STATS * const stats,
										 MYSQLND_ERROR_INFO * const error_info);
	zend_bool           has_more_results(xmysqlnd_stmt * stmt);

	st_xmysqlnd_stmt_result *		get_buffered_result(xmysqlnd_stmt * const stmt,
													zend_bool * const has_more_results,
													const struct st_xmysqlnd_stmt_on_warning_bind on_warning,
													const struct st_xmysqlnd_stmt_on_error_bind on_error,
													MYSQLND_STATS * const stats,
													MYSQLND_ERROR_INFO * const error_info);

	st_xmysqlnd_stmt_result *		get_fwd_result(xmysqlnd_stmt * const stmt,
												const size_t rows,
												zend_bool * const has_more_rows_in_set,
												zend_bool * const has_more_results,
												const struct st_xmysqlnd_stmt_on_warning_bind on_warning,
												const struct st_xmysqlnd_stmt_on_error_bind on_error,
												MYSQLND_STATS * const stats,
												MYSQLND_ERROR_INFO * const error_info);

	enum_func_status				skip_one_result(xmysqlnd_stmt * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	enum_func_status				skip_all_results(xmysqlnd_stmt * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	st_xmysqlnd_stmt_result_meta *	create_meta(void * ctx);
	st_xmysqlnd_stmt_execution_state *	create_execution_state(void * ctx);
	st_xmysqlnd_warning_list *			create_warning_list(void * ctx);
	xmysqlnd_stmt *					get_reference(xmysqlnd_stmt * const stmt);
	enum_func_status				free_reference(xmysqlnd_stmt * const stmt);
	void							free_contents(xmysqlnd_stmt * const stmt);
	void							cleanup(xmysqlnd_stmt * const stmt);
	XMYSQLND_SESSION				get_session() {
		return session;
	}
	st_xmysqlnd_msg__sql_stmt_execute& get_msg_stmt_exec() {
		return msg_stmt_exec;
	}
	st_xmysqlnd_stmt_bind_ctx& get_read_ctx() {
		return read_ctx;
	}
	zend_bool get_persistent() {
		return persistent;
	}
private:
	XMYSQLND_SESSION session;
	st_xmysqlnd_msg__sql_stmt_execute msg_stmt_exec;
	st_xmysqlnd_stmt_bind_ctx read_ctx;
	zend_bool         partial_read_started;
	unsigned int	refcount;
	zend_bool		persistent;
public: //To be removed anyway
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;
};


PHP_MYSQL_XDEVAPI_API xmysqlnd_stmt * xmysqlnd_stmt_create(XMYSQLND_SESSION session,
													  const zend_bool persistent,
													  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info);

PHP_MYSQL_XDEVAPI_API void xmysqlnd_stmt_free(xmysqlnd_stmt * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STMT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
