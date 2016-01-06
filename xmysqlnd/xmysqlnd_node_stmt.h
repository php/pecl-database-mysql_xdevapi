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

#ifndef XMYSQLND_NODE_STMT_H
#define XMYSQLND_NODE_STMT_H

#include "xmysqlnd_driver.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_sql_stmt_execute_message_ctx */

struct st_xmysqlnd_node_session_data;
struct st_xmysqlnd_node_stmt_result;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_xmysqlnd_node_stmt		XMYSQLND_NODE_STMT;
typedef struct st_xmysqlnd_node_stmt_data	XMYSQLND_NODE_STMT_DATA;


typedef enum_func_status	(*func_xmysqlnd_node_stmt__init)(XMYSQLND_NODE_STMT * const stmt, struct st_xmysqlnd_node_session_data * const session, const MYSQLND_CSTRING query, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt__send_query)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef struct st_xmysqlnd_node_stmt_result *		(*func_xmysqlnd_node_stmt__read_one_result)(XMYSQLND_NODE_STMT * const stmt, zend_bool * const has_more, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status							(*func_xmysqlnd_node_stmt__skip_one_result)(XMYSQLND_NODE_STMT * const stmt, zend_bool * const has_more, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef struct st_xmysqlnd_node_stmt_result *		(*func_xmysqlnd_node_stmt__create_result)(void * ctx);
typedef struct st_xmysqlnd_node_stmt_result_meta *	(*func_xmysqlnd_node_stmt__create_meta)(void * ctx);
typedef struct st_xmysqlnd_result_field_meta *		(*func_xmysqlnd_node_stmt__create_meta_field)(void * ctx);
typedef XMYSQLND_NODE_STMT *(*func_xmysqlnd_node_stmt__get_reference)(XMYSQLND_NODE_STMT * const stmt);
typedef enum_func_status	(*func_xmysqlnd_node_stmt__free_reference)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void				(*func_xmysqlnd_node_stmt__free_contents)(XMYSQLND_NODE_STMT * const stmt);
typedef void				(*func_xmysqlnd_node_stmt__dtor)(XMYSQLND_NODE_STMT * const stmt, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt)
{
	func_xmysqlnd_node_stmt__init init;
	func_xmysqlnd_node_stmt__send_query send_query;
	func_xmysqlnd_node_stmt__read_one_result read_one_result;
	func_xmysqlnd_node_stmt__skip_one_result skip_one_result;
	func_xmysqlnd_node_stmt__create_result create_result; 			/* export the function for binding */
	func_xmysqlnd_node_stmt__create_meta create_meta; 				/* export the function for binding */
	func_xmysqlnd_node_stmt__create_meta_field create_meta_field;	/* export the function for binding */
	func_xmysqlnd_node_stmt__get_reference get_reference;
	func_xmysqlnd_node_stmt__free_reference free_reference;
	func_xmysqlnd_node_stmt__free_contents free_contents;
	func_xmysqlnd_node_stmt__dtor dtor;
};


struct st_xmysqlnd_node_stmt_data
{
	struct st_xmysqlnd_node_session_data * session;
	MYSQLND_STRING query;
	struct st_xmysqlnd_sql_stmt_execute_message_ctx msg_stmt_exec;

	unsigned int	refcount;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_stmt
{
	XMYSQLND_NODE_STMT_DATA * data;

	zend_bool		persistent;
};


PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_node_stmt);
PHPAPI XMYSQLND_NODE_STMT * xmysqlnd_node_stmt_init(struct st_xmysqlnd_node_session_data * session, const MYSQLND_CSTRING query, const zend_bool persistent, MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHPAPI void xmysqlnd_node_stmt_free(XMYSQLND_NODE_STMT * const result, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_STMT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
