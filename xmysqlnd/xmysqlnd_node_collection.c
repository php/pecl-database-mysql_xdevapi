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
#include "xmysqlnd_node_collection.h"

/* {{{ xmysqlnd_node_collection::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, init)(XMYSQLND_NODE_COLLECTION * const collection,
										  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										  XMYSQLND_NODE_SCHEMA * const schema,
										  const MYSQLND_CSTRING collection_name,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_collection::init");
	if (!(collection->data->schema = schema->data->m.get_reference(schema))) {
		return FAIL;
	}
	collection->data->collection_name = mnd_dup_cstring(collection_name, collection->data->persistent);
	DBG_INF_FMT("name=[%d]%*s", collection->data->collection_name.l, collection->data->collection_name.l, collection->data->collection_name.s);

	collection->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::get_reference */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_collection, get_reference)(XMYSQLND_NODE_COLLECTION * const collection)
{
	DBG_ENTER("xmysqlnd_node_collection::get_reference");
	++collection->data->refcount;
	DBG_INF_FMT("collection=%p new_refcount=%u", collection, collection->data->refcount);
	DBG_RETURN(collection);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, free_reference)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_collection::free_reference");
	DBG_INF_FMT("collection=%p old_refcount=%u", collection, collection->data->refcount);
	if (!(--collection->data->refcount)) {
		collection->data->m.dtor(collection, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_collection, free_contents)(XMYSQLND_NODE_COLLECTION * const collection)
{
	const zend_bool pers = collection->data->persistent;
	DBG_ENTER("xmysqlnd_node_collection::free_contents");
	if (collection->data->collection_name.s) {
		mnd_pefree(collection->data->collection_name.s, pers);
		collection->data->collection_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_collection::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_collection, dtor)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_collection::dtor");
	if (collection) {
		collection->data->m.free_contents(collection);

		xmysqlnd_node_schema_free(collection->data->schema, stats, error_info);

		mnd_pefree(collection->data, collection->data->persistent);
		mnd_pefree(collection, collection->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_collection)
	XMYSQLND_METHOD(xmysqlnd_node_collection, init),

	XMYSQLND_METHOD(xmysqlnd_node_collection, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_collection, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_collection);


/* {{{ xmysqlnd_node_collection_create */
PHPAPI XMYSQLND_NODE_COLLECTION *
xmysqlnd_node_collection_create(XMYSQLND_NODE_SCHEMA * schema,
								const MYSQLND_CSTRING collection_name,
								const zend_bool persistent,
								const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								MYSQLND_STATS * const stats,
								MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_COLLECTION * ret = NULL;
	DBG_ENTER("xmysqlnd_node_collection_create");
	if (collection_name.s && collection_name.l) {
		ret = object_factory->get_node_collection(object_factory, schema, collection_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->data->m.get_reference(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection_free */
PHPAPI void
xmysqlnd_node_collection_free(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_collection_free");
	DBG_INF_FMT("collection=%p  collection->data=%p  dtor=%p", collection, collection? collection->data:NULL, collection? collection->data->m.dtor:NULL);
	if (collection) {
		collection->data->m.free_reference(collection, stats, error_info);
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
