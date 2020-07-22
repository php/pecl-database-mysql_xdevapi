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
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_extension_plugin.h"
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
#include "xmysqlnd_warning_list.h"
#include "xmysqlnd_object_factory.h"

namespace mysqlx {

namespace drv {

static xmysqlnd_session *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_session)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														   const zend_bool persistent,
														   MYSQLND_STATS* stats,
														   MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_session");
	DBG_INF_FMT("persistent=%u", persistent);
	xmysqlnd_session* object{ nullptr };
	try{
		object = new xmysqlnd_session(factory, stats, error_info);
	}catch(std::exception&)
	{
		DBG_RETURN(nullptr);
	}
	object->persistent = persistent;

	DBG_RETURN(object);
}

static xmysqlnd_session_data *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_session_data)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
                                                                const zend_bool persistent,
                                                                MYSQLND_STATS* stats,
                                                                MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_session_data");
	DBG_INF_FMT("persistent=%u", persistent);
	xmysqlnd_session_data * object{ nullptr };
	try{
		object = new xmysqlnd_session_data( factory, stats, error_info );
	}catch(std::exception&)
	{
		DBG_RETURN(nullptr);
	}
	object->persistent = persistent;
	DBG_RETURN(object);
}

static xmysqlnd_schema *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_schema)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
													XMYSQLND_SESSION session,
													 const util::string_view& schema_name,
													 MYSQLND_STATS* stats,
													 MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_schema");
	xmysqlnd_schema*      object{nullptr};
	try{
		object = new xmysqlnd_schema(factory,
										session,
										schema_name);
	}catch(std::exception&)
	{
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(object);
}

static xmysqlnd_collection *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_collection)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
														 xmysqlnd_schema* schema,
														 const util::string_view& collection_name,
														 const zend_bool persistent,
														 MYSQLND_STATS* stats,
														 MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_collection");
	DBG_INF_FMT("persistent=%u", persistent);
	xmysqlnd_collection*      object{ nullptr };
	try{
		object = new xmysqlnd_collection(schema,
										collection_name,
										persistent);
	} catch( std::exception& /*ex*/ ) {
		DBG_RETURN(nullptr);
	}


	DBG_RETURN(object);
}

static xmysqlnd_table *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_table)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														 xmysqlnd_schema* schema,
														 const util::string_view& table_name,
														 const zend_bool persistent,
														 MYSQLND_STATS* stats,
														 MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_table");
	DBG_INF_FMT("persistent=%u", persistent);
	xmysqlnd_table* object{ nullptr };
	try{
		object = new xmysqlnd_table(factory,
									schema,
									table_name,
									persistent);
	} catch( std::exception& /*ex*/ ) {
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(object);
}

static xmysqlnd_stmt *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														XMYSQLND_SESSION session,
														const zend_bool persistent,
														MYSQLND_STATS* stats,
														MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_stmt");

	xmysqlnd_stmt* object{ nullptr };
	try{
		object = new xmysqlnd_stmt(factory, session);
	} catch( std::exception& /*ex*/ ) {
		DBG_RETURN(nullptr);
	}
	DBG_RETURN(object);
}

static XMYSQLND_STMT_RESULT *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_result)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
															   const zend_bool persistent,
															   MYSQLND_STATS* stats,
															   MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_STMT_RESULT* object = new XMYSQLND_STMT_RESULT;

	DBG_ENTER("xmysqlnd_object_factory::get_stmt_result");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->m = *xmysqlnd_stmt_result_get_methods();

		if (PASS != object->m.init(object, factory, stats, error_info)) {
			object->m.dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}

static XMYSQLND_ROWSET_BUFFERED *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_buffered)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
															  xmysqlnd_stmt* stmt,
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

static XMYSQLND_ROWSET_FWD *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_fwd)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														 const size_t prefetch_rows,
														 xmysqlnd_stmt* stmt,
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

static XMYSQLND_ROWSET *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
													 unsigned int type,
													 const size_t prefetch_rows,
													 xmysqlnd_stmt* stmt,
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

static XMYSQLND_STMT_RESULT_META *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_result_meta)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const /*factory*/,
																	const zend_bool persistent,
																	MYSQLND_STATS* stats,
																	MYSQLND_ERROR_INFO* error_info)
{
	XMYSQLND_STMT_RESULT_META* object = new XMYSQLND_STMT_RESULT_META;

	DBG_ENTER("xmysqlnd_object_factory::get_stmt_result_meta");
	DBG_INF_FMT("persistent=%u", persistent);
	if (object) {
		object->persistent = persistent;
		object->m = xmysqlnd_stmt_result_meta_get_methods();

		if (PASS != object->m->init(object, stats, error_info)) {
			object->m->dtor(object, stats, error_info);
			object = nullptr;
		}
	}
	DBG_RETURN(object);
}

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

static XMYSQLND_WARNING_LIST *
XMYSQLND_METHOD(xmysqlnd_object_factory, get_warnings_list)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)* const factory,
														   const zend_bool persistent,
														   MYSQLND_STATS* stats,
														   MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_object_factory::get_warnings_list");
	DBG_INF_FMT("persistent=%u", persistent);
	XMYSQLND_WARNING_LIST* object = new XMYSQLND_WARNING_LIST;
	DBG_RETURN(object);
}

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

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_object_factory)
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_session),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_session_data),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_schema),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_collection),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_table),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_result),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_buffered),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset_fwd),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_rowset),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_result_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_result_field_meta),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_pfc),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_warnings_list),
	XMYSQLND_METHOD(xmysqlnd_object_factory, get_stmt_execution_state),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_object_factory);

} // namespace drv

} // namespace mysqlx
