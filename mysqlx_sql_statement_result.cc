/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_rowset.h"
#include "xmysqlnd/xmysqlnd_rowset_buffered.h"
#include "xmysqlnd/xmysqlnd_rowset_fwd.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "mysqlx_sql_statement.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_sql_statement_result_iterator.h"
#include "mysqlx_sql_statement_result.h"
#include "mysqlx_base_result.h"
#include "mysqlx_column_result.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/functions.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_sql_statement_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__has_data, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_affected_items_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_last_insert_id, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_generated_ids, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_warnings_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_column_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_column_names, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__get_columns, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_sql_statement_result__next_result, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

st_mysqlx_sql_statement_result::~st_mysqlx_sql_statement_result()
{
	xmysqlnd_stmt_free(stmt, nullptr, nullptr);
	xmysqlnd_stmt_result_free(result, nullptr, nullptr);
}

static const enum_hnd_func_status
mysqlx_sql_stmt_result_on_warning(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int /*code*/,
	const util::string_view& /*message*/)
{
	DBG_ENTER("mysqlx_sql_stmt_result_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.length(), message.data());
	DBG_RETURN(HND_AGAIN);
}

static const enum_hnd_func_status
mysqlx_sql_stmt_result_on_error(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	DBG_ENTER("mysqlx_sql_stmt_result_on_error");
	create_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}

static int mysqlx_sql_statement_read_next_result(st_mysqlx_sql_statement_result* object)
{
	int nextResult{0};
	if (PASS == object->send_query_status) {
		xmysqlnd_stmt * stmt = object->stmt;

		const st_xmysqlnd_stmt_on_warning_bind on_warning = { mysqlx_sql_stmt_result_on_warning, nullptr };
		const st_xmysqlnd_stmt_on_error_bind on_error = { mysqlx_sql_stmt_result_on_error, nullptr };
		XMYSQLND_STMT_RESULT * result;

		if (object->execute_flags & MYSQLX_EXECUTE_FLAG_BUFFERED) {
			result = stmt->get_buffered_result(stmt, &object->has_more_results, on_warning, on_error, nullptr, nullptr);
		} else {
			result = stmt->get_fwd_result(stmt, MYSQLX_EXECUTE_FWD_PREFETCH_COUNT, &object->has_more_rows_in_set, &object->has_more_results, on_warning, on_error, nullptr, nullptr);
		}

		if (result) {
			st_xmysqlnd_stmt_result* prevResult = object->result;
			if (prevResult) {
				xmysqlnd_stmt_result_free(prevResult, nullptr, nullptr);
			}

			object->result = result;
			nextResult = object->has_more_results || result->rowset;
		} else {
			RAISE_EXCEPTION_FETCH_FAIL();
			/* Or we should close the connection, rendering it unusable at this point ?*/
			object->send_query_status = FAIL;
		}
	}

	return nextResult;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, hasData)
{
	DBG_ENTER("mysqlx_sql_statement_result::hasData");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	RETVAL_BOOL(FALSE == data_object.result->m.eof(data_object.result));
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, fetchOne)
{
	DBG_ENTER("mysqlx_sql_statement_result::fetchOne");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_NULL();

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };
	auto& obj_result{ data_object.result };
	if (FALSE == obj_result->m.eof(obj_result)) {
		if (PASS == obj_result->m.fetch_current(obj_result, return_value, nullptr, nullptr)) {
			obj_result->m.next(obj_result, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, fetchAll)
{
	DBG_ENTER("mysqlx_sql_statement_result::fetchAll");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };
	if (data_object.result) {
		data_object.result->m.fetch_all(data_object.result, return_value, nullptr, nullptr);
	}
	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getAffectedItemsCount)
{
	DBG_ENTER("mysqlx_sql_statement_result::getAffectedItemsCount");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	RETVAL_LONG(0);
	const XMYSQLND_STMT_EXECUTION_STATE* exec_state = data_object.result->exec_state;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (exec_state) {
		const uint64_t value = exec_state->m->get_affected_items_count(exec_state);
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			RETVAL_LONG(static_cast<zend_long>(value));
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getLastInsertId)
{
	DBG_ENTER("mysqlx_sql_statement_result::getLastInsertId");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	RETVAL_FALSE;
	const XMYSQLND_STMT_EXECUTION_STATE* const exec_state = data_object.result->exec_state;
	if (exec_state) {
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (exec_state) {
			const uint64_t value = exec_state->m->get_last_insert_id(exec_state);
			if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
				ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
				DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
			} else {
				ZVAL_LONG(return_value, static_cast<zend_long>(value));
				DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
			}
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getGeneratedIds)
{
	DBG_ENTER("mysqlx_sql_statement_result::getGeneratedIds");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	const XMYSQLND_STMT_EXECUTION_STATE* const exec_state = data_object.result->exec_state;
	if ( exec_state == nullptr ) {
		php_error_docref(nullptr, E_WARNING, "Unable to get the correct exec_state");
		DBG_VOID_RETURN;
	}
	auto& ids = exec_state->generated_doc_ids;
	util::zvalue docs(util::zvalue::create_array(ids.size()));
	for( auto& elem : ids ) {
		docs.push_back(elem);
	}
	docs.move_to(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getWarningsCount)
{
	DBG_ENTER("mysqlx_sql_statement_result::getWarningsCount");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	RETVAL_LONG(0);
	const XMYSQLND_WARNING_LIST* const warnings = data_object.result->warnings;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (warnings) {
		const std::size_t value = warnings->count();
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			RETVAL_LONG(value);
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getWarnings)
{
	DBG_ENTER("mysqlx_sql_statement_result::getWarnings");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	const XMYSQLND_WARNING_LIST * const warnings = data_object.result->warnings;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (warnings) {
		const std::size_t count{warnings->count()};
		array_init_size(return_value, static_cast<uint32_t>(count));
		for (std::size_t i{0}; i < count; ++i) {
			const XMYSQLND_WARNING warning = warnings->get_warning(i);
			util::zvalue warning_obj = create_warning(warning.message, warning.level, warning.code);
			if (!warning_obj.is_undef()) {
				zend_hash_next_index_insert(Z_ARRVAL_P(return_value), warning_obj.ptr());
				warning_obj.invalidate();
			}
		}
	}

	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

static st_xmysqlnd_stmt_result_meta* get_stmt_result_meta(st_xmysqlnd_stmt_result* stmt_result)
{
	st_xmysqlnd_stmt_result_meta* meta = 0;
	if (stmt_result && stmt_result->rowset)
	{
		switch (stmt_result->rowset->type)
		{
			case XMYSQLND_TYPE_ROWSET_BUFFERED:
				meta = stmt_result->rowset->buffered->meta;
				break;

			case XMYSQLND_TYPE_ROWSET_FWD_ONLY:
				meta = stmt_result->rowset->fwd->meta;
				break;

			default:
				assert(!"unknown rowset type!");
		}
	}
	return meta;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumnsCount)
{
	DBG_ENTER("mysqlx_sql_statement_result::getColumnsCount");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(data_object.result) };
	if (meta) {
		const size_t value{ meta->m->get_field_count(meta) };
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			ZVAL_LONG(return_value, value);
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumns)
{
	DBG_ENTER("mysqlx_sql_statement_result::getColumns");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(data_object.result) };
	/* Maybe check here if there was an error and throw an Exception or return a column */
	if (meta) {
		const unsigned int count{meta->m->get_field_count(meta)};
		array_init_size(return_value, count);
		for (unsigned int i{0}; i < count; ++i) {
			const XMYSQLND_RESULT_FIELD_META* column = meta->m->get_field(meta, i);
			util::zvalue column_obj = create_column_result(column);
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), column_obj.ptr());
			column_obj.invalidate();
		}
	}

	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumnNames)
{
	DBG_ENTER("mysqlx_sql_statement_result::getColumns");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(data_object.result) };
	/* Maybe check here if there was an error and throw an Exception or return a column */
	if (meta) {
		const unsigned int count{meta->m->get_field_count(meta)};
		array_init_size(return_value, count);
		for (unsigned int i{0}; i < count; ++i) {
			const XMYSQLND_RESULT_FIELD_META* column{ meta->m->get_field(meta, i) };
			zval column_name;

			ZVAL_UNDEF(&column_name);
			ZVAL_STRINGL(&column_name, column->name.data(), column->name.length());

			if (Z_TYPE(column_name) != IS_UNDEF) {
				zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &column_name);
			}
		}
	}

	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, nextResult)
{
	DBG_ENTER("mysqlx_sql_statement_result::nextResult");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	auto& data_object{ util::fetch_data_object<st_mysqlx_sql_statement_result>(object_zv) };

	RETVAL_FALSE;
	if (data_object.has_more_results) {
		if (mysqlx_sql_statement_read_next_result(&data_object)) {
			RETVAL_TRUE;
		}
	}
}

static const zend_function_entry mysqlx_sql_statement_result_methods[] = {
	PHP_ME(mysqlx_sql_statement_result, __construct, arginfo_mysqlx_sql_statement_result__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_sql_statement_result, hasData,				arginfo_mysqlx_sql_statement_result__has_data,					ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, fetchOne,				arginfo_mysqlx_sql_statement_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, fetchAll,				arginfo_mysqlx_sql_statement_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, getAffectedItemsCount,	arginfo_mysqlx_sql_statement_result__get_affected_items_count,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getLastInsertId,		arginfo_mysqlx_sql_statement_result__get_last_insert_id,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getGeneratedIds,		arginfo_mysqlx_sql_statement_result__get_generated_ids,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getWarningsCount,		arginfo_mysqlx_sql_statement_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getWarnings,			arginfo_mysqlx_sql_statement_result__get_warnings, 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, getColumnsCount,		arginfo_mysqlx_sql_statement_result__get_column_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getColumnNames,		arginfo_mysqlx_sql_statement_result__get_column_names,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getColumns,			arginfo_mysqlx_sql_statement_result__get_columns,	 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, nextResult,			arginfo_mysqlx_sql_statement_result__next_result,			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_sql_statement_result_handlers;
static HashTable mysqlx_sql_statement_result_properties;

const st_mysqlx_property_entry mysqlx_sql_statement_result_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_sql_statement_result_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_sql_statement_result>(object);
}

static zend_object *
php_mysqlx_sql_statement_result_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_sql_statement_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_sql_statement_result>(
		class_type,
		&mysqlx_object_sql_statement_result_handlers,
		&mysqlx_sql_statement_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_sql_statement_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_sql_statement_result_class_entry,
		"SqlStatementResult",
		mysqlx_std_object_handlers,
		mysqlx_object_sql_statement_result_handlers,
		php_mysqlx_sql_statement_result_object_allocator,
		mysqlx_sql_statement_result_free_storage,
		mysqlx_sql_statement_result_methods,
		mysqlx_sql_statement_result_properties,
		mysqlx_sql_statement_result_property_entries,
		mysqlx_base_result_interface_entry);

	mysqlx_register_sql_statement_result_iterator(mysqlx_sql_statement_result_class_entry);
}

void
mysqlx_unregister_sql_statement_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_sql_statement_result_properties);
}

util::zvalue
create_sql_stmt_result(XMYSQLND_STMT_RESULT* result, st_mysqlx_statement* stmt)
{
	DBG_ENTER("create_sql_stmt_result");

	util::zvalue sql_stmt_result_obj;
	st_mysqlx_sql_statement_result& data_object{
		util::init_object<st_mysqlx_sql_statement_result>(mysqlx_sql_statement_result_class_entry, sql_stmt_result_obj) };
	data_object.result = result;
	stmt->stmt->get_reference(stmt->stmt);
	data_object.stmt = stmt->stmt;
	data_object.execute_flags = stmt->execute_flags;
	data_object.send_query_status = stmt->send_query_status;
	data_object.has_more_results = stmt->has_more_results;
	data_object.has_more_rows_in_set = stmt->has_more_rows_in_set;

	DBG_RETURN(sql_stmt_result_obj);
}

} // namespace devapi

} // namespace mysqlx
