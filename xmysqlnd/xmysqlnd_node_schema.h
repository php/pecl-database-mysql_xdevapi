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
#ifndef XMYSQLND_NODE_SCHEMA_H
#define XMYSQLND_NODE_SCHEMA_H

#include "xmysqlnd_driver.h"

struct st_xmysqlnd_node_session;
struct st_xmysqlnd_node_collection;
struct st_xmysqlnd_node_table;

#ifdef __cplusplus
extern "C" {
#endif
extern const MYSQLND_CSTRING xmysqlnd_object_type_filter__table;
extern const MYSQLND_CSTRING xmysqlnd_object_type_filter__collection;


typedef struct st_xmysqlnd_node_schema		XMYSQLND_NODE_SCHEMA;
typedef struct st_xmysqlnd_node_schema_data	XMYSQLND_NODE_SCHEMA_DATA;

struct st_xmysqlnd_node_schema_on_database_object_bind
{
	void (*handler)(void * context, XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING object_name, const MYSQLND_CSTRING object_type);
	void * ctx;
};


struct st_xmysqlnd_node_schema_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, const XMYSQLND_NODE_SCHEMA * const schema, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


typedef enum_func_status (*func_xmysqlnd_node_schema__init)(XMYSQLND_NODE_SCHEMA * const schema,
															const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
															struct st_xmysqlnd_node_session * const session,
															const MYSQLND_CSTRING schema_name,
															MYSQLND_STATS * const stats,
															MYSQLND_ERROR_INFO * const error_info);

typedef struct st_xmysqlnd_node_collection *	(*func_xmysqlnd_node_schema__create_collection_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING collection_name);

typedef struct st_xmysqlnd_node_collection *	(*func_xmysqlnd_node_schema__create_collection)(XMYSQLND_NODE_SCHEMA * const schema,
																								const MYSQLND_CSTRING collection_name,
																								const struct st_xmysqlnd_node_schema_on_error_bind on_error);

typedef enum_func_status						(*func_xmysqlnd_node_schema__drop_collection)(XMYSQLND_NODE_SCHEMA * const schema,
																							  const MYSQLND_CSTRING collection_name,
																							  const struct st_xmysqlnd_node_schema_on_error_bind on_error);

typedef struct st_xmysqlnd_node_table * 		(*func_xmysqlnd_node_schema__create_table_object)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CSTRING table_name);


typedef enum_func_status						(*func_xmysqlnd_node_schema__get_db_objects)(XMYSQLND_NODE_SCHEMA * const schema,
																							 const MYSQLND_CSTRING collection_name,
																							 const MYSQLND_CSTRING object_type_filter,
																							 const struct st_xmysqlnd_node_schema_on_database_object_bind on_object,
																							 const struct st_xmysqlnd_node_schema_on_error_bind on_error);

typedef XMYSQLND_NODE_SCHEMA *	(*func_xmysqlnd_node_schema__get_reference)(XMYSQLND_NODE_SCHEMA * const schema);
typedef enum_func_status		(*func_xmysqlnd_node_schema__free_reference)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void					(*func_xmysqlnd_node_schema__free_contents)(XMYSQLND_NODE_SCHEMA * const schema);
typedef void					(*func_xmysqlnd_node_schema__dtor)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema)
{
	func_xmysqlnd_node_schema__init init;

	func_xmysqlnd_node_schema__create_collection_object create_collection_object;
	func_xmysqlnd_node_schema__create_collection create_collection;
	func_xmysqlnd_node_schema__drop_collection drop_collection;
	func_xmysqlnd_node_schema__create_table_object create_table_object;

	func_xmysqlnd_node_schema__get_db_objects get_db_objects;

	func_xmysqlnd_node_schema__get_reference get_reference;
	func_xmysqlnd_node_schema__free_reference free_reference;

	func_xmysqlnd_node_schema__free_contents free_contents;
	func_xmysqlnd_node_schema__dtor dtor;
};

struct st_xmysqlnd_node_schema_data
{
	struct st_xmysqlnd_node_session * session;
	MYSQLND_STRING schema_name;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema) m;
	zend_bool		persistent;
};


struct st_xmysqlnd_node_schema
{
	XMYSQLND_NODE_SCHEMA_DATA * data;

	zend_bool		persistent;
};


PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_schema);
PHPAPI XMYSQLND_NODE_SCHEMA * xmysqlnd_node_schema_create(struct st_xmysqlnd_node_session * session,
														  const MYSQLND_CSTRING schema_name,
														  const zend_bool persistent,
														  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
														  MYSQLND_STATS * const stats,
														  MYSQLND_ERROR_INFO * const error_info);

PHPAPI void xmysqlnd_node_schema_free(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_SCHEMA_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
