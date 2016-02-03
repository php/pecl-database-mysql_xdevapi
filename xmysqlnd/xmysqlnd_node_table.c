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
#include "xmysqlnd_node_table.h"

/* {{{ xmysqlnd_node_table::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, init)(XMYSQLND_NODE_TABLE * const table,
										   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										   XMYSQLND_NODE_SCHEMA * const schema,
										   const MYSQLND_CSTRING table_name,
										   MYSQLND_STATS * const stats,
										   MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_table::init");
	if (!(table->data->schema = schema->data->m.get_reference(schema))) {
		return FAIL;
	}
	table->data->table_name = mnd_dup_cstring(table_name, table->data->persistent);
	DBG_INF_FMT("name=[%d]%*s", table->data->table_name.l, table->data->table_name.l, table->data->table_name.s);

	table->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_table::get_reference */
static XMYSQLND_NODE_TABLE *
XMYSQLND_METHOD(xmysqlnd_node_table, get_reference)(XMYSQLND_NODE_TABLE * const table)
{
	DBG_ENTER("xmysqlnd_node_table::get_reference");
	++table->data->refcount;
	DBG_INF_FMT("table=%p new_refcount=%u", table, table->data->refcount);
	DBG_RETURN(table);
}
/* }}} */


/* {{{ xmysqlnd_node_table::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, free_reference)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_table::free_reference");
	DBG_INF_FMT("table=%p old_refcount=%u", table, table->data->refcount);
	if (!(--table->data->refcount)) {
		table->data->m.dtor(table, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_table, free_contents)(XMYSQLND_NODE_TABLE * const table)
{
	const zend_bool pers = table->data->persistent;
	DBG_ENTER("xmysqlnd_node_table::free_contents");
	if (table->data->table_name.s) {
		mnd_pefree(table->data->table_name.s, pers);
		table->data->table_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_table::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_table, dtor)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_table::dtor");
	if (table) {
		table->data->m.free_contents(table);

		xmysqlnd_node_schema_free(table->data->schema, stats, error_info);

		mnd_pefree(table->data, table->data->persistent);
		mnd_pefree(table, table->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_table)
	XMYSQLND_METHOD(xmysqlnd_node_table, init),

	XMYSQLND_METHOD(xmysqlnd_node_table, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_table, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_table);


/* {{{ xmysqlnd_node_table_create */
PHPAPI XMYSQLND_NODE_TABLE *
xmysqlnd_node_table_create(XMYSQLND_NODE_SCHEMA * schema,
						   const MYSQLND_CSTRING table_name,
						   const zend_bool persistent,
						   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						   MYSQLND_STATS * const stats,
						   MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_TABLE * ret = NULL;
	DBG_ENTER("xmysqlnd_node_table_create");
	if (table_name.s && table_name.l) {
		ret = object_factory->get_node_table(object_factory, schema, table_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->data->m.get_reference(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table_free */
PHPAPI void
xmysqlnd_node_table_free(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_table_free");
	DBG_INF_FMT("table=%p  table->data=%p  dtor=%p", table, table? table->data:NULL, table? table->data->m.dtor:NULL);
	if (table) {
		table->data->m.free_reference(table, stats, error_info);
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
