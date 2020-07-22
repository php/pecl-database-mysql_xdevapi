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
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_rowset.h"
#include "xmysqlnd/xmysqlnd_rowset_buffered.h"
#include "xmysqlnd/xmysqlnd_rowset_fwd.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_result_iterator.h"
#include "mysqlx_result.h"
#include "mysqlx_base_result.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_result__get_affected_items_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_result__get_auto_increment_value, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_result__get_generated_ids, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_result__get_warnings_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

st_mysqlx_result::~st_mysqlx_result()
{
	xmysqlnd_stmt_result_free(result, nullptr, nullptr);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getAffectedItemsCount)
{
	DBG_ENTER("mysqlx_result::getAffectedItemsCount");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_result>(object_zv) };

	const XMYSQLND_STMT_EXECUTION_STATE* exec_state = data_object.result->exec_state;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (exec_state) {
		const size_t value = exec_state->m->get_affected_items_count(exec_state);
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

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getAutoIncrementValue)
{
	DBG_ENTER("mysqlx_result::getAutoIncrementValue");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_result>(object_zv) };

	const XMYSQLND_STMT_EXECUTION_STATE* const exec_state = data_object.result->exec_state;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (exec_state) {
		const uint64_t value = exec_state->m->get_last_insert_id(exec_state);
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			const auto& value_str{ util::to_string(value) };
			ZVAL_NEW_STR(return_value, strpprintf(0, "%s", value_str.c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			ZVAL_LONG(return_value, static_cast<zend_long>(value));
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getGeneratedIds)
{
    DBG_ENTER("mysqlx_result::getGeneratedIds");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_result>(object_zv) };
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

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getWarningsCount)
{
	DBG_ENTER("mysqlx_result::getWarningsCount");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_result>(object_zv) };
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

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getWarnings)
{
	DBG_ENTER("mysqlx_result::getWarnings");
	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_result>(object_zv) };
	const XMYSQLND_WARNING_LIST* const warnings = data_object.result->warnings;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (warnings) {
		const std::size_t count{warnings->count()};
		array_init_size(return_value, static_cast<uint32_t>(count));
		for (std::size_t i{0}; i < count; ++i) {
			const XMYSQLND_WARNING warning = warnings->get_warning(i);
			util::zvalue warning_zv;
			mysqlx_new_warning(warning_zv.ptr(), warning.message, warning.level, warning.code);
			if (!warning_zv.is_undef()) {
				zend_hash_next_index_insert(Z_ARRVAL_P(return_value), warning_zv.ptr());
				warning_zv.invalidate();
			}
		}
	}

	util::zend::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_result_methods[] = {
	PHP_ME(mysqlx_result, __construct,			nullptr,														ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_result, getAffectedItemsCount,	arginfo_mysqlx_result__get_affected_items_count,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getAutoIncrementValue, 	arginfo_mysqlx_result__get_auto_increment_value,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getGeneratedIds,			arginfo_mysqlx_result__get_generated_ids,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getWarningsCount,			arginfo_mysqlx_result__get_warnings_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getWarnings,				arginfo_mysqlx_result__get_warnings, 				ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_result_handlers;
static HashTable mysqlx_result_properties;

const st_mysqlx_property_entry mysqlx_result_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_result_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_result>(object);
}

static zend_object *
php_mysqlx_result_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_result>(
		class_type,
		&mysqlx_object_result_handlers,
		&mysqlx_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_result_class_entry,
		"Result",
		mysqlx_std_object_handlers,
		mysqlx_object_result_handlers,
		php_mysqlx_result_object_allocator,
		mysqlx_result_free_storage,
		mysqlx_result_methods,
		mysqlx_result_properties,
		mysqlx_result_property_entries,
		mysqlx_base_result_interface_entry);

	mysqlx_register_result_iterator(mysqlx_result_class_entry);
}

void
mysqlx_unregister_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_result_properties);
}

void
mysqlx_new_result(zval* return_value, XMYSQLND_STMT_RESULT* result)
{
	DBG_ENTER("mysqlx_new_result");

	st_mysqlx_result& data_object{
		util::init_object<st_mysqlx_result>(mysqlx_result_class_entry, return_value) };
	data_object.result = result;

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
