/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/

#ifndef XMYSQLND_NODE_STMT_RESULT_H
#define XMYSQLND_NODE_STMT_RESULT_H

#include "xmysqlnd_driver.h"

struct st_xmysqlnd_node_stmt;
struct st_xmysqlnd_node_stmt_result_meta;
struct st_xmysqlnd_stmt_execution_state;
struct st_xmysqlnd_rowset;
struct st_xmysqlnd_warning_list;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_xmysqlnd_node_stmt_result			XMYSQLND_NODE_STMT_RESULT;

typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__init)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__fetch_current)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__fetch_one)(XMYSQLND_NODE_STMT_RESULT * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__rewind)(XMYSQLND_NODE_STMT_RESULT * const result);
typedef zend_bool			(*func_xmysqlnd_node_stmt_result__eof)(const XMYSQLND_NODE_STMT_RESULT * const result);

typedef zval *				(*func_xmysqlnd_node_stmt_result__create_row)(XMYSQLND_NODE_STMT_RESULT * const result, const struct st_xmysqlnd_node_stmt_result_meta * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt_result__destroy_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef size_t				(*func_xmysqlnd_node_stmt_result__get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result);
typedef void				(*func_xmysqlnd_node_stmt_result__free_rows_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt_result__free_rows)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__attach_rowset)(XMYSQLND_NODE_STMT_RESULT * const result, struct st_xmysqlnd_rowset * const st_xmysqlnd_rowset, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__attach_execution_state)(XMYSQLND_NODE_STMT_RESULT * const result, struct st_xmysqlnd_stmt_execution_state * const exec_state);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__attach_warning_list)(XMYSQLND_NODE_STMT_RESULT * const result, struct st_xmysqlnd_warning_list * const warning_list);


typedef XMYSQLND_NODE_STMT_RESULT *	(*func_xmysqlnd_node_stmt_result__get_reference)(XMYSQLND_NODE_STMT_RESULT * const result);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__free_reference)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef void				(*func_xmysqlnd_node_stmt_result__free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt_result__dtor)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result)
{
	func_xmysqlnd_node_stmt_result__init init;

	func_xmysqlnd_node_stmt_result__next next;
	func_xmysqlnd_node_stmt_result__fetch_current fetch_current;
	func_xmysqlnd_node_stmt_result__fetch_one fetch_one;
	func_xmysqlnd_node_stmt_result__fetch_all fetch_all;
	func_xmysqlnd_node_stmt_result__rewind rewind;
	func_xmysqlnd_node_stmt_result__eof eof;

	func_xmysqlnd_node_stmt_result__create_row create_row;
	func_xmysqlnd_node_stmt_result__destroy_row destroy_row;
	func_xmysqlnd_node_stmt_result__add_row add_row;
	func_xmysqlnd_node_stmt_result__get_row_count get_row_count;
	func_xmysqlnd_node_stmt_result__free_rows_contents free_rows_contents;
	func_xmysqlnd_node_stmt_result__free_rows free_rows;

	func_xmysqlnd_node_stmt_result__attach_rowset attach_rowset;
	func_xmysqlnd_node_stmt_result__attach_execution_state attach_execution_state;
	func_xmysqlnd_node_stmt_result__attach_warning_list attach_warning_list;

	func_xmysqlnd_node_stmt_result__get_reference get_reference;
	func_xmysqlnd_node_stmt_result__free_reference free_reference;

	func_xmysqlnd_node_stmt_result__free_contents free_contents;
	func_xmysqlnd_node_stmt_result__dtor dtor;
};


struct st_xmysqlnd_node_stmt_result
{
	struct st_xmysqlnd_rowset * rowset;
	struct st_xmysqlnd_stmt_execution_state * exec_state;
	struct st_xmysqlnd_warning_list * warnings;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) m;
	size_t		refcount;
	zend_bool	persistent;
};


PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_node_stmt_result);
PHPAPI XMYSQLND_NODE_STMT_RESULT * xmysqlnd_node_stmt_result_init(const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHPAPI void xmysqlnd_node_stmt_result_free(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_STMT_RESULT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
