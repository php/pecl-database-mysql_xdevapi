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
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_extension_plugin.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result.h"
#include "xmysqlnd_node_stmt_result_buffered.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_warning_list.h"

static zend_bool xmysqlnd_library_initted = FALSE;

static struct st_mysqlnd_plugin_core xmysqlnd_plugin_core =
{
	{
		MYSQLND_PLUGIN_API_VERSION,
		"xmysqlnd",
		XMYSQLND_VERSION_ID,
		PHP_XMYSQLND_VERSION,
		"PHP License 3.01",
		"Andrey Hristov <andrey@mysql.com>",
		{
			NULL, /* will be filled later */
			mysqlnd_stats_values_names,
		},
		{
			NULL /* plugin shutdown */
		}
	}
};


/* {{{ mysqlnd_library_end */
PHPAPI void xmysqlnd_library_end(void)
{
	if (xmysqlnd_library_initted == TRUE) {
		mysqlnd_stats_end(xmysqlnd_global_stats, 1);
		xmysqlnd_global_stats = NULL;
		xmysqlnd_library_initted = FALSE;
	}
}
/* }}} */


/* {{{ mysqlnd_library_init */
PHPAPI void xmysqlnd_library_init(void)
{
	if (xmysqlnd_library_initted == FALSE) {
		xmysqlnd_library_initted = TRUE;

		xmysqlnd_node_session_module_init();

		/* Should be calloc, as mnd_calloc will reference LOCK_access*/
		mysqlnd_stats_init(&xmysqlnd_global_stats, XMYSQLND_STAT_LAST, 1);
		{
			xmysqlnd_plugin_core.plugin_header.plugin_stats.values = xmysqlnd_global_stats;
			mysqlnd_plugin_register_ex((struct st_mysqlnd_plugin_header *) &xmysqlnd_plugin_core);
		}
	}
}
/* }}} */

/* {{{ xmysqlnd_get_client_info */
PHPAPI const char * xmysqlnd_get_client_info()
{
	return PHP_XMYSQLND_VERSION;
}
/* }}} */


/* {{{ xmysqlnd_get_client_version */
PHPAPI unsigned int xmysqlnd_get_client_version()
{
	return XMYSQLND_VERSION_ID;
}
/* }}} */


/* {{{ mysqlnd_object_factory::get_node_session */
static XMYSQLND_NODE_SESSION *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory, const zend_bool persistent)
{
	const size_t alloc_size = sizeof(XMYSQLND_NODE_SESSION) + mysqlnd_plugin_count() * sizeof(void *);
	const size_t data_alloc_size = sizeof(XMYSQLND_NODE_SESSION_DATA) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_NODE_SESSION * object;
	XMYSQLND_NODE_SESSION_DATA * data;

	DBG_ENTER("xmysqlnd_object_factory::get_node_session");
	DBG_INF_FMT("persistent=%u", persistent);
	object = mnd_pecalloc(1, alloc_size, persistent);
	if (!object) {
		DBG_RETURN(NULL);
	}
	object->data = mnd_pecalloc(1, data_alloc_size, persistent);
	if (!object->data) {
		mnd_pefree(object, persistent);
		DBG_RETURN(NULL);
	}
	object->persistent = persistent;
	object->m = xmysqlnd_node_session_get_methods();
	data = object->data;

	if (FAIL == mysqlnd_error_info_init(&data->error_info_impl, persistent)) {
		object->m->dtor(object);
		DBG_RETURN(NULL);
	}
	data->error_info = &data->error_info_impl;

	data->options = &(data->options_impl);

	data->persistent = persistent;
	data->m = xmysqlnd_node_session_data_get_methods();
	data->object_factory = *factory;

	xmysqlnd_node_session_state_init(&data->state);

	data->m->get_reference(data);

	mysqlnd_stats_init(&data->stats, STAT_LAST, persistent);

	data->io.pfc = xmysqlnd_pfc_init(persistent, factory, data->stats, data->error_info);
	data->io.vio = mysqlnd_vio_init(persistent, NULL, data->stats, data->error_info);

	data->charset = mysqlnd_find_charset_name(XMYSQLND_NODE_SESSION_CHARSET);

	if (!data->io.pfc || !data->io.vio || !data->charset) {
		object->m->dtor(object);
		DBG_RETURN(NULL);
	}

	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
														XMYSQLND_NODE_SESSION_DATA * session,
														const MYSQLND_CSTRING query,
														const zend_bool persistent,
														MYSQLND_STATS * stats,
														MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_NODE_STMT) + mysqlnd_plugin_count() * sizeof(void *);
	const size_t data_alloc_size = sizeof(XMYSQLND_NODE_STMT_DATA) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_NODE_STMT * object = mnd_pecalloc(1, alloc_size, persistent);
	XMYSQLND_NODE_STMT_DATA * object_data = mnd_pecalloc(1, data_alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_node_stmt_get_methods();

		if (PASS != object->data->m.init(object, session, query, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = NULL;
		}
	} else {
		if (object_data) {
			mnd_pefree(object_data, persistent);
			object_data = NULL;
		}
		if (object) {
			mnd_pefree(object, persistent);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt_result */
static XMYSQLND_NODE_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
															   XMYSQLND_NODE_STMT * stmt,
															   const zend_bool persistent,
															   MYSQLND_STATS * stats,
															   MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_NODE_STMT_RESULT) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_NODE_STMT_RESULT * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt_result");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_node_stmt_result_get_methods();

		if (PASS != object->m.init(object, factory, stmt, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt_result_buffered */
static XMYSQLND_NODE_STMT_RESULT_BUFFERED *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_buffered)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
																	XMYSQLND_NODE_STMT * stmt,
																	const zend_bool persistent,
																	MYSQLND_STATS * stats,
																	MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_NODE_STMT_RESULT_BUFFERED) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_NODE_STMT_RESULT_BUFFERED * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt_result_buffered");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_node_stmt_result_buffered_get_methods();

		if (PASS != object->m.init(object, factory, stmt, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_node_stmt_result_meta */
static XMYSQLND_NODE_STMT_RESULT_META *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_meta)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
																	const zend_bool persistent,
																	MYSQLND_STATS * stats,
																	MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_NODE_STMT_RESULT_META) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_NODE_STMT_RESULT_META * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_node_stmt_result_meta");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_node_stmt_result_meta_get_methods();

		if (PASS != object->m->init(object, stats, error_info)) {
			object->m->dtor(object, stats, error_info);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_result_field_meta */
static XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_result_field_meta)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
																const zend_bool persistent,
																MYSQLND_STATS * stats,
																MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_RESULT_FIELD_META) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_RESULT_FIELD_META * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_result_field_meta");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_result_field_meta_get_methods();

		if (PASS != object->m->init(object, stats, error_info)) {
			object->m->dtor(object, stats, error_info);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_pfc */
static XMYSQLND_PFC *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_pfc)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
												  const zend_bool persistent,
												  MYSQLND_STATS * stats,
												  MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_PFC) + mysqlnd_plugin_count() * sizeof(void *);
	const size_t data_alloc_size = sizeof(MYSQLND_PFC_DATA) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_PFC * object = mnd_pecalloc(1, alloc_size, persistent);
	XMYSQLND_PFC_DATA * object_data = mnd_pecalloc(1, data_alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_pfc");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object && object_data) {
		object->data = object_data;
		object->persistent = object->data->persistent = persistent;
		object->data->m = *xmysqlnd_pfc_get_methods();

		if (PASS != object->data->m.init(object, stats, error_info)) {
			object->data->m.dtor(object, stats, error_info);
			object = NULL;
		}
		object->data->max_packet_size = XMYSQLND_MAX_PACKET_SIZE;
	} else {
		if (object_data) {
			mnd_pefree(object_data, persistent);
			object_data = NULL;
		}
		if (object) {
			mnd_pefree(object, persistent);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_warning_list */
static XMYSQLND_WARNING_LIST *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_warning_list)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
														   const zend_bool persistent,
														   MYSQLND_STATS * stats,
														   MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_WARNING_LIST) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_WARNING_LIST * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_warning_list");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_warning_list_get_methods();

		if (PASS != object->m->init(object, factory, stats, error_info)) {
			object->m->dtor(object);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_object_factory::get_stmt_execution_state */
static XMYSQLND_STMT_EXECUTION_STATE *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_execution_state)(MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) *factory,
																   const zend_bool persistent,
																   MYSQLND_STATS * stats,
																   MYSQLND_ERROR_INFO * error_info)
{
	const size_t alloc_size = sizeof(XMYSQLND_STMT_EXECUTION_STATE) + mysqlnd_plugin_count() * sizeof(void *);
	XMYSQLND_STMT_EXECUTION_STATE * object = mnd_pecalloc(1, alloc_size, persistent);

	DBG_ENTER("xmysqlnd_object_factory::get_stmt_execution_state");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_stmt_execution_state_get_methods();

		if (PASS != object->m->init(object, factory, stats, error_info)) {
			object->m->dtor(object);
			object = NULL;
		}
	}
	DBG_RETURN(object);
}
/* }}} */


MYSQLND_CLASS_METHODS_START(xmysqlnd_object_factory)
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_session),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_buffered),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_node_stmt_result_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_result_field_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_pfc),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_warning_list),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_execution_state),
MYSQLND_CLASS_METHODS_END;

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
