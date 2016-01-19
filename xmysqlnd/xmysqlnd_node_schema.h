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

struct st_xmysqlnd_node_session_data;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_xmysqlnd_node_schema		XMYSQLND_NODE_SCHEMA;
typedef struct st_xmysqlnd_node_schema_data	XMYSQLND_NODE_SCHEMA_DATA;


typedef enum_func_status		(*func_xmysqlnd_node_schema__init)(XMYSQLND_NODE_SCHEMA * const schema, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, struct st_xmysqlnd_node_session_data * const session, const MYSQLND_CSTRING schema_name, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);

typedef XMYSQLND_NODE_SCHEMA *	(*func_xmysqlnd_node_schema__get_reference)(XMYSQLND_NODE_SCHEMA * const schema);
typedef enum_func_status		(*func_xmysqlnd_node_schema__free_reference)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void					(*func_xmysqlnd_node_schema__free_contents)(XMYSQLND_NODE_SCHEMA * const schema);
typedef void					(*func_xmysqlnd_node_schema__dtor)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema)
{
	func_xmysqlnd_node_schema__init init;

	func_xmysqlnd_node_schema__get_reference get_reference;
	func_xmysqlnd_node_schema__free_reference free_reference;

	func_xmysqlnd_node_schema__free_contents free_contents;
	func_xmysqlnd_node_schema__dtor dtor;
};

struct st_xmysqlnd_node_schema_data
{
	struct st_xmysqlnd_node_session_data * session;
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
PHPAPI XMYSQLND_NODE_SCHEMA * xmysqlnd_node_schema_create(struct st_xmysqlnd_node_session_data * session, const MYSQLND_CSTRING schema_name, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
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
