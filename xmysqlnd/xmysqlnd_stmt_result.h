/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#ifndef XMYSQLND_STMT_RESULT_H
#define XMYSQLND_STMT_RESULT_H

#include "xmysqlnd_driver.h"
#include "util/allocator.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_stmt;
struct st_xmysqlnd_stmt_result_meta;
struct st_xmysqlnd_stmt_execution_state;
struct st_xmysqlnd_rowset;
class xmysqlnd_warning_list;


typedef struct st_xmysqlnd_stmt_result			XMYSQLND_STMT_RESULT;

typedef enum_func_status	(*func_xmysqlnd_stmt_result__init)(XMYSQLND_STMT_RESULT * const result, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__next)(XMYSQLND_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__fetch_current)(XMYSQLND_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__fetch_one)(XMYSQLND_STMT_RESULT * const result, const size_t row_cursor, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__fetch_one_c)(XMYSQLND_STMT_RESULT * const result, const size_t row_cursor, zval ** row, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__fetch_all)(XMYSQLND_STMT_RESULT * const result, zval * set, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__fetch_all_c)(XMYSQLND_STMT_RESULT * const result, zval ** set, const zend_bool duplicate, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__rewind)(XMYSQLND_STMT_RESULT * const result);
typedef zend_bool			(*func_xmysqlnd_stmt_result__eof)(const XMYSQLND_STMT_RESULT * const result);

typedef zval *				(*func_xmysqlnd_stmt_result__create_row)(XMYSQLND_STMT_RESULT * const result, const st_xmysqlnd_stmt_result_meta* const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_stmt_result__destroy_row)(XMYSQLND_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__add_row)(XMYSQLND_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef size_t				(*func_xmysqlnd_stmt_result__get_row_count)(const XMYSQLND_STMT_RESULT * const result);
typedef void				(*func_xmysqlnd_stmt_result__free_rows_contents)(XMYSQLND_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_stmt_result__free_rows)(XMYSQLND_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef enum_func_status	(*func_xmysqlnd_stmt_result__attach_rowset)(XMYSQLND_STMT_RESULT * const result, st_xmysqlnd_rowset* const st_xmysqlnd_rowset, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__attach_meta)(XMYSQLND_STMT_RESULT * const result, st_xmysqlnd_stmt_result_meta* const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__attach_execution_state)(XMYSQLND_STMT_RESULT * const result, st_xmysqlnd_stmt_execution_state* const exec_state);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__attach_warning_list)(XMYSQLND_STMT_RESULT * const result, xmysqlnd_warning_list* const warning_list);


typedef XMYSQLND_STMT_RESULT *	(*func_xmysqlnd_stmt_result__get_reference)(XMYSQLND_STMT_RESULT * const result);
typedef enum_func_status	(*func_xmysqlnd_stmt_result__free_reference)(XMYSQLND_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef void				(*func_xmysqlnd_stmt_result__free_contents)(XMYSQLND_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_stmt_result__dtor)(XMYSQLND_STMT_RESULT* const result, MYSQLND_STATS* stats, MYSQLND_ERROR_INFO* error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result)
{
	func_xmysqlnd_stmt_result__init init;

	func_xmysqlnd_stmt_result__next next;
	func_xmysqlnd_stmt_result__fetch_current fetch_current;
	func_xmysqlnd_stmt_result__fetch_one fetch_one;
	func_xmysqlnd_stmt_result__fetch_one_c fetch_one_c;
	func_xmysqlnd_stmt_result__fetch_all fetch_all;
	func_xmysqlnd_stmt_result__fetch_all_c fetch_all_c;
	func_xmysqlnd_stmt_result__rewind rewind;
	func_xmysqlnd_stmt_result__eof eof;

	func_xmysqlnd_stmt_result__create_row create_row;
	func_xmysqlnd_stmt_result__destroy_row destroy_row;
	func_xmysqlnd_stmt_result__add_row add_row;
	func_xmysqlnd_stmt_result__get_row_count get_row_count;
	func_xmysqlnd_stmt_result__free_rows_contents free_rows_contents;
	func_xmysqlnd_stmt_result__free_rows free_rows;

	func_xmysqlnd_stmt_result__attach_rowset attach_rowset;
	func_xmysqlnd_stmt_result__attach_meta attach_meta;
	func_xmysqlnd_stmt_result__attach_execution_state attach_execution_state;
	func_xmysqlnd_stmt_result__attach_warning_list attach_warning_list;

	func_xmysqlnd_stmt_result__get_reference get_reference;
	func_xmysqlnd_stmt_result__free_reference free_reference;

	func_xmysqlnd_stmt_result__free_contents free_contents;
	func_xmysqlnd_stmt_result__dtor dtor;
};



struct st_xmysqlnd_stmt_result : public util::custom_allocable
{
	st_xmysqlnd_rowset* rowset;
	st_xmysqlnd_stmt_result_meta* meta;
	st_xmysqlnd_stmt_execution_state* exec_state;
	xmysqlnd_warning_list* warnings;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result) m;
	size_t		refcount;
	zend_bool	persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_stmt_result);
PHP_MYSQL_XDEVAPI_API XMYSQLND_STMT_RESULT * xmysqlnd_stmt_result_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_stmt_result_free(XMYSQLND_STMT_RESULT* const result, MYSQLND_STATS* stats, MYSQLND_ERROR_INFO* error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STMT_RESULT_H */
