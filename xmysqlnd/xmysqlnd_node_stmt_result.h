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

#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_sql_stmt_execute_message_ctx */

struct st_xmysqlnd_node_stmt;
struct st_xmysqlnd_node_stmt_result_meta;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_xmysqlnd_node_stmt_result			XMYSQLND_NODE_STMT_RESULT;
typedef struct st_xmysqlnd_node_stmt_result_data	XMYSQLND_NODE_STMT_RESULT_DATA;

struct st_xmysqlnd_warning
{
	MYSQLND_STRING	message;
	unsigned int	code;
	enum xmysqlnd_stmt_warning_level level;
};


typedef struct st_xmysqlnd_cwarning
{
	MYSQLND_CSTRING	message;
	unsigned int	code;
	enum xmysqlnd_stmt_warning_level level;
} XMYSQLND_WARNING;

typedef struct st_xmysqlnd_warning_list XMYSQLND_WARNING_LIST;

typedef enum_func_status		(*func_xmysqlnd_warning_list__init)(XMYSQLND_WARNING_LIST * const warn_list, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef unsigned int			(*func_xmysqlnd_warning_list__count)(const XMYSQLND_WARNING_LIST * const warn_list);
typedef const XMYSQLND_WARNING	(*func_xmysqlnd_warning_list__get_warning)(const XMYSQLND_WARNING_LIST * const warn_list, unsigned int offset);
typedef void					(*func_xmysqlnd_warning_list__add_warning)(XMYSQLND_WARNING_LIST * const warn_list, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
typedef void					(*func_xmysqlnd_warning_list__free_contents)(XMYSQLND_WARNING_LIST * const warn_list);
typedef void					(*func_xmysqlnd_warning_list__dtor)(XMYSQLND_WARNING_LIST * const warn_list);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list)
{
	func_xmysqlnd_warning_list__init init;
	func_xmysqlnd_warning_list__add_warning add_warning;
	func_xmysqlnd_warning_list__count count;
	func_xmysqlnd_warning_list__get_warning get_warning;

	func_xmysqlnd_warning_list__free_contents free_contents;
	func_xmysqlnd_warning_list__dtor dtor;
};

struct st_xmysqlnd_warning_list
{
	unsigned int warning_count;
	struct st_xmysqlnd_warning * warnings;
	unsigned int warnings_allocated;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) *m;
	zend_bool persistent;
};

PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_warning_list);
PHPAPI XMYSQLND_WARNING_LIST * xmysqlnd_warning_list_init(const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHPAPI void xmysqlnd_warning_list_free(XMYSQLND_WARNING_LIST * const list);



typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__init)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory, struct st_xmysqlnd_node_stmt * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef size_t				(*func_xmysqlnd_node_stmt_result__get_affected_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result);
typedef size_t				(*func_xmysqlnd_node_stmt_result__get_matched_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result);
typedef size_t				(*func_xmysqlnd_node_stmt_result__get_found_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result);
typedef uint64_t			(*func_xmysqlnd_node_stmt_result__get_last_insert_id)(const XMYSQLND_NODE_STMT_RESULT * const result);

typedef void				(*func_xmysqlnd_node_stmt_result__set_affected_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result, const size_t value);
typedef void				(*func_xmysqlnd_node_stmt_result__set_matched_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result, const size_t value);
typedef void				(*func_xmysqlnd_node_stmt_result__set_found_items_count)(const XMYSQLND_NODE_STMT_RESULT * const result, const size_t value);
typedef void				(*func_xmysqlnd_node_stmt_result__set_last_insert_id)(const XMYSQLND_NODE_STMT_RESULT * const result, const uint64_t value);

#if 0
typedef unsigned int		(*func_xmysqlnd_node_stmt_result__get_warning_count)(const XMYSQLND_NODE_STMT_RESULT * const result);
typedef const XMYSQLND_WARNING	(*func_xmysqlnd_node_stmt_result__get_warning)(const XMYSQLND_NODE_STMT_RESULT * const result, unsigned int offset);
typedef void				(*func_xmysqlnd_node_stmt_result__add_warning)(XMYSQLND_NODE_STMT_RESULT * const result, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
#endif

typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__has_data)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__next)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__fetch)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__fetch_all)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef zend_bool			(*func_xmysqlnd_node_stmt_result__eof)(const XMYSQLND_NODE_STMT_RESULT * const result);

typedef zval *				(*func_xmysqlnd_node_stmt_result__create_row)(XMYSQLND_NODE_STMT_RESULT * const result, const struct st_xmysqlnd_node_stmt_result_meta * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt_result__destroy_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__add_row)(XMYSQLND_NODE_STMT_RESULT * const result, zval * row, const struct st_xmysqlnd_node_stmt_result_meta * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef size_t				(*func_xmysqlnd_node_stmt_result__get_row_count)(const XMYSQLND_NODE_STMT_RESULT * const result);

typedef enum_func_status	(*func_xmysqlnd_node_stmt_result__attach_meta)(XMYSQLND_NODE_STMT_RESULT * const result, struct st_xmysqlnd_node_stmt_result_meta * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef void				(*func_xmysqlnd_node_stmt_result__free_contents)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_node_stmt_result__dtor)(XMYSQLND_NODE_STMT_RESULT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result)
{
	func_xmysqlnd_node_stmt_result__init init;
	func_xmysqlnd_node_stmt_result__get_affected_items_count get_affected_items_count;
	func_xmysqlnd_node_stmt_result__get_matched_items_count get_matched_items_count;
	func_xmysqlnd_node_stmt_result__get_found_items_count get_found_items_count;
	func_xmysqlnd_node_stmt_result__get_last_insert_id get_last_insert_id;

	func_xmysqlnd_node_stmt_result__set_affected_items_count set_affected_items_count;
	func_xmysqlnd_node_stmt_result__set_matched_items_count set_matched_items_count;
	func_xmysqlnd_node_stmt_result__set_found_items_count set_found_items_count;
	func_xmysqlnd_node_stmt_result__set_last_insert_id set_last_insert_id;
#if 0
	func_xmysqlnd_node_stmt_result__get_warning_count get_warning_count;
	func_xmysqlnd_node_stmt_result__get_warning get_warning;
	func_xmysqlnd_node_stmt_result__add_warning add_warning;
#endif

	func_xmysqlnd_node_stmt_result__has_data has_data;
	func_xmysqlnd_node_stmt_result__next next;
	func_xmysqlnd_node_stmt_result__fetch fetch;
	func_xmysqlnd_node_stmt_result__fetch_all fetch_all;
	func_xmysqlnd_node_stmt_result__eof eof;

	func_xmysqlnd_node_stmt_result__create_row create_row;
	func_xmysqlnd_node_stmt_result__destroy_row destroy_row;
	func_xmysqlnd_node_stmt_result__add_row add_row;
	func_xmysqlnd_node_stmt_result__get_row_count get_row_count;

	func_xmysqlnd_node_stmt_result__attach_meta attach_meta;

	func_xmysqlnd_node_stmt_result__free_contents free_contents;
	func_xmysqlnd_node_stmt_result__dtor dtor;
};


struct st_xmysqlnd_node_stmt_result_data
{
	struct st_xmysqlnd_node_stmt * stmt;
	struct st_xmysqlnd_node_stmt_result_meta * meta;
	size_t row_count;
	size_t items_affected;
	size_t items_matched;
	size_t items_found;
	uint64_t last_insert_id;
	/*UUID  last_document_id; */
	XMYSQLND_WARNING_LIST * warnings;
#if 0
	unsigned int warning_count;
	struct st_xmysqlnd_warning *warnings;
	unsigned int warnings_allocated;
#endif
	zval ** rows; /* every row is a memory segment of field_count * sizeof(zval) */
	size_t rows_allocated;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_stmt_result
{
	XMYSQLND_NODE_STMT_RESULT_DATA * data;

	zend_bool		persistent;
};


PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_node_stmt_result);
PHPAPI XMYSQLND_NODE_STMT_RESULT * xmysqlnd_node_stmt_result_init(struct st_xmysqlnd_node_stmt * stmt, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
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
