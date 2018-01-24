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
#ifndef XMYSQLND_NODE_STMT_H
#define XMYSQLND_NODE_STMT_H

#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_msg__sql_stmt_execute */
#include "util/allocator.h"

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_node_session;
struct st_xmysqlnd_node_stmt_result;
struct st_xmysqlnd_stmt_execution_state;
struct st_xmysqlnd_warning_list;
struct st_xmysqlnd_rowset;

typedef struct st_xmysqlnd_node_stmt		XMYSQLND_NODE_STMT;
typedef struct st_xmysqlnd_node_stmt_data	XMYSQLND_NODE_STMT_DATA;


struct st_xmysqlnd_node_stmt_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt);
	void * ctx;
};

struct st_xmysqlnd_node_stmt_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt, const struct st_xmysqlnd_node_stmt_result_meta * const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_node_stmt_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_node_stmt_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_node_stmt_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_node_stmt_on_statement_ok_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_STMT * const stmt, const struct st_xmysqlnd_stmt_execution_state * const exec_state);
	void * ctx;
};


typedef enum_func_status	(*func_xmysqlnd_node_stmt__init)(XMYSQLND_NODE_STMT * const stmt,
															 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
															 struct st_xmysqlnd_node_session * const session,
															 MYSQLND_STATS * const stats,
															 MYSQLND_ERROR_INFO * const error_info);

typedef enum_func_status	(*func_xmysqlnd_node_stmt__send_raw_message)(XMYSQLND_NODE_STMT * const stmt,
																		 const struct st_xmysqlnd_pb_message_shell message_shell,
																		 MYSQLND_STATS * const stats,
																		 MYSQLND_ERROR_INFO * const error_info);


typedef enum_func_status	(*func_xmysqlnd_node_stmt__read_one_result)(XMYSQLND_NODE_STMT * const stmt,
																		const struct st_xmysqlnd_node_stmt_on_row_bind on_row,
																		const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
																		const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
																		const struct st_xmysqlnd_node_stmt_on_result_end_bind on_resultset_end,
																		const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok,
																		zend_bool * const has_more_results,
																		MYSQLND_STATS * const stats,
																		MYSQLND_ERROR_INFO * const error_info);

typedef enum_func_status	(*func_xmysqlnd_node_stmt__read_all_results)(XMYSQLND_NODE_STMT * const stmt,
																		 const struct st_xmysqlnd_node_stmt_on_row_bind on_row,
																		 const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
																		 const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
																		 const struct st_xmysqlnd_node_stmt_on_result_start_bind on_result_start,
																		 const struct st_xmysqlnd_node_stmt_on_result_end_bind on_resultset_end,
																		 const struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok,
																		 MYSQLND_STATS * const stats,
																		 MYSQLND_ERROR_INFO * const error_info);
typedef zend_bool									(*func_xmysqlnd_node_stmt__has_more_results)(const XMYSQLND_NODE_STMT * const stmt);

typedef struct st_xmysqlnd_node_stmt_result *		(*func_xmysqlnd_node_stmt__get_buffered_result)(XMYSQLND_NODE_STMT * const stmt,
																									zend_bool * const has_more_results,
																									const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
																									const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
																									MYSQLND_STATS * const stats,
																									MYSQLND_ERROR_INFO * const error_info);

typedef struct st_xmysqlnd_node_stmt_result *		(*func_xmysqlnd_node_stmt__get_fwd_result)(XMYSQLND_NODE_STMT * const stmt,
																							   const size_t rows,
																							   zend_bool * const has_more_rows_in_set,
																							   zend_bool * const has_more_results,
																							   const struct st_xmysqlnd_node_stmt_on_warning_bind on_warning,
																							   const struct st_xmysqlnd_node_stmt_on_error_bind on_error,
																							   MYSQLND_STATS * const stats,
																							   MYSQLND_ERROR_INFO * const error_info);

typedef enum_func_status							(*func_xmysqlnd_node_stmt__skip_one_result)(XMYSQLND_NODE_STMT * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status							(*func_xmysqlnd_node_stmt__skip_all_results)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef struct st_xmysqlnd_rowset *					(*func_xmysqlnd_node_stmt__create_rowset)(void * ctx);
typedef struct st_xmysqlnd_node_stmt_result_meta *	(*func_xmysqlnd_node_stmt__create_meta)(void * ctx);
typedef struct st_xmysqlnd_result_field_meta *		(*func_xmysqlnd_node_stmt__create_meta_field)(void * ctx);
typedef struct st_xmysqlnd_stmt_execution_state *	(*func_xmysqlnd_node_stmt__create_execution_state)(void * ctx);
typedef struct st_xmysqlnd_warning_list *			(*func_xmysqlnd_node_stmt__create_warning_list)(void * ctx);

typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_row_field)(void * context, const MYSQLND_CSTRING buffer, const unsigned int idx, func_xmysqlnd_wireprotocol__row_field_decoder decoder);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_meta_field)(void * context, struct st_xmysqlnd_result_field_meta * field);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_warning)(void * context, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_error)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_exec_state_change)(void * context, const enum xmysqlnd_execution_state_type type, const size_t value);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_trx_state_change)(void * context, const enum xmysqlnd_transaction_state_type type);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_statement_ok)(void * context);
typedef const enum_hnd_func_status					(*func_xmysqlnd_node_stmt__handler_on_resultset_end)(void * context, const zend_bool has_more);

typedef XMYSQLND_NODE_STMT *(*func_xmysqlnd_node_stmt__get_reference)(XMYSQLND_NODE_STMT * const stmt);
typedef enum_func_status	(*func_xmysqlnd_node_stmt__free_reference)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt__free_contents)(XMYSQLND_NODE_STMT * const stmt);
typedef void				(*func_xmysqlnd_node_stmt__dtor)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt)
{
	func_xmysqlnd_node_stmt__init init;
	func_xmysqlnd_node_stmt__send_raw_message send_raw_message;
	func_xmysqlnd_node_stmt__read_one_result read_one_result;
	func_xmysqlnd_node_stmt__read_all_results read_all_results;
	func_xmysqlnd_node_stmt__has_more_results has_more_results;
	func_xmysqlnd_node_stmt__get_buffered_result get_buffered_result;
	func_xmysqlnd_node_stmt__get_fwd_result get_fwd_result;
	func_xmysqlnd_node_stmt__skip_one_result skip_one_result;
	func_xmysqlnd_node_stmt__skip_all_results skip_all_results;

	func_xmysqlnd_node_stmt__create_rowset create_rowset_fwd; 				/* export the function for binding */
	func_xmysqlnd_node_stmt__create_rowset create_rowset_buffered; 			/* export the function for binding */
	func_xmysqlnd_node_stmt__create_meta create_meta; 						/* export the function for binding */
	func_xmysqlnd_node_stmt__create_meta_field create_meta_field;			/* export the function for binding */

	func_xmysqlnd_node_stmt__handler_on_row_field handler_on_row_field;		/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_meta_field handler_on_meta_field;	/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_warning handler_on_warning;			/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_error handler_on_error;				/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_exec_state_change handler_on_exec_state_change;/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_trx_state_change handler_on_trx_state_change;/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_statement_ok handler_on_statement_ok;	/* export the function for binding */
	func_xmysqlnd_node_stmt__handler_on_resultset_end handler_on_resultset_end;	/* export the function for binding */

	func_xmysqlnd_node_stmt__get_reference get_reference;
	func_xmysqlnd_node_stmt__free_reference free_reference;

	func_xmysqlnd_node_stmt__free_contents free_contents;
	func_xmysqlnd_node_stmt__dtor dtor;
};

struct st_xmysqlnd_node_stmt_bind_ctx
{
	XMYSQLND_NODE_STMT * stmt;
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	func_xmysqlnd_node_stmt__create_rowset create_rowset;
	size_t fwd_prefetch_count;
	size_t prefetch_counter;
	zval* current_row{nullptr};
	st_xmysqlnd_rowset* rowset;
	st_xmysqlnd_node_stmt_result_meta* meta;
	st_xmysqlnd_node_stmt_result* result;
	st_xmysqlnd_warning_list* warnings;
	st_xmysqlnd_stmt_execution_state* exec_state;

	struct st_xmysqlnd_node_stmt_on_row_bind on_row;
	struct st_xmysqlnd_node_stmt_on_warning_bind on_warning;
	struct st_xmysqlnd_node_stmt_on_error_bind on_error;
	struct st_xmysqlnd_node_stmt_on_result_end_bind on_resultset_end;
	struct st_xmysqlnd_node_stmt_on_statement_ok_bind on_statement_ok;
};

struct st_xmysqlnd_node_stmt_data : public util::permanent_allocable
{
	st_xmysqlnd_node_session* session;
	struct st_xmysqlnd_msg__sql_stmt_execute msg_stmt_exec;

	struct st_xmysqlnd_node_stmt_bind_ctx read_ctx;

	zend_bool         partial_read_started;
	MYSQLND_CSTRING	* assigned_document_ids;
	int               num_of_assigned_doc_ids;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_stmt : public util::permanent_allocable
{
	XMYSQLND_NODE_STMT_DATA * data;

	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_stmt);
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_STMT * xmysqlnd_node_stmt_create(struct st_xmysqlnd_node_session * session,
													  const zend_bool persistent,
													  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info);

PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_stmt_free(XMYSQLND_NODE_STMT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_NODE_STMT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
