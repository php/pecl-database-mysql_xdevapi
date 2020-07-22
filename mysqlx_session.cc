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
extern "C" {
#include <ext/standard/url.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "mysqlx_schema.h"
#include "mysqlx_sql_statement.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

zend_class_entry *mysqlx_session_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__create_schema, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, schema_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__drop_schema, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, schema_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_server_version, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__generate_uuid, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__sql, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, query, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__quote_name, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schemas, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_default_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__get_schema, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__start_transaction, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__commit, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__rollback, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__set_savepoint, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(no_pass_by_ref, savepoint_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__rollback_to, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, savepoint_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__release_savepoint, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, savepoint_name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session__close, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

bool Session_data::close_connection()
{
	if (!session) return false;

	if (session->is_pooled()) {
		session->pool_callback->on_close(session);
	} else {
		session->close(SESSION_CLOSE_EXPLICIT);
	}
	return true;
}

template<typename T>
Session_data& fetch_session_data(T* from, bool allow_closed = false)
{
	auto& data_object{ util::fetch_data_object<Session_data>(from) };
	if (!allow_closed && data_object.session->is_closed()) {
		throw util::xdevapi_exception(util::xdevapi_exception::Code::session_closed);
	}
	return data_object;
}

static zend_bool
mysqlx_throw_exception_from_session_if_needed(const XMYSQLND_SESSION_DATA session)
{
    const int error_num = session->get_error_no();
	DBG_ENTER("mysqlx_throw_exception_from_session_if_needed");
	if (error_num) {
        const char* sqlstate = session->get_sqlstate();
        const char* errmsg = session->get_error_str();
		mysqlx_new_exception(error_num, sqlstate, errmsg);
		DBG_RETURN(TRUE);
	}
	DBG_RETURN(FALSE);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, getServerVersion)
{
	DBG_ENTER("mysqlx_session::getServerVersion");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
		RETVAL_LONG(session->get_server_version());
		mysqlx_throw_exception_from_session_if_needed(session->get_data());
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, generateUUID)
{
	DBG_ENTER("mysqlx_session::generateUUID");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_FALSE;

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
		auto uuid = session->session_uuid->generate();
		if (uuid.size() > 0) {
			RETVAL_STRINGL(uuid.data(), uuid.size());
		}
		mysqlx_throw_exception_from_session_if_needed(session->data);
	}

	DBG_VOID_RETURN;
}

struct st_mysqlx_get_schemas_ctx
{
	zval* list;
};

static const enum_hnd_func_status
get_schemas_handler_on_row(void * context,
						   XMYSQLND_SESSION const session,
						   xmysqlnd_stmt * const /*stmt*/,
						   const XMYSQLND_STMT_RESULT_META * const /*meta*/,
						   const zval * const row,
						   MYSQLND_STATS * const /*stats*/,
						   MYSQLND_ERROR_INFO * const /*error_info*/)
{
	const st_mysqlx_get_schemas_ctx* ctx = (const st_mysqlx_get_schemas_ctx* ) context;
	DBG_ENTER("get_schemas_handler_on_row");
	if (ctx && ctx->list && row) {
		if (Z_TYPE_P(ctx->list) != IS_ARRAY) {
			array_init(ctx->list);
		}
		if (Z_TYPE_P(ctx->list) == IS_ARRAY) {
			const util::string_view schema_name{ Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
			xmysqlnd_schema * schema = session->create_schema_object(schema_name);
			if (schema) {
				zval zv;
				ZVAL_UNDEF(&zv);
				mysqlx_new_schema(&zv, schema);
				zend_hash_next_index_insert(Z_ARRVAL_P(ctx->list), &zv);
			}
		}
	}
	DBG_RETURN(HND_AGAIN);
}

static const enum_hnd_func_status
mysqlx_session_command_handler_on_error(
	void * /*context*/,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt * const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_session_command_handler_on_error");
    if (session) {
        xmysqlnd_session_data_handler_on_error(session->data.get(), code, sql_state, message);
	}
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, getSchemas)
{
	DBG_ENTER("mysqlx_session::getSchemas");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	RETVAL_FALSE;
	if (XMYSQLND_SESSION session = data_object.session) {
		const st_xmysqlnd_session_query_bind_variable_bind var_binder{ nullptr, nullptr };
		constexpr util::string_view list_query("SHOW DATABASES");
		zval list;
		st_mysqlx_get_schemas_ctx ctx{ &list };
		const st_xmysqlnd_session_on_result_start_bind on_result_start{ nullptr, nullptr };
		const st_xmysqlnd_session_on_row_bind on_row{ get_schemas_handler_on_row, &ctx };
		const st_xmysqlnd_session_on_warning_bind on_warning{ nullptr, nullptr };
		const st_xmysqlnd_session_on_error_bind on_error{ mysqlx_session_command_handler_on_error, nullptr };
		const st_xmysqlnd_session_on_result_end_bind on_result_end{ nullptr, nullptr };
		const st_xmysqlnd_session_on_statement_ok_bind on_statement_ok{ nullptr, nullptr };

		ZVAL_UNDEF(&list);

		if (PASS == session->query_cb(namespace_sql, list_query, var_binder, on_result_start, on_row, on_warning, on_error, on_result_end, on_statement_ok)) {
			ZVAL_COPY_VALUE(return_value, &list);
		} else {
			zval_dtor(&list);
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, getDefaultSchema)
{
	DBG_ENTER("mysqlx_session::getDefaultSchema");

	RETVAL_NULL();

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(
		execute_data,
		getThis(),
		"O", &object_zv,
		mysqlx_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
		util::string_view schema_name{ session->get_data()->default_schema };
		if (!schema_name.empty()) {
			xmysqlnd_schema* schema = session->create_schema_object(schema_name);
			if (schema) {
				mysqlx_new_schema(return_value, schema);
			} else {
				mysqlx_throw_exception_from_session_if_needed(session->data);
			}
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, getSchema)
{
	DBG_ENTER("mysqlx_session::getSchema");

	zval* object_zv{nullptr};
	util::param_string schema_name;
	if (util::zend::parse_method_parameters(execute_data,
									 getThis(),
									 "Os", &object_zv,
									 mysqlx_session_class_entry,
									&(schema_name.str), &(schema_name.len)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
		xmysqlnd_schema * schema = session->create_schema_object(schema_name.to_view());
		if (schema) {
			mysqlx_new_schema(return_value, schema);
		} else {
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

static void
mysqlx_execute_session_query(XMYSQLND_SESSION  session,
								  const std::string_view& namespace_,
								  const util::string_view& query,
								  const zend_long flags,
								  zval * const return_value,
								  const unsigned int argc,
								  const zval * args)
{
	xmysqlnd_stmt * stmt = session->create_statement_object(session);
	DBG_ENTER("mysqlx_execute_session_query");

	if (stmt) {
		zval stmt_zv;
		ZVAL_UNDEF(&stmt_zv);
		mysqlx_new_sql_stmt(&stmt_zv, stmt, namespace_, query);
		if (Z_TYPE(stmt_zv) == IS_NULL) {
			xmysqlnd_stmt_free(stmt, nullptr, nullptr);
		}
		if (Z_TYPE(stmt_zv) == IS_OBJECT) {
			zval zv;
			ZVAL_UNDEF(&zv);

			bool found{ false };
			for (unsigned int i{0}; i < argc; ++i) {
				ZVAL_UNDEF(&zv);
				mysqlx_sql_statement_bind_one_param(&stmt_zv, &args[i], &zv);
				if (Z_TYPE(zv) == IS_FALSE) {
					found = true;
					break;
				}
				zval_dtor(&zv);
			}
			if( false == found ) {
				ZVAL_UNDEF(&zv);

				mysqlx_sql_statement_execute(Z_MYSQLX_P(&stmt_zv), flags, &zv);

				ZVAL_COPY(return_value, &zv);
				zval_dtor(&zv);
			}
		}
		zval_ptr_dtor(&stmt_zv);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, sql)
{
	DBG_ENTER("mysqlx_session::sql");

	zval* object_zv{nullptr};
	XMYSQLND_SESSION session;
	util::param_string query;

	if (util::zend::parse_method_parameters(execute_data, getThis(), "Os", &object_zv, mysqlx_session_class_entry,
																	   &query.str, &query.len) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	if (query.empty()) {
		php_error_docref(nullptr, E_WARNING, "Empty query");
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if ((session = data_object.session)) {
		xmysqlnd_stmt * const stmt = session->create_statement_object(session);
		if (stmt) {
			mysqlx_new_sql_stmt(return_value, stmt, namespace_sql, query.to_view());
			if (Z_TYPE_P(return_value) == IS_NULL) {
				xmysqlnd_stmt_free(stmt, nullptr, nullptr);
				mysqlx_throw_exception_from_session_if_needed(session->data);
			}
		}
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, quoteName)
{
	DBG_ENTER("mysqlx_session::quoteName");

	zval* object_zv{nullptr};
	util::param_string name;
	if (util::zend::parse_method_parameters(execute_data, getThis(), "Os", &object_zv, mysqlx_session_class_entry,
																	   &(name.str), &(name.len)) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
        util::string quoted_name = session->data->quote_name(name.to_view());
		RETVAL_STRINGL(quoted_name.c_str(), quoted_name.length());
		mysqlx_throw_exception_from_session_if_needed(session->data);
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, createSchema)
{
	DBG_ENTER("mysqlx_session::createSchema");

	zval* object_zv{nullptr};
	util::param_string schema_name;
	if (util::zend::parse_method_parameters(execute_data, getThis(), "Os", &object_zv, mysqlx_session_class_entry,
																	   &schema_name.str, &schema_name.len) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv) };
	if (XMYSQLND_SESSION session = data_object.session) {
		xmysqlnd_schema* schema{nullptr};
		const util::string_view& schema_name_view = schema_name.to_view();
		if ((PASS == session->create_db(schema_name_view)) &&
			((schema = session->create_schema_object(schema_name_view)) != nullptr))
		{
			DBG_INF_FMT("schema=%p", schema);
			mysqlx_new_schema(return_value, schema);
		} else {
			mysqlx_throw_exception_from_session_if_needed(session->data);
		}
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, dropSchema)
{
	DBG_ENTER("mysqlx_session::dropSchema");

	zval* object_zv{nullptr};
	util::param_string schema_name;
	if (util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_session_class_entry,
		&(schema_name.str), &(schema_name.len)) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_FALSE;

	auto& data_object = fetch_session_data(object_zv);
	try {
		auto session = data_object.session;
		if (PASS == session->drop_db(schema_name.to_view())) {
			RETVAL_TRUE;
		} else {
			util::log_warning("cannot drop schema '" + schema_name.to_string() + "'");
		}
	} catch(std::exception& e) {
		util::log_warning(e.what());
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, startTransaction)
{
	DBG_ENTER("mysqlx_session::startTransaction");

	zval* object_zv{nullptr};
	constexpr util::string_view query{"START TRANSACTION"};
	zval* args{nullptr};
	int argc{0};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ fetch_session_data(object_zv) };
	if (data_object.session) {
		mysqlx_execute_session_query(data_object.session, namespace_sql, query, MYSQLX_EXECUTE_FLAG_BUFFERED, return_value, argc, args);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, commit)
{
	DBG_ENTER("mysqlx_session::commit");

	zval* object_zv{nullptr};
	constexpr util::string_view query{"COMMIT"};
	zval* args{nullptr};
	int argc{0};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ fetch_session_data(object_zv) };
	if (data_object.session) {
		mysqlx_execute_session_query(data_object.session, namespace_sql, query, MYSQLX_EXECUTE_FLAG_BUFFERED, return_value, argc, args);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, rollback)
{
	DBG_ENTER("mysqlx_session::rollback");

	zval* object_zv{nullptr};
	constexpr util::string_view query{"ROLLBACK"};
	zval* args{nullptr};
	int argc{0};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE)
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ fetch_session_data(object_zv) };
	if (data_object.session) {
		mysqlx_execute_session_query(data_object.session, namespace_sql, query, MYSQLX_EXECUTE_FLAG_BUFFERED, return_value, argc, args);
	}

	DBG_VOID_RETURN;
}

static util::string
generate_savepoint_name( const unsigned int name_seed )
{
	static const std::string SAVEPOINT_NAME_PREFIX{ "SAVEPOINT" };
	std::stringstream output;
	output << SAVEPOINT_NAME_PREFIX << name_seed;
	return output.str().c_str();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, setSavepoint)
{
	DBG_ENTER("mysqlx_session::setSavepoint");

	zval* object_zv{nullptr};
	util::param_string savepoint_name;
	if (util::zend::parse_method_parameters(
		execute_data, getThis(), "O|s",
		&object_zv, mysqlx_session_class_entry,
		&(savepoint_name.str), &(savepoint_name.len)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = fetch_session_data(object_zv);
	util::string query{ "SAVEPOINT " };
	util::string name;
	if( savepoint_name.empty() ) {
		//Generate a valid savepoint name
		name = generate_savepoint_name( data_object.session->data->savepoint_name_seed++ );
	} else {
		name = savepoint_name.to_string();
	}

	query += escape_identifier( name );

	if (data_object.session) {
		zval * args{ nullptr };
		int argc = 0;
		mysqlx_execute_session_query(
					data_object.session,
					namespace_sql,
					{query.c_str(), query.size()} ,
					MYSQLX_EXECUTE_FLAG_BUFFERED,
					return_value, argc, args);
	}
	RETVAL_STRINGL( name.c_str(), name.size() );

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, rollbackTo)
{
	DBG_ENTER("mysqlx_session::rollbackTo");

	zval* object_zv{ nullptr };
	util::param_string savepoint_name;
	if (util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_session_class_entry,
		&(savepoint_name.str), &(savepoint_name.len)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	util::string name = escape_identifier( savepoint_name.to_string() );
	auto& data_object{ fetch_session_data( object_zv) };
	const util::string query{ "ROLLBACK TO " + name };

	if (data_object.session) {
		zval * args{ nullptr };
		int argc = 0;
		mysqlx_execute_session_query(
					data_object.session,
					namespace_sql,
					{query.c_str(), query.size()} ,
					MYSQLX_EXECUTE_FLAG_BUFFERED,
					return_value, argc, args);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, releaseSavepoint)
{
	DBG_ENTER("mysqlx_session::releaseSavepoint");

	zval* object_zv{ nullptr };
	util::param_string savepoint_name;
	if (util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, mysqlx_session_class_entry,
		&(savepoint_name.str), &(savepoint_name.len)) == FAILURE) {
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	util::string name = escape_identifier( savepoint_name.to_string() );
	auto& data_object{ fetch_session_data( object_zv) };
	const util::string query{ "RELEASE SAVEPOINT " + name };

	if (data_object.session) {
		zval * args{ nullptr };
		int argc = 0;
		mysqlx_execute_session_query(
					data_object.session,
					namespace_sql,
					{query.c_str(), query.size()} ,
					MYSQLX_EXECUTE_FLAG_BUFFERED,
					return_value, argc, args);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, close)
{
	DBG_ENTER("mysqlx_session::close");

	zval* object_zv{nullptr};
	if (util::zend::parse_method_parameters(execute_data, getThis(), "O", &object_zv, mysqlx_session_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}

	auto& data_object{ fetch_session_data(object_zv, true) };
	if (data_object.close_connection()) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

static const zend_function_entry mysqlx_session_methods[] = {
	PHP_ME(mysqlx_session, __construct, 	nullptr, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_session, sql,			arginfo_mysqlx_session__sql, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, quoteName,		arginfo_mysqlx_session__quote_name, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, getServerVersion, arginfo_mysqlx_session__get_server_version, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, generateUUID, arginfo_mysqlx_session__generate_uuid, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, createSchema, arginfo_mysqlx_session__create_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, dropSchema, arginfo_mysqlx_session__drop_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, getSchemas, arginfo_mysqlx_session__get_schemas, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, getDefaultSchema, arginfo_mysqlx_session__get_default_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, getSchema, arginfo_mysqlx_session__get_schema, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, startTransaction, arginfo_mysqlx_session__start_transaction, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, commit, arginfo_mysqlx_session__commit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, rollback, arginfo_mysqlx_session__rollback, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, setSavepoint, arginfo_mysqlx_session__set_savepoint, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, rollbackTo, arginfo_mysqlx_session__rollback_to, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, releaseSavepoint, arginfo_mysqlx_session__release_savepoint, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session, close, arginfo_mysqlx_session__close, ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_session_handlers;
static HashTable mysqlx_session_properties;

const st_mysqlx_property_entry mysqlx_session_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_session_free_storage(zend_object* object)
{
	auto& data_object{ fetch_session_data(object, true) };
	data_object.close_connection();
	util::free_object<Session_data>(object);
}

static zend_object *
php_mysqlx_session_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_session_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Session_data>(
		class_type,
		&mysqlx_object_session_handlers,
		&mysqlx_session_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_session_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_session_class_entry,
		"Session",
		mysqlx_std_object_handlers,
		mysqlx_object_session_handlers,
		php_mysqlx_session_object_allocator,
		mysqlx_session_free_storage,
		mysqlx_session_methods,
		mysqlx_session_properties,
		mysqlx_session_property_entries);
}

void
mysqlx_unregister_session_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_session_properties);
}

void
mysqlx_new_session(zval* return_value)
{
	DBG_ENTER("mysqlx_new_session");

	mysqlx_new_session(return_value, drv::create_session(false));

	DBG_VOID_RETURN;
}

void
mysqlx_new_session(zval* return_value, drv::XMYSQLND_SESSION session)
{
	DBG_ENTER("mysqlx_new_session");

	Session_data& data_object{
		util::init_object<Session_data>(mysqlx_session_class_entry, return_value) };
	data_object.session = session;

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_FUNCTION(mysql_xdevapi_getSession)
{
	util::param_string uri_string;

	RETVAL_NULL();

	DBG_ENTER("mysql_xdevapi_getSession");
	if (FAILURE == util::zend::parse_function_parameters(execute_data, "s",
										 &(uri_string.str), &(uri_string.len)))
	{
		DBG_VOID_RETURN;
	}

	if (uri_string.empty()) {
		php_error_docref(nullptr, E_WARNING, "Empty URI string");
		DBG_VOID_RETURN;
	}

	drv::xmysqlnd_new_session_connect(uri_string.str,return_value);

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
