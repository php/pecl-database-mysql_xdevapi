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
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_buffered.h"
#include "xmysqlnd_node_stmt_result_fwd.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_extension_plugin.h"

static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session) *xmysqlnd_node_session_methods;
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data) *xmysqlnd_node_session_data_methods;


/* {{{ xmysqlnd_plugin__get_node_session_plugin_area */
static void **
xmysqlnd_plugin__get_node_session_plugin_area(const XMYSQLND_NODE_SESSION * session, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_connection_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!session || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)session + sizeof(XMYSQLND_NODE_SESSION) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_session_data_plugin_area */
static void **
xmysqlnd_plugin__get_node_session_data_plugin_area(const XMYSQLND_NODE_SESSION_DATA * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_session_data_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_SESSION_DATA) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_stmt_plugin_area */
static void **
xmysqlnd_plugin__get_node_stmt_plugin_area(const XMYSQLND_NODE_STMT * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_stmt_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_STMT) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_stmt_result_plugin_area */
static void **
xmysqlnd_plugin__get_node_stmt_result_plugin_area(const XMYSQLND_NODE_STMT_RESULT * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_stmt_result_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_STMT_RESULT) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_stmt_result_buffered_plugin_area */
static void **
xmysqlnd_plugin__get_node_stmt_result_buffered_plugin_area(const XMYSQLND_NODE_STMT_RESULT_BUFFERED * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_stmt_result_buffered_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_STMT_RESULT_BUFFERED) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_stmt_result_fwd_plugin_area */
static void **
xmysqlnd_plugin__get_node_stmt_result_fwd_plugin_area(const XMYSQLND_NODE_STMT_RESULT_FWD * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_stmt_result_fwd_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_STMT_RESULT_FWD) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_node_query_result_meta_plugin_area */
static void **
xmysqlnd_plugin__get_node_query_result_meta_plugin_area(const XMYSQLND_NODE_STMT_RESULT_META * object, const unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_node_query_result_meta_plugin_area");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_NODE_STMT_RESULT_META) + plugin_id * sizeof(void *)));
}
/* }}} */


/* {{{ xmysqlnd_plugin__get_plugin_pfc_data */
static void **
xmysqlnd_plugin__get_plugin_pfc_data(const XMYSQLND_PFC * object, unsigned int plugin_id)
{
	DBG_ENTER("xmysqlnd_plugin__get_plugin_pfc_data");
	DBG_INF_FMT("plugin_id=%u", plugin_id);
	if (!object || plugin_id >= mysqlnd_plugin_count()) {
		return NULL;
	}
	DBG_RETURN((void *)((char *)object + sizeof(XMYSQLND_PFC) + plugin_id * sizeof(void *)));
}
/* }}} */

struct st_xmysqlnd_plugin__plugin_area_getters xmysqlnd_plugin_area_getters =
{
	xmysqlnd_plugin__get_node_session_plugin_area,
	xmysqlnd_plugin__get_node_session_data_plugin_area,
	xmysqlnd_plugin__get_node_stmt_plugin_area,
	xmysqlnd_plugin__get_node_stmt_result_plugin_area,
	xmysqlnd_plugin__get_node_stmt_result_buffered_plugin_area,
	xmysqlnd_plugin__get_node_stmt_result_fwd_plugin_area,
	xmysqlnd_plugin__get_node_query_result_meta_plugin_area,
	xmysqlnd_plugin__get_plugin_pfc_data,
};



/* {{{ _xmysqlnd_object_factory_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *
_xmysqlnd_object_factory_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory);
}
/* }}} */

/* {{{ _xmysqlnd_object_factory_set_methods */
static void
_xmysqlnd_object_factory_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_object_factory) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_session_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session) *
_xmysqlnd_node_session_get_methods()
{
	return xmysqlnd_node_session_methods;
}
/* }}} */

/* {{{ _xmysqlnd_node_session_set_methods */
static void
_xmysqlnd_node_session_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session) *methods)
{
	xmysqlnd_node_session_methods = methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_session_data_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data) *
_xmysqlnd_node_session_data_get_methods()
{
	return xmysqlnd_node_session_data_methods;
}
/* }}} */

/* {{{ _xmysqlnd_node_session_data_set_methods */
static void
_xmysqlnd_node_session_data_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data) * methods)
{
	xmysqlnd_node_session_data_methods = methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_stmt_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) *
_xmysqlnd_node_stmt_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt);
}
/* }}} */

/* {{{ _xmysqlnd_node_stmt_set_methods */
static void
_xmysqlnd_node_stmt_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_stmt_result_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) *
_xmysqlnd_node_stmt_result_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result);
}
/* }}} */

/* {{{ _xmysqlnd_node_stmt_result_set_methods */
static void
_xmysqlnd_node_stmt_result_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_stmt_result_buffered_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_buffered) *
_xmysqlnd_node_stmt_result_buffered_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_buffered);
}
/* }}} */

/* {{{ _xmysqlnd_node_stmt_result_buffered_set_methods */
static void
_xmysqlnd_node_stmt_result_buffered_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_buffered) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_buffered) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_stmt_result_fwd_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_fwd) *
_xmysqlnd_node_stmt_result_fwd_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_fwd);
}
/* }}} */

/* {{{ _xmysqlnd_node_stmt_result_fwd_set_methods */
static void
_xmysqlnd_node_stmt_result_fwd_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_fwd) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_fwd) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_node_stmt_result_meta_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta) *
_xmysqlnd_node_stmt_result_meta_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_meta);
}
/* }}} */

/* {{{ _xmysqlnd_node_stmt_result_meta_set_methods */
static void
_xmysqlnd_node_stmt_result_meta_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_node_stmt_result_meta) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_result_field_meta_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) *
_xmysqlnd_result_field_meta_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_result_field_meta);
}
/* }}} */

/* {{{ _xmysqlnd_result_field_meta_set_methods */
static void
_xmysqlnd_result_field_meta_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) *methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_result_field_meta) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_pfc_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) *
_xmysqlnd_pfc_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_protocol_packet_frame_codec);
}
/* }}} */


/* {{{ _xmysqlnd_pfc_set_methods */
static void
_xmysqlnd_pfc_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) * methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_protocol_packet_frame_codec) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_warning_list_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) *
_xmysqlnd_warning_list_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_warning_list);
}
/* }}} */


/* {{{ _xmysqlnd_warning_list_set_methods */
static void
_xmysqlnd_warning_list_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) * methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_warning_list) = *methods;
}
/* }}} */


/* {{{ _xmysqlnd_stmt_execution_state_get_methods */
static MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) *
_xmysqlnd_stmt_execution_state_get_methods()
{
	return &MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_stmt_execution_state);
}
/* }}} */


/* {{{ _xmysqlnd_stmt_execution_state_set_methods */
static void
_xmysqlnd_stmt_execution_state_set_methods(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) * methods)
{
	MYSQLND_CLASS_METHOD_TABLE_NAME(xmysqlnd_stmt_execution_state) = *methods;
}
/* }}} */


struct st_xmysqlnd_plugin_methods_xetters xmysqlnd_plugin_methods_xetters =
{
	{
		_xmysqlnd_object_factory_get_methods,
		_xmysqlnd_object_factory_set_methods
	},
	{
		_xmysqlnd_node_session_get_methods,
		_xmysqlnd_node_session_set_methods,
	},
	{
		_xmysqlnd_node_session_data_get_methods,
		_xmysqlnd_node_session_data_set_methods,
	},
	{
		_xmysqlnd_node_stmt_get_methods,
		_xmysqlnd_node_stmt_set_methods,
	},
	{
		_xmysqlnd_node_stmt_result_get_methods,
		_xmysqlnd_node_stmt_result_set_methods,
	},
	{
		_xmysqlnd_node_stmt_result_buffered_get_methods,
		_xmysqlnd_node_stmt_result_buffered_set_methods,
	},
	{
		_xmysqlnd_node_stmt_result_fwd_get_methods,
		_xmysqlnd_node_stmt_result_fwd_set_methods,
	},
	{
		_xmysqlnd_node_stmt_result_meta_get_methods,
		_xmysqlnd_node_stmt_result_meta_set_methods,
	},
	{
		_xmysqlnd_result_field_meta_get_methods,
		_xmysqlnd_result_field_meta_set_methods,
	},
	{
		_xmysqlnd_pfc_get_methods,
		_xmysqlnd_pfc_set_methods,
	},
	{
		_xmysqlnd_warning_list_get_methods,
		_xmysqlnd_warning_list_set_methods,
	},
	{
		_xmysqlnd_stmt_execution_state_get_methods,
		_xmysqlnd_stmt_execution_state_set_methods,
	},
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
