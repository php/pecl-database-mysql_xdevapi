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
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_sql_statement_result_class_entry;

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

#define MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_sql_statement_result*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_sql_stmt_result_on_warning */
static const enum_hnd_func_status
mysqlx_sql_stmt_result_on_warning(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const enum xmysqlnd_stmt_warning_level /*level*/,
	const unsigned int /*code*/,
	const MYSQLND_CSTRING /*message*/)
{
	DBG_ENTER("mysqlx_sql_stmt_result_on_warning");
	//php_error_docref(nullptr, E_WARNING, "[%d] %*s", code, message.l, message.s);
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ mysqlx_sql_stmt_result_on_error */
static const enum_hnd_func_status
mysqlx_sql_stmt_result_on_error(
	void * /*context*/,
	xmysqlnd_stmt * const /*stmt*/,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_sql_stmt_result_on_error");
	mysqlx_new_exception(code, sql_state, message);
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ mysqlx_sql_statement_read_next_result */
static int mysqlx_sql_statement_read_next_result(st_mysqlx_sql_statement_result* object)
{
	int nextResult{0};
	if (PASS == object->send_query_status) {
		xmysqlnd_stmt * stmt = object->stmt;

		const struct st_xmysqlnd_stmt_on_warning_bind on_warning = { mysqlx_sql_stmt_result_on_warning, nullptr };
		const struct st_xmysqlnd_stmt_on_error_bind on_error = { mysqlx_sql_stmt_result_on_error, nullptr };
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
/* }}} */


/* {{{ mysqlx_sql_statement_result::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::hasData(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, hasData)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::hasData");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_BOOL(object && object->result && FALSE == object->result->m.eof(object->result));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::fetchOne(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, fetchOne)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	RETVAL_NULL();

	DBG_ENTER("mysqlx_sql_statement_result::fetchOne");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	if (object && object->result && FALSE == object->result->m.eof(object->result)) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == object->result->m.fetch_current(object->result, &row, nullptr, nullptr)) {
			ZVAL_COPY_VALUE(return_value, &row);
			object->result->m.next(object->result, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::fetchAll(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, fetchAll)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	RETVAL_NULL();

	DBG_ENTER("mysqlx_sql_statement_result::fetchAll");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	if (object && object->result) {
		zval set;
		ZVAL_UNDEF(&set);
		if (PASS == object->result->m.fetch_all(object->result, &set, nullptr, nullptr)) {
			ZVAL_COPY_VALUE(return_value, &set);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getAffectedItemsCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getAffectedItemsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getAffectedItemsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_STMT_EXECUTION_STATE * exec_state = object->result->exec_state;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (exec_state) {
			const size_t value = exec_state->m->get_affected_items_count(exec_state);
			if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
				ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
				DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
			} else {
				ZVAL_LONG(return_value, value);
				DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getLastInsertId(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getLastInsertId)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getLastInsertId");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->result->exec_state) {
		const XMYSQLND_STMT_EXECUTION_STATE * const exec_state = object->result->exec_state;
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
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getGeneratedIds(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getGeneratedIds)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getGeneratedIds");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->result->exec_state) {
		const XMYSQLND_STMT_EXECUTION_STATE * const exec_state = object->result->exec_state;
		if ( exec_state == nullptr ) {
			php_error_docref(nullptr, E_WARNING, "Unable to get the correct exec_state");
			DBG_VOID_RETURN;
		}
		auto& ids = exec_state->generated_doc_ids;
		const size_t num_of_docs = ids.size();
		array_init_size(return_value, static_cast<uint32_t>(num_of_docs));
		for( auto& elem : ids ) {
			zval id;
			ZVAL_STRINGL(&id,elem.c_str(),elem.size());
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &id);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getWarningsCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getWarningsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getWarningsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_WARNING_LIST * const warnings = object->result->warnings;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (warnings) {
			const size_t value = warnings->m->count(warnings);
			if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
				ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
				DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
			} else {
				ZVAL_LONG(return_value, value);
				DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getWarnings(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getWarnings)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getWarnings");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_WARNING_LIST * const warnings = object->result->warnings;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (warnings) {
			const unsigned int count{warnings->m->count(warnings)};
			array_init_size(return_value, count);
			for (unsigned int i{0}; i < count; ++i) {
				const XMYSQLND_WARNING warning = warnings->m->get_warning(warnings, i);
				zval warning_zv;

				ZVAL_UNDEF(&warning_zv);
				mysqlx_new_warning(&warning_zv, warning.message, warning.level, warning.code);

				if (Z_TYPE(warning_zv) != IS_UNDEF) {
					zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &warning_zv);
				}
			}
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ get_stmt_result_meta */
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
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getColumnCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumnCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getColumnCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		st_xmysqlnd_stmt_result_meta* meta = get_stmt_result_meta(object->result);
		if (meta)
		{
			const size_t value = meta->m->get_field_count(meta);
			if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
				ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
				DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
			} else {
				ZVAL_LONG(return_value, value);
				DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
			}
		} else {
			array_init_size(return_value, 0);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getColumns(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumns)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getColumns");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		st_xmysqlnd_stmt_result_meta* meta = get_stmt_result_meta(object->result);
		/* Maybe check here if there was an error and throw an Exception or return a column */
		if (meta) {
			const unsigned int count{meta->m->get_field_count(meta)};
			array_init_size(return_value, count);
			for (unsigned int i{0}; i < count; ++i) {
				const XMYSQLND_RESULT_FIELD_META* column = meta->m->get_field(meta, i);
				zval column_zv;

				ZVAL_UNDEF(&column_zv);
				mysqlx_new_column_result(&column_zv, column);

				if (Z_TYPE(column_zv) != IS_UNDEF) {
					zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &column_zv);
				}
			}
		} else {
			array_init_size(return_value, 0);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::getColumnNames(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, getColumnNames)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::getColumns");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		st_xmysqlnd_stmt_result_meta* meta = get_stmt_result_meta(object->result);
		/* Maybe check here if there was an error and throw an Exception or return a column */
		if (meta) {
			const unsigned int count{meta->m->get_field_count(meta)};
			array_init_size(return_value, count);
			for (unsigned int i{0}; i < count; ++i) {
				const XMYSQLND_RESULT_FIELD_META* column = meta->m->get_field(meta, i);
				zval column_name;

				ZVAL_UNDEF(&column_name);
				ZVAL_STRINGL(&column_name, column->name.s, column->name.l);

				if (Z_TYPE(column_name) != IS_UNDEF) {
					zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &column_name);
				}
			}
		} else {
			array_init_size(return_value, 0);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_sql_statement_result::nextResult() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_sql_statement_result, nextResult)
{
	zval* object_zv{nullptr};
	st_mysqlx_sql_statement_result* object{nullptr};

	DBG_ENTER("mysqlx_sql_statement_result::nextResult");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_sql_statement_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_SQL_STATEMENT_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->has_more_results) {
		if (mysqlx_sql_statement_read_next_result(object)) {
			RETVAL_TRUE;
		}
	}
}
/* }}} */


/* {{{ mysqlx_sql_statement_result_methods[] */
static const zend_function_entry mysqlx_sql_statement_result_methods[] = {
	PHP_ME(mysqlx_sql_statement_result, __construct,			nullptr,																ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_sql_statement_result, hasData,				arginfo_mysqlx_sql_statement_result__has_data,					ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, fetchOne,				arginfo_mysqlx_sql_statement_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, fetchAll,				arginfo_mysqlx_sql_statement_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, getAffectedItemsCount,	arginfo_mysqlx_sql_statement_result__get_affected_items_count,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getLastInsertId,		arginfo_mysqlx_sql_statement_result__get_last_insert_id,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getGeneratedIds,		arginfo_mysqlx_sql_statement_result__get_generated_ids,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getWarningsCount,		arginfo_mysqlx_sql_statement_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getWarnings,			arginfo_mysqlx_sql_statement_result__get_warnings, 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, getColumnCount,		arginfo_mysqlx_sql_statement_result__get_column_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getColumnNames,		arginfo_mysqlx_sql_statement_result__get_column_names,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_sql_statement_result, getColumns,			arginfo_mysqlx_sql_statement_result__get_columns,	 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_sql_statement_result, nextResult,			arginfo_mysqlx_sql_statement_result__next_result,			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_sql_statement_result_handlers;
static HashTable mysqlx_sql_statement_result_properties;

const struct st_mysqlx_property_entry mysqlx_sql_statement_result_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_sql_statement_result_free_storage */
static void
mysqlx_sql_statement_result_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_sql_statement_result* inner_obj = (st_mysqlx_sql_statement_result*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->stmt) {
			xmysqlnd_stmt_free(inner_obj->stmt, nullptr, nullptr);
		}
		if (inner_obj->result) {
			xmysqlnd_stmt_result_free(inner_obj->result, nullptr, nullptr);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_sql_statement_result_object_allocator */
static zend_object *
php_mysqlx_sql_statement_result_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_sql_statement_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_sql_statement_result>(
		class_type,
		&mysqlx_object_sql_statement_result_handlers,
		&mysqlx_sql_statement_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_sql_statement_result_class */
void
mysqlx_register_sql_statement_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_sql_statement_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_sql_statement_result_handlers.free_obj = mysqlx_sql_statement_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "SqlStatementResult", mysqlx_sql_statement_result_methods);
		tmp_ce.create_object = php_mysqlx_sql_statement_result_object_allocator;

		mysqlx_sql_statement_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_sql_statement_result_class_entry, 1, mysqlx_base_result_interface_entry);

		mysqlx_register_sql_statement_result_iterator(mysqlx_sql_statement_result_class_entry);
	}

	zend_hash_init(&mysqlx_sql_statement_result_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_sql_statement_result_properties, mysqlx_sql_statement_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_sql_statement_result_class */
void
mysqlx_unregister_sql_statement_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_sql_statement_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_sql_stmt_result */
void
mysqlx_new_sql_stmt_result(zval * return_value, XMYSQLND_STMT_RESULT * result, st_mysqlx_statement* stmt)
{
	DBG_ENTER("mysqlx_new_sql_stmt_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_sql_statement_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_sql_statement_result* const object = (st_mysqlx_sql_statement_result*) mysqlx_object->ptr;
		if (object) {
			object->result = result;
			stmt->stmt->get_reference(stmt->stmt);
			object->stmt = stmt->stmt;
			object->execute_flags = stmt->execute_flags;
			object->send_query_status = stmt->send_query_status;
			object->has_more_results = stmt->has_more_results;
			object->has_more_rows_in_set = stmt->has_more_rows_in_set;
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
