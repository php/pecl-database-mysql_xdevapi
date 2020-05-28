/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "mysqlnd_api.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_schema.h"
#include "xmysqlnd_collection.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_table.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_rowset.h"
#include "xmysqlnd_rowset_fwd.h"
#include "xmysqlnd_rowset_buffered.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "xmysqlnd_extension_plugin.h"

namespace mysqlx {

namespace drv {

static void **
xmysqlnd_plugin__get_session_plugin_area(const xmysqlnd_session * session, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_connection_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!session || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_session*>(session) + sizeof(xmysqlnd_session) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_session_data_plugin_area(const xmysqlnd_session_data * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_session_data_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
		DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_session_data*>(object) + sizeof(XMYSQLND_SESSION_DATA) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_schema_plugin_area(const xmysqlnd_schema * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_schema_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_schema*>(object) + sizeof(xmysqlnd_schema) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_collection_plugin_area(const xmysqlnd_collection * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_collection_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_collection*>(object) + sizeof(xmysqlnd_collection) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_table_plugin_area(const xmysqlnd_table * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_table_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_table*>(object) + sizeof(xmysqlnd_table) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_stmt_plugin_area(const xmysqlnd_stmt * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_stmt_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<xmysqlnd_stmt*>(object) + sizeof(xmysqlnd_stmt) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_stmt_result_plugin_area(const XMYSQLND_STMT_RESULT * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_stmt_result_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_STMT_RESULT*>(object) + sizeof(XMYSQLND_STMT_RESULT) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_rowset_buffered_plugin_area(const XMYSQLND_ROWSET_BUFFERED * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_rowset_buffered_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_ROWSET_BUFFERED*>(object) + sizeof(XMYSQLND_ROWSET_BUFFERED) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_rowset_fwd_plugin_area(const XMYSQLND_ROWSET_FWD * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_rowset_fwd_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_ROWSET_FWD*>(object) + sizeof(XMYSQLND_ROWSET_FWD) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_query_result_meta_plugin_area(const XMYSQLND_STMT_RESULT_META * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_query_result_meta_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_STMT_RESULT_META*>(object) + sizeof(XMYSQLND_STMT_RESULT_META) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_rowset_plugin_area(const XMYSQLND_ROWSET * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_rowset_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_ROWSET*>(object) + sizeof(XMYSQLND_ROWSET) + plugin_id * sizeof(void *)));
}

static void **
xmysqlnd_plugin__get_plugin_pfc_data(const XMYSQLND_PFC * object, unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_plugin_pfc_data");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return nullptr;
	}
	DBG_RETURN(reinterpret_cast<void**>(const_cast<XMYSQLND_PFC*>(object) + sizeof(XMYSQLND_PFC) + plugin_id * sizeof(void *)));
}

struct st_xmysqlnd_plugin__plugin_area_getters xmysqlnd_plugin_area_getters =
{
	xmysqlnd_plugin__get_session_plugin_area,
	xmysqlnd_plugin__get_session_data_plugin_area,
	xmysqlnd_plugin__get_schema_plugin_area,
	xmysqlnd_plugin__get_collection_plugin_area,
	xmysqlnd_plugin__get_table_plugin_area,
	xmysqlnd_plugin__get_stmt_plugin_area,
	xmysqlnd_plugin__get_stmt_result_plugin_area,
	xmysqlnd_plugin__get_rowset_buffered_plugin_area,
	xmysqlnd_plugin__get_rowset_fwd_plugin_area,
	xmysqlnd_plugin__get_query_result_meta_plugin_area,
	xmysqlnd_plugin__get_rowset_plugin_area,
	xmysqlnd_plugin__get_plugin_pfc_data,
};



static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *
_xmysqlnd_object_factory_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
}

static void
_xmysqlnd_object_factory_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result) *
_xmysqlnd_stmt_result_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_result);
}

static void
_xmysqlnd_stmt_result_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_result) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_buffered) *
_xmysqlnd_rowset_buffered_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset_buffered);
}

static void
_xmysqlnd_rowset_buffered_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_buffered) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset_buffered) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_fwd) *
_xmysqlnd_rowset_fwd_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset_fwd);
}

static void
_xmysqlnd_rowset_fwd_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_fwd) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset_fwd) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result_meta) *
_xmysqlnd_stmt_result_meta_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_result_meta);
}

static void
_xmysqlnd_stmt_result_meta_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result_meta) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_result_meta) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) *
_xmysqlnd_result_field_meta_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_result_field_meta);
}

static void
_xmysqlnd_result_field_meta_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_result_field_meta) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset) *
_xmysqlnd_rowset_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset);
}

static void
_xmysqlnd_rowset_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_rowset) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) *
_xmysqlnd_pfc_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_protocol_packet_frame_codec);
}

static void
_xmysqlnd_pfc_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_protocol_packet_frame_codec) = methods;
}

static const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) *
_xmysqlnd_stmt_execution_state_get_methods()
{
	return MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_execution_state);
}

static void
_xmysqlnd_stmt_execution_state_set_methods(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) * const methods)
{
	MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_stmt_execution_state) = methods;
}

PHP_MYSQL_XDEVAPI_API struct st_xmysqlnd_plugin_methods_xetters xmysqlnd_plugin_methods_xetters =
{
	{
		_xmysqlnd_object_factory_get_methods,
		_xmysqlnd_object_factory_set_methods
	},
	{
		_xmysqlnd_stmt_result_get_methods,
		_xmysqlnd_stmt_result_set_methods,
	},
	{
		_xmysqlnd_rowset_buffered_get_methods,
		_xmysqlnd_rowset_buffered_set_methods,
	},
	{
		_xmysqlnd_rowset_fwd_get_methods,
		_xmysqlnd_rowset_fwd_set_methods,
	},
	{
		_xmysqlnd_stmt_result_meta_get_methods,
		_xmysqlnd_stmt_result_meta_set_methods,
	},
	{
		_xmysqlnd_result_field_meta_get_methods,
		_xmysqlnd_result_field_meta_set_methods,
	},
	{
		_xmysqlnd_rowset_get_methods,
		_xmysqlnd_rowset_set_methods,
	},
	{
		_xmysqlnd_pfc_get_methods,
		_xmysqlnd_pfc_set_methods,
	},
	{
		_xmysqlnd_stmt_execution_state_get_methods,
		_xmysqlnd_stmt_execution_state_set_methods,
	},
};

} // namespace drv

} // namespace mysqlx
