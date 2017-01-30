/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#ifndef XMYSQLND_NODE_TABLE_H
#define XMYSQLND_NODE_TABLE_H

#include "xmysqlnd_driver.h"

#include "xmysqlnd_crud_table_commands.h"
#include <phputils/allocator.h>

struct st_xmysqlnd_node_schema;
struct st_xmysqlnd_node_session_on_error_bind;

typedef struct st_xmysqlnd_node_table		XMYSQLND_NODE_TABLE;
typedef struct st_xmysqlnd_node_table_data	XMYSQLND_NODE_TABLE_DATA;


typedef enum_func_status (*func_xmysqlnd_node_table__init)(
			XMYSQLND_NODE_TABLE * const table,
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
			struct st_xmysqlnd_node_schema * const schema,
			const MYSQLND_CSTRING table_name,
			MYSQLND_STATS * const stats,
			MYSQLND_ERROR_INFO * const error_info);

typedef XMYSQLND_NODE_TABLE *	(*func_xmysqlnd_node_table__get_reference)(XMYSQLND_NODE_TABLE * const table);
typedef enum_func_status		(*func_xmysqlnd_node_table__exists_in_database)(XMYSQLND_NODE_TABLE * const table, struct st_xmysqlnd_node_session_on_error_bind on_error, zval* exists);
typedef enum_func_status		(*func_xmysqlnd_node_table__count)(XMYSQLND_NODE_TABLE * const table, struct st_xmysqlnd_node_session_on_error_bind on_error, zval* counter);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_table__insert)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__INSERT * op);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_table__delete)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__DELETE * op);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_table__update)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__UPDATE * op);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_table__select)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__SELECT * op);
typedef enum_func_status		(*func_xmysqlnd_node_table__free_reference)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void					(*func_xmysqlnd_node_table__free_contents)(XMYSQLND_NODE_TABLE * const table);
typedef void					(*func_xmysqlnd_node_table__dtor)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_table)
{
	func_xmysqlnd_node_table__init init;

	func_xmysqlnd_node_table__exists_in_database exists_in_database;
	func_xmysqlnd_node_table__count count;

	func_xmysqlnd_node_table__insert insert;

	func_xmysqlnd_node_table__delete opdelete;
	func_xmysqlnd_node_table__update update;
	func_xmysqlnd_node_table__select select;

	func_xmysqlnd_node_table__get_reference get_reference;
	func_xmysqlnd_node_table__free_reference free_reference;

	func_xmysqlnd_node_table__free_contents free_contents;
	func_xmysqlnd_node_table__dtor dtor;
};

struct st_xmysqlnd_node_table_data
{
	struct st_xmysqlnd_node_schema * schema;
	MYSQLND_STRING table_name;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_table) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_table : mysqlx::phputils::custom_allocable
{
	XMYSQLND_NODE_TABLE_DATA * data;

	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_table);
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_TABLE * xmysqlnd_node_table_create(struct st_xmysqlnd_node_schema * schema,
														const MYSQLND_CSTRING table_name,
														const zend_bool persistent,
														const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
														MYSQLND_STATS * const stats,
														MYSQLND_ERROR_INFO * const error_info);

PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_table_free(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

#endif /* XMYSQLND_NODE_TABLE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
