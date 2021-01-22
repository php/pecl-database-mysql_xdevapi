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
#include "xmysqlnd/xmysqlnd_utils.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_doc_result_iterator.h"
#include "mysqlx_doc_result.h"
#include "mysqlx_base_result.h"
#include "util/allocator.h"
#include "util/arguments.h"
#include "util/functions.h"
#include "util/object.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_doc_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__get_warnings_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

st_mysqlx_doc_result::~st_mysqlx_doc_result()
{
	xmysqlnd_stmt_result_free(result, nullptr, nullptr);
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, fetchOne)
{
	DBG_ENTER("mysqlx_doc_result::fetchOne");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_NULL();

	auto& data_object{ util::fetch_data_object<st_mysqlx_doc_result>(object_zv) };
	drv::st_xmysqlnd_stmt_result* result{ data_object.result};
	if (FALSE == data_object.result->m.eof(result)) {
		util::zvalue row;
		if (PASS == data_object.result->m.fetch_current(result, row.ptr(), nullptr, nullptr)) {
			xmysqlnd_utils_decode_doc_row(row).move_to(return_value);
			data_object.result->m.next(result, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, fetchAll)
{

	DBG_ENTER("mysqlx_doc_result::fetchAll");

	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_doc_result>(object_zv) };
	if (data_object.result) {
		util::zvalue set;
		if (PASS == data_object.result->m.fetch_all(data_object.result, set.ptr(), nullptr, nullptr)) {
			xmysqlnd_utils_decode_doc_rows(set).move_to(return_value);
		}
	}
	util::zvalue::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, getWarningsCount)
{
	DBG_ENTER("mysqlx_doc_result::getWarningsCount");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	RETVAL_LONG(0);

	auto& data_object{ util::fetch_data_object<st_mysqlx_doc_result>(object_zv) };
	std::size_t warning_count{ 0 };
	const XMYSQLND_WARNING_LIST* const warnings = data_object.result->warnings;
	/* Maybe check here if there was an error and throw an Exception or return a warning */
	if (warnings) {
		warning_count = warnings->count();
	}
	if (UNEXPECTED(warning_count >= ZEND_LONG_MAX)) {
		ZVAL_NEW_STR(return_value, strpprintf(0, "%s", util::to_string(warning_count).c_str()));
		DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
	} else {
		RETVAL_LONG(warning_count);
		DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
	}
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, getWarnings)
{
	DBG_ENTER("mysqlx_doc_result::getWarnings");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_doc_result>(object_zv) };

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

static const zend_function_entry mysqlx_doc_result_methods[] = {
	PHP_ME(mysqlx_doc_result, __construct, arginfo_mysqlx_doc_result__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_doc_result, fetchOne,				arginfo_mysqlx_doc_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_doc_result, fetchAll,				arginfo_mysqlx_doc_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_doc_result, getWarningsCount,		arginfo_mysqlx_doc_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_doc_result, getWarnings,			arginfo_mysqlx_doc_result__get_warnings, 			ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_doc_result_handlers;
static HashTable mysqlx_doc_result_properties;

const st_mysqlx_property_entry mysqlx_doc_result_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_doc_result_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_doc_result>(object);
}

static zend_object *
php_mysqlx_doc_result_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_doc_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_doc_result>(
		class_type,
		&mysqlx_object_doc_result_handlers,
		&mysqlx_doc_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_doc_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_doc_result_class_entry,
		"DocResult",
		mysqlx_std_object_handlers,
		mysqlx_object_doc_result_handlers,
		php_mysqlx_doc_result_object_allocator,
		mysqlx_doc_result_free_storage,
		mysqlx_doc_result_methods,
		mysqlx_doc_result_properties,
		mysqlx_doc_result_property_entries,
		mysqlx_base_result_interface_entry);

	mysqlx_register_doc_result_iterator(mysqlx_doc_result_class_entry);
}

void
mysqlx_unregister_doc_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_doc_result_properties);
}

util::zvalue
create_doc_result(XMYSQLND_STMT_RESULT* result)
{
	DBG_ENTER("create_doc_result");

	util::zvalue new_doc_obj;
	st_mysqlx_doc_result& data_object{
		util::init_object<st_mysqlx_doc_result>(mysqlx_doc_result_class_entry, new_doc_obj) };
	data_object.result = result;

	DBG_RETURN(new_doc_obj);
}

util::zvalue fetch_one_from_doc_result(const util::zvalue& resultset) {
	DBG_ENTER("fetch_one_from_doc_result");

	assert(resultset.is_object());

	util::zvalue row;

	st_mysqlx_doc_result& doc_result = util::fetch_data_object<st_mysqlx_doc_result>(resultset);
	if (TRUE == doc_result.result->m.eof(doc_result.result)) {
		DBG_RETURN(row);
	}

	util::zvalue encoded_row;
	if (PASS == doc_result.result->m.fetch_current(doc_result.result, encoded_row.ptr(), nullptr, nullptr)) {
		row = xmysqlnd_utils_decode_doc_row(encoded_row);
		doc_result.result->m.next(doc_result.result, nullptr, nullptr);
	}

	DBG_RETURN(row);
}

} // namespace devapi

} // namespace mysqlx
