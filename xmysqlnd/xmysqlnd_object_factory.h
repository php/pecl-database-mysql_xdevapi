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

#ifndef XMYSQLND_OBJECT_FACTORY_H
#define XMYSQLND_OBJECT_FACTORY_H

struct st_xmysqlnd_node_session;
struct st_xmysqlnd_node_session_data;
struct st_xmysqlnd_node_stmt;
struct st_xmysqlnd_node_stmt_result;
struct st_xmysqlnd_node_stmt_rowset;
struct st_xmysqlnd_rowset_buffered;
struct st_xmysqlnd_rowset_fwd;
struct st_xmysqlnd_result_field_meta;
struct st_xmysqlnd_protocol_frame_codec;
struct st_xmysqlnd_warning_list;
struct st_xmysqlnd_stmt_execution_state;

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory);

typedef struct st_xmysqlnd_node_session *			(*func_xmysqlnd_object_factory__get_node_session)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_node_session_data *		(*func_xmysqlnd_object_factory__get_node_session_data)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_node_stmt *				(*func_xmysqlnd_object_factory__get_node_stmt)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, struct st_xmysqlnd_node_session_data * session, const MYSQLND_CSTRING query, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_node_stmt_result *		(*func_xmysqlnd_object_factory__get_node_stmt_result)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_rowset_buffered *		(*func_xmysqlnd_object_factory__get_rowset_buffered)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, struct st_xmysqlnd_node_stmt * stmt, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_rowset_fwd *	(*func_xmysqlnd_object_factory__get_rowset_fwd)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const size_t prefetch_rows, struct st_xmysqlnd_node_stmt * stmt, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_rowset *					(*func_xmysqlnd_object_factory__get_rowset)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, unsigned int type, const size_t prefetch_rows, struct st_xmysqlnd_node_stmt * stmt, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_node_stmt_result_meta *	(*func_xmysqlnd_object_factory__get_node_stmt_result_meta)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_result_field_meta *		(*func_xmysqlnd_object_factory__get_result_field_meta)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_protocol_frame_codec *	(*func_xmysqlnd_object_factory__get_pfc)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_warning_list *			(*func_xmysqlnd_object_factory__get_warning_list)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef struct st_xmysqlnd_stmt_execution_state *	(*func_xmysqlnd_object_factory__get_stmt_execution_state)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * factory, const zend_bool persistent, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)
{
	func_xmysqlnd_object_factory__get_node_session get_node_session;
	func_xmysqlnd_object_factory__get_node_session_data get_node_session_data;
	func_xmysqlnd_object_factory__get_node_stmt get_node_stmt;
	func_xmysqlnd_object_factory__get_node_stmt_result get_node_stmt_result;
	func_xmysqlnd_object_factory__get_rowset_buffered get_rowset_buffered;
	func_xmysqlnd_object_factory__get_rowset_fwd get_rowset_fwd;
	func_xmysqlnd_object_factory__get_rowset get_rowset;
	func_xmysqlnd_object_factory__get_node_stmt_result_meta get_node_stmt_result_meta;
	func_xmysqlnd_object_factory__get_result_field_meta get_result_field_meta;
	func_xmysqlnd_object_factory__get_pfc get_protocol_frame_codec;
	func_xmysqlnd_object_factory__get_warning_list get_warning_list;
	func_xmysqlnd_object_factory__get_stmt_execution_state get_stmt_execution_state;
};

PHPAPI extern MYSQLND_CLASS_METHOD_TABLE_NAME_FORWARD(xmysqlnd_object_factory);

#endif	/* XMYSQLND_OBJECT_FACTORY_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
