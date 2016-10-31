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
#ifndef XMYSQLND_NODE_COLLECTION_H
#define XMYSQLND_NODE_COLLECTION_H

#include "xmysqlnd_driver.h"

#include "xmysqlnd_crud_collection_commands.h"

struct st_xmysqlnd_node_schema;
struct st_xmysqlnd_node_session_on_error_bind;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_xmysqlnd_node_collection		XMYSQLND_NODE_COLLECTION;
typedef struct st_xmysqlnd_node_collection_data	XMYSQLND_NODE_COLLECTION_DATA;


typedef enum_func_status (*func_xmysqlnd_node_collection__init)(
			XMYSQLND_NODE_COLLECTION * const collection,
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
			struct st_xmysqlnd_node_schema * const schema,
			const MYSQLND_CSTRING collection_name,
			MYSQLND_STATS * const stats,
			MYSQLND_ERROR_INFO * const error_info);

typedef XMYSQLND_NODE_COLLECTION * (*func_xmysqlnd_node_collection__get_reference)(XMYSQLND_NODE_COLLECTION * const collection);
typedef enum_func_status		(*func_xmysqlnd_node_collection__exists_in_database)(XMYSQLND_NODE_COLLECTION * const collection, struct st_xmysqlnd_node_session_on_error_bind on_error, zval* exists);
typedef enum_func_status		(*func_xmysqlnd_node_collection__count)(XMYSQLND_NODE_COLLECTION * const collection, struct st_xmysqlnd_node_session_on_error_bind on_error, zval* counter);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_collection__add)(XMYSQLND_NODE_COLLECTION * const collection, const MYSQLND_CSTRING json);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_collection__remove)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__REMOVE * op);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_collection__modify)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__MODIFY * op);
typedef struct st_xmysqlnd_node_stmt * (*func_xmysqlnd_node_collection__find)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__FIND * op);
typedef enum_func_status		(*func_xmysqlnd_node_collection__free_reference)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void					(*func_xmysqlnd_node_collection__free_contents)(XMYSQLND_NODE_COLLECTION * const collection);
typedef void					(*func_xmysqlnd_node_collection__dtor)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_collection)
{
	func_xmysqlnd_node_collection__init init;

	func_xmysqlnd_node_collection__exists_in_database exists_in_database;
	func_xmysqlnd_node_collection__count count;

	func_xmysqlnd_node_collection__add add;

	func_xmysqlnd_node_collection__remove remove;
	func_xmysqlnd_node_collection__modify modify;
	func_xmysqlnd_node_collection__find find;

	func_xmysqlnd_node_collection__get_reference get_reference;
	func_xmysqlnd_node_collection__free_reference free_reference;

	func_xmysqlnd_node_collection__free_contents free_contents;
	func_xmysqlnd_node_collection__dtor dtor;
};

struct st_xmysqlnd_node_collection_data
{
	struct st_xmysqlnd_node_schema * schema;
	MYSQLND_STRING collection_name;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_collection) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_collection
{
	XMYSQLND_NODE_COLLECTION_DATA * data;

	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_collection);
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_COLLECTION * xmysqlnd_node_collection_create(struct st_xmysqlnd_node_schema * schema,
																  const MYSQLND_CSTRING collection_name,
																  const zend_bool persistent,
																  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
																  MYSQLND_STATS * const stats,
																  MYSQLND_ERROR_INFO * const error_info);

PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_collection_free(XMYSQLND_NODE_COLLECTION * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_COLLECTION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
