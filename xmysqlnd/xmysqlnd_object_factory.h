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
#ifndef XMYSQLND_OBJECT_FACTORY_H
#define XMYSQLND_OBJECT_FACTORY_H

#include "php_mysql_xdevapi.h"
#include <memory>
#include "util/strings.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_session;
typedef std::shared_ptr< xmysqlnd_session > XMYSQLND_SESSION;
class xmysqlnd_session_data;
class xmysqlnd_schema;
struct st_xmysqlnd_schema_data;
struct xmysqlnd_collection;
class xmysqlnd_stmt;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_stmt_rowset;
struct st_xmysqlnd_rowset_buffered;
struct st_xmysqlnd_rowset_fwd;
struct st_xmysqlnd_result_field_meta;
struct st_xmysqlnd_protocol_frame_codec;
class xmysqlnd_warning_list;
struct st_xmysqlnd_stmt_execution_state;

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory);

typedef class xmysqlnd_session * (*func_xmysqlnd_object_factory__get_session)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef class xmysqlnd_session_data* (*func_xmysqlnd_object_factory__get_session_data)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef class xmysqlnd_schema * (*func_xmysqlnd_object_factory__get_schema)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			XMYSQLND_SESSION session,
			const util::string_view& schema_name,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct xmysqlnd_collection * (*func_xmysqlnd_object_factory__get_collection)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			xmysqlnd_schema * schema,
			const util::string_view& collection_name,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct xmysqlnd_table * (*func_xmysqlnd_object_factory__get_table)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			class xmysqlnd_schema * schema,
			const util::string_view& table_name,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef class xmysqlnd_stmt * (*func_xmysqlnd_object_factory__get_stmt)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			XMYSQLND_SESSION session,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_stmt_result * (*func_xmysqlnd_object_factory__get_stmt_result)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_rowset_buffered * (*func_xmysqlnd_object_factory__get_rowset_buffered)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			class xmysqlnd_stmt * stmt,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_rowset_fwd *	(*func_xmysqlnd_object_factory__get_rowset_fwd)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const size_t prefetch_rows,
			class xmysqlnd_stmt * stmt,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_rowset * (*func_xmysqlnd_object_factory__get_rowset)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			unsigned int type,
			const size_t prefetch_rows,
			class xmysqlnd_stmt * stmt,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_stmt_result_meta * (*func_xmysqlnd_object_factory__get_stmt_result_meta)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_result_field_meta * (*func_xmysqlnd_object_factory__get_result_field_meta)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_protocol_frame_codec * (*func_xmysqlnd_object_factory__get_pfc)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef xmysqlnd_warning_list * (*func_xmysqlnd_object_factory__get_warnings_list)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

typedef struct st_xmysqlnd_stmt_execution_state *(*func_xmysqlnd_object_factory__get_stmt_execution_state)(
			const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
			const zend_bool persistent,
			MYSQLND_STATS * stats,
			MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory)
{
	func_xmysqlnd_object_factory__get_session get_session;
	func_xmysqlnd_object_factory__get_session_data get_session_data;
	func_xmysqlnd_object_factory__get_schema get_schema;
	func_xmysqlnd_object_factory__get_collection get_collection;
	func_xmysqlnd_object_factory__get_table get_table;
	func_xmysqlnd_object_factory__get_stmt get_stmt;
	func_xmysqlnd_object_factory__get_stmt_result get_stmt_result;
	func_xmysqlnd_object_factory__get_rowset_buffered get_rowset_buffered;
	func_xmysqlnd_object_factory__get_rowset_fwd get_rowset_fwd;
	func_xmysqlnd_object_factory__get_rowset get_rowset;
	func_xmysqlnd_object_factory__get_stmt_result_meta get_stmt_result_meta;
	func_xmysqlnd_object_factory__get_result_field_meta get_result_field_meta;
	func_xmysqlnd_object_factory__get_pfc get_protocol_frame_codec;
	func_xmysqlnd_object_factory__get_warnings_list get_warnings_list;
	func_xmysqlnd_object_factory__get_stmt_execution_state get_stmt_execution_state;
};

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_object_factory);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_OBJECT_FACTORY_H */
