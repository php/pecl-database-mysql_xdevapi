/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#include "php_api.h"
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_collection.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_node_table.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_rowset.h"
#include "xmysqlnd_rowset_fwd.h"
#include "xmysqlnd_rowset_buffered.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_object_factory.h"

namespace mysqlx {

namespace drv {

/* {{{ mysqlnd_object_factory::get_node_session */
static XMYSQLND_NODE_SESSION *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														   const zend_bool persistent,
														   MYSQLND_STATS* stats,
														   MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_node_session");
	DBG_INF_FMT("persistent=%u", persistent);
	XMYSQLND_NODE_SESSION* object = new XMYSQLND_NODE_SESSION;
	object->persistent = persistent;
	object->m = xmysqlnd_node_session_get_methods();

	if (FAIL == object->m->init(object, factory, stats, error_info)) {
		object->m->dtor(object);
		DBG_RETURN(nullptr);
	}

	DBG_RETURN(object);
}
/* }}} */


/* {{{ mysqlnd_object_factory::get_node_session_data */
static st_xmysqlnd_node_session_data *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session_data)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
                                                                const zend_bool persistent,
                                                                MYSQLND_STATS* stats,
                                                                MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_node_session_data");
	DBG_INF_FMT("persistent=%u", persistent);
        st_xmysqlnd_node_session_data * object = new st_xmysqlnd_node_session_data;
	object->persistent = persistent;
        object->m = xmysqlnd_node_session_data_get_methods();

	if (FAIL == object->m->init(object, factory, stats, error_info)) {
                object->m->dtor(object);
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_schema */
static XMYSQLND_NODE_SCHEMA *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_schema)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														  XMYSQLND_NODE_SESSION* session,
														  const MYSQLND_CSTRING schema_name,
														  const zend_bool persistent,
														  MYSQLND_STATS* stats,
														  MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_SCHEMA* object = new XMYSQLND_NODE_SCHEMA;
	XMYSQLND_NODE_SCHEMA_DATA* object_data = new XMYSQLND_NODE_SCHEMA_DATA;

	DBG_ENTER("xmysqlnd_object_factory::get_node_schema");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_node_schema_get_methods();

		if (PASS != object->data->m.init(object, factory, session, schema_name, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	} else {
		delete object_data;
		delete object;
		object = nullptr;
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_collection */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_collection)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
															  XMYSQLND_NODE_SCHEMA* schema,
															  const MYSQLND_CSTRING collection_name,
															  const zend_bool persistent,
															  MYSQLND_STATS* stats,
															  MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_COLLECTION* object = new XMYSQLND_NODE_COLLECTION;
	XMYSQLND_NODE_COLLECTION_DATA* object_data = new XMYSQLND_NODE_COLLECTION_DATA;

	DBG_ENTER("xmysqlnd_object_factory::get_node_collection");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_node_collection_get_methods();

		if (PASS != object->data->m.init(object, factory, schema, collection_name, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	} else {
		delete object_data;
		delete object;
		object = nullptr;
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_table */
static XMYSQLND_NODE_TABLE *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_table)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														 XMYSQLND_NODE_SCHEMA* schema,
														 const MYSQLND_CSTRING table_name,
														 const zend_bool persistent,
														 MYSQLND_STATS* stats,
														 MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_TABLE* object = new XMYSQLND_NODE_TABLE;
	XMYSQLND_NODE_TABLE_DATA* object_data = new XMYSQLND_NODE_TABLE_DATA;

	DBG_ENTER("xmysqlnd_object_factory::get_node_table");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_node_table_get_methods();

		if (PASS != object->data->m.init(object, factory, schema, table_name, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	} else {
		delete object_data;
		delete object;
		object = nullptr;
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														XMYSQLND_NODE_SESSION* session,
														const zend_bool persistent,
														MYSQLND_STATS* stats,
														MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_STMT* object = new XMYSQLND_NODE_STMT;
	XMYSQLND_NODE_STMT_DATA* object_data = new XMYSQLND_NODE_STMT_DATA;

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_node_stmt_get_methods();

		if (PASS != object->data->m.init(object, factory, session, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	} else {
		delete object_data;
		delete object;
		object = nullptr;
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt_result */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
															   const zend_bool persistent,
															   MYSQLND_STATS* stats,
															   MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_STMT_RESULT* object = new XMYSQLND_NODE_STMT_RESULT;

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt_result");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_node_stmt_result_get_methods();

		if (PASS != object->m.init(object, factory, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_rowset_buffered */
static XMYSQLND_ROWSET_BUFFERED *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_buffered)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
															  XMYSQLND_NODE_STMT* stmt,
															  const zend_bool persistent,
															  MYSQLND_STATS* stats,
															  MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_ROWSET_BUFFERED* object = new XMYSQLND_ROWSET_BUFFERED;

	DBG_ENTER("xmysqlnd_object_factory::get_rowset_buffered");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_rowset_buffered_get_methods();

		if (PASS != object->m.init(object, factory, stmt, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_rowset_fwd */
static XMYSQLND_ROWSET_FWD *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_fwd)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														 const size_t prefetch_rows,
														 XMYSQLND_NODE_STMT* stmt,
														 const zend_bool persistent,
														 MYSQLND_STATS* stats,
														 MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_ROWSET_FWD* object = new XMYSQLND_ROWSET_FWD;

	DBG_ENTER("xmysqlnd_object_factory::get_rowset_fwd");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_rowset_fwd_get_methods();

		if (PASS != object->m.init(object, factory, prefetch_rows, stmt, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_rowset */
static XMYSQLND_ROWSET *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
													 unsigned int type,
													 const size_t prefetch_rows,
													 XMYSQLND_NODE_STMT* stmt,
													 const zend_bool persistent,
													 MYSQLND_STATS* stats,
													 MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_ROWSET* object = new XMYSQLND_ROWSET;

	DBG_ENTER("xmysqlnd_object_factory::get_rowset");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_rowset_get_methods();

		if (PASS != object->m.init(object, factory, static_cast<xmysqlnd_rowset_type>(type), prefetch_rows, stmt, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt_result_meta */
static XMYSQLND_NODE_STMT_RESULT_META *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_meta)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
																	const zend_bool persistent,
																	MYSQLND_STATS* stats,
																	MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_NODE_STMT_RESULT_META* object = new XMYSQLND_NODE_STMT_RESULT_META;

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt_result_meta");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_node_stmt_result_meta_get_methods();

		if (PASS != object->m->init(object, stats, error_info)) {
			object->m->dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_result_field_meta */
static XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_result_field_meta)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
																const zend_bool persistent,
																MYSQLND_STATS* stats,
																MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_RESULT_FIELD_META* object = new XMYSQLND_RESULT_FIELD_META;

	DBG_ENTER("xmysqlnd_object_factory::get_result_field_meta");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_result_field_meta_get_methods();

		if (PASS != object->m->init(object, factory, stats, error_info)) {
			object->m->dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_pfc */
static XMYSQLND_PFC *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_pfc)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
												  const zend_bool persistent,
												  MYSQLND_STATS* stats,
												  MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_PFC* object = new XMYSQLND_PFC;
	XMYSQLND_PFC_DATA* object_data = new XMYSQLND_PFC_DATA;

	DBG_ENTER("xmysqlnd_object_factory::get_pfc");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_pfc_get_methods();

		if (PASS != object->data->m.init(object, factory, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = nullptr;
		}
		object->data->max_packet_size = XMYSQLND_MAX_PACKET_SIZE;
	} else {
		delete object_data;
		delete object;
		object = nullptr;
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_warning_list */
static XMYSQLND_WARNING_LIST *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_warning_list)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														   const zend_bool persistent,
														   MYSQLND_STATS* stats,
														   MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_WARNING_LIST* object = new XMYSQLND_WARNING_LIST;

	DBG_ENTER("xmysqlnd_object_factory::get_warning_list");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_warning_list_get_methods();

		if (PASS != object->m->init(object, factory, stats, error_info)) {
			object->m->dtor(object);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_stmt_execution_state */
static XMYSQLND_STMT_EXECUTION_STATE *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_execution_state)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
																   const zend_bool persistent,
																   MYSQLND_STATS* stats,
																   MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_STMT_EXECUTION_STATE* object = new XMYSQLND_STMT_EXECUTION_STATE;

	DBG_ENTER("xmysqlnd_object_factory::get_stmt_execution_state");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_stmt_execution_state_get_methods();

		if (PASS != object->m->init(object, factory, stats, error_info)) {
			object->m->dtor(object);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_object_factory)
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session_data),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_schema),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_collection),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_table),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_buffered),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_fwd),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_result_field_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_pfc),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_warning_list),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_execution_state),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_object_factory);

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
