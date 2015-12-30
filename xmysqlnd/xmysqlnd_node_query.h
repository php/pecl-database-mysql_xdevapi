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

#ifndef XMYSQLND_NODE_QUERY_H
#define XMYSQLND_NODE_QUERY_H

#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_sql_stmt_execute_message_ctx */

struct st_xmysqlnd_node_session_data;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_xmysqlnd_node_query		XMYSQLND_NODE_QUERY;
typedef struct st_xmysqlnd_node_query_data	XMYSQLND_NODE_QUERY_DATA;

enum xmysqlnd_node_query_state
{
	XNODE_QR_WAIT_META,
	XNODE_QR_READING_ROWS,
	XNODE_QR_EOF,
};

typedef enum_func_status	(*func_xmysqlnd_node_query__init)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_query__send_query)(XMYSQLND_NODE_QUERY * const result, const MYSQLND_CSTRING query, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_query__read_result)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_query__skip_result)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef zend_bool			(*func_xmysqlnd_node_query__eof)(const XMYSQLND_NODE_QUERY * const result);
typedef void				(*func_xmysqlnd_node_query__free_contents)(XMYSQLND_NODE_QUERY * const result);
typedef void				(*func_xmysqlnd_node_query__dtor)(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_query)
{
	func_xmysqlnd_node_query__init init;
	func_xmysqlnd_node_query__send_query send_query;
	func_xmysqlnd_node_query__read_result read_result;
	func_xmysqlnd_node_query__skip_result skip_result;
	func_xmysqlnd_node_query__eof eof;
	func_xmysqlnd_node_query__free_contents free_contents;
	func_xmysqlnd_node_query__dtor dtor;
};


struct st_xmysqlnd_node_query_data
{
	struct st_xmysqlnd_node_session_data * session;
	struct st_xmysqlnd_sql_stmt_execute_message_ctx msg_stmt_exec;
	enum xmysqlnd_node_query_state state;
	struct st_xmysqlnd_node_session_data * metadata;
	size_t row_count;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_query) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_query
{
	XMYSQLND_NODE_QUERY_DATA * data;

	zend_bool 		persistent;
};


PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_node_query);
PHPAPI XMYSQLND_NODE_QUERY * xmysqlnd_node_query_init(struct st_xmysqlnd_node_session_data * session, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHPAPI void xmysqlnd_node_query_free(XMYSQLND_NODE_QUERY * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_QUERY_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
