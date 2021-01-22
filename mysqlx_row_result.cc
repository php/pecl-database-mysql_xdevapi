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

#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_row_result_iterator.h"
#include "mysqlx_row_result.h"
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

static zend_class_entry *mysqlx_row_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__get_warnings_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__get_column_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__get_column_names, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_row_result__get_columns, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

st_mysqlx_row_result::~st_mysqlx_row_result()
{
	xmysqlnd_stmt_result_free(result, nullptr, nullptr);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, fetchOne)
{
	DBG_ENTER("mysqlx_row_result::fetchOne");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_NULL();

	auto& data_object{ util::fetch_data_object<st_mysqlx_row_result>(object_zv) };
	auto& obj_result{ data_object.result };
	if (FALSE == obj_result->m.eof(obj_result)) {
		if (PASS == obj_result->m.fetch_current(obj_result, return_value, nullptr, nullptr)) {
			obj_result->m.next(obj_result, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, fetchAll)
{
	DBG_ENTER("mysqlx_row_result::fetchAll");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_row_result>(object_zv) };
	data_object.result->m.fetch_all(data_object.result, return_value, nullptr, nullptr);
	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getWarningsCount)
{
	DBG_ENTER("mysqlx_row_result::getWarningsCount");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_row_result>(object_zv) };
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

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getWarnings)
{
	DBG_ENTER("mysqlx_row_result::getWarnings");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_row_result>(object_zv) };
	const XMYSQLND_WARNING_LIST* const warnings = data_object.result->warnings;
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
	if (stmt_result && stmt_result->meta)
	{
		meta = stmt_result->meta;
	}
	return meta;
}

static st_xmysqlnd_stmt_result_meta* get_stmt_result_meta(INTERNAL_FUNCTION_PARAMETERS)
{
	DBG_ENTER("get_stmt_result_meta");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_RETURN(nullptr);
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_row_result>(object_zv) };
	RETVAL_FALSE;
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(data_object.result) };

	if(meta == nullptr) {
		RAISE_EXCEPTION(10001,"get_stmt_result_meta: Unable to extract metadata");
	}

	DBG_RETURN(meta);
}


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumnsCount)
{
	DBG_ENTER("mysqlx_row_result::getColumnsCount");
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU) };

	if (meta) {
		const size_t value = meta->m->get_field_count(meta);
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(value).c_str()));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			RETVAL_LONG(value);
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	} else {
		RETVAL_LONG(0);
	}

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumns)
{
	DBG_ENTER("mysqlx_row_result::getColumns");
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU) };

	if (meta) {
		const unsigned int count{meta->m->get_field_count(meta)};
		array_init_size(return_value, count);
		for (unsigned int i{0}; i < count; ++i) {
			const XMYSQLND_RESULT_FIELD_META* column = meta->m->get_field(meta, i);
			util::zvalue column_obj = create_column_result(column);
			if (!column_obj.is_undef()) {
				zend_hash_next_index_insert(Z_ARRVAL_P(return_value), column_obj.ptr());
				column_obj.invalidate();
			}
		}
	}

	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumnNames)
{
	DBG_ENTER("mysqlx_row_result::getColumnNames");
	st_xmysqlnd_stmt_result_meta* meta{ get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU) };

	if (meta) {
		const unsigned int count{meta->m->get_field_count(meta)};
		util::zvalue column_names = util::zvalue::create_array(count);
		for (unsigned int i{0}; i < count; ++i) {
			const XMYSQLND_RESULT_FIELD_META* column = meta->m->get_field(meta, i);
			const util::string& column_name = column->name;
			column_names.push_back(column_name);
		}
		column_names.move_to(return_value);
	}

	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_row_result_methods[] = {
	PHP_ME(mysqlx_row_result, __construct, arginfo_mysqlx_row_result__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_row_result, fetchOne,				arginfo_mysqlx_row_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, fetchAll,				arginfo_mysqlx_row_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_row_result, getWarningsCount,		arginfo_mysqlx_row_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getWarnings,			arginfo_mysqlx_row_result__get_warnings, 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_row_result, getColumnsCount,		arginfo_mysqlx_row_result__get_column_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getColumnNames,		arginfo_mysqlx_row_result__get_column_names,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getColumns,			arginfo_mysqlx_row_result__get_columns,	 			ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_row_result_handlers;
static HashTable mysqlx_row_result_properties;

const st_mysqlx_property_entry mysqlx_row_result_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_row_result_free_storage(zend_object* object)
{
	util::free_object<st_mysqlx_row_result>(object);
}

static zend_object *
php_mysqlx_row_result_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_row_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_row_result>(
		class_type,
		&mysqlx_object_row_result_handlers,
		&mysqlx_row_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_row_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_row_result_class_entry,
		"RowResult",
		mysqlx_std_object_handlers,
		mysqlx_object_row_result_handlers,
		php_mysqlx_row_result_object_allocator,
		mysqlx_row_result_free_storage,
		mysqlx_row_result_methods,
		mysqlx_row_result_properties,
		mysqlx_row_result_property_entries,
		mysqlx_base_result_interface_entry);

	mysqlx_register_row_result_iterator(mysqlx_row_result_class_entry);
}

void
mysqlx_unregister_row_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_row_result_properties);
}

util::zvalue
create_row_result(XMYSQLND_STMT_RESULT* result)
{
	DBG_ENTER("create_row_result");

	util::zvalue row_result_obj;
	st_mysqlx_row_result& data_object{
		util::init_object<st_mysqlx_row_result>(mysqlx_row_result_class_entry, row_result_obj) };
	data_object.result = result;

	DBG_RETURN(row_result_obj);
}

} // namespace devapi

} // namespace mysqlx
