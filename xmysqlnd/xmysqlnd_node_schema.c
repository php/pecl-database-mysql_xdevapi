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
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_schema.h"


/* {{{ xmysqlnd_node_schema::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, init)(XMYSQLND_NODE_SCHEMA * const schema,
										  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										  XMYSQLND_NODE_SESSION_DATA * const session,
										  const MYSQLND_CSTRING schema_name,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_schema::init");
	if (!(schema->data->session = session->m->get_reference(session))) {
		return FAIL;
	}
	schema->data->schema_name = mnd_dup_cstring(schema_name, schema->data->persistent);
	DBG_INF_FMT("query=[%d]%*s", schema->data->schema_name.l, schema->data->schema_name.l, schema->data->schema_name.s);

	schema->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */

/* {{{ xmysqlnd_node_schema::get_reference */
static XMYSQLND_NODE_SCHEMA *
XMYSQLND_METHOD(xmysqlnd_node_schema, get_reference)(XMYSQLND_NODE_SCHEMA * const schema)
{
	DBG_ENTER("xmysqlnd_node_schema::get_reference");
	++schema->data->refcount;
	DBG_INF_FMT("schema=%p new_refcount=%u", schema, schema->data->refcount);
	DBG_RETURN(schema);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_schema, free_reference)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_schema::free_reference");
	DBG_INF_FMT("schema=%p old_refcount=%u", schema, schema->data->refcount);
	if (!(--schema->data->refcount)) {
		schema->data->m.dtor(schema, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_schema::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_schema, free_contents)(XMYSQLND_NODE_SCHEMA * const schema)
{
	const zend_bool pers = schema->data->persistent;
	DBG_ENTER("xmysqlnd_node_schema::free_contents");
	if (schema->data->schema_name.s) {
		mnd_pefree(schema->data->schema_name.s, pers);
		schema->data->schema_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_schema::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_schema, dtor)(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_schema::dtor");
	if (schema) {
		schema->data->m.free_contents(schema);
		schema->data->session->m->free_reference(schema->data->session);

		mnd_pefree(schema->data, schema->data->persistent);
		mnd_pefree(schema, schema->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_schema)
	XMYSQLND_METHOD(xmysqlnd_node_schema, init),

	XMYSQLND_METHOD(xmysqlnd_node_schema, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_schema, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_schema, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_schema);

/* {{{ xmysqlnd_node_schema_create */
PHPAPI XMYSQLND_NODE_SCHEMA *
xmysqlnd_node_schema_create(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING schema_name, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_SCHEMA * schema = NULL;
	DBG_ENTER("xmysqlnd_node_schema_create");
	if (schema_name.s && schema_name.l) {
		schema = object_factory->get_node_schema(object_factory, session, schema_name, persistent, stats, error_info);
		if (schema) {
			schema = schema->data->m.get_reference(schema);
		}
	}
	DBG_RETURN(schema);
}
/* }}} */


/* {{{ xmysqlnd_node_schema_free */
PHPAPI void
xmysqlnd_node_schema_free(XMYSQLND_NODE_SCHEMA * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_schema_free");
	DBG_INF_FMT("schema=%p  schema->data=%p  dtor=%p", schema, schema? schema->data:NULL, schema? schema->data->m.dtor:NULL);
	if (schema) {
		if (!stats) {
			stats = schema->data->session->stats;
		}
		if (!error_info) {
			error_info = schema->data->session->error_info;
		}
		schema->data->m.free_reference(schema, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
