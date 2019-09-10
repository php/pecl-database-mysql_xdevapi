/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_doc_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__get_warnings_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_doc_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

#define MYSQLX_FETCH_DOC_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_doc_result*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, fetchOne)
{
	zval* object_zv{nullptr};
	st_mysqlx_doc_result* object{nullptr};

	RETVAL_NULL();

	DBG_ENTER("mysqlx_doc_result::fetchOne");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_DOC_RESULT_FROM_ZVAL(object, object_zv);

	if (object && object->result && FALSE == object->result->m.eof(object->result)) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == object->result->m.fetch_current(object->result, &row, nullptr, nullptr)) {
			xmysqlnd_utils_decode_doc_row(&row, return_value);
			//ZVAL_COPY_VALUE(return_value, &row);
			zval_ptr_dtor(&row);
			object->result->m.next(object->result, nullptr, nullptr);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, fetchAll)
{

	DBG_ENTER("mysqlx_doc_result::fetchAll");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object{ util::fetch_data_object<st_mysqlx_doc_result>(object_zv) };
	if (data_object.result) {
		zval set;
		ZVAL_UNDEF(&set);
		if (PASS == data_object.result->m.fetch_all(data_object.result, &set, nullptr, nullptr)) {
			xmysqlnd_utils_decode_doc_rows(&set, return_value);
			//ZVAL_COPY_VALUE(return_value, &set);
			zval_ptr_dtor(&set);
		}
	}
	util::zend::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, getWarningsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_doc_result* object{nullptr};

	DBG_ENTER("mysqlx_doc_result::getWarningsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_DOC_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_LONG(0);
	size_t warning_count{ 0 };
	if (object->result) {
		const XMYSQLND_WARNING_LIST * const warnings = object->result->warnings;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (warnings) {
			warning_count = warnings->m->count(warnings);
		}
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
/* }}} */


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_doc_result, getWarnings)
{
	zval* object_zv{nullptr};
	st_mysqlx_doc_result* object{nullptr};

	DBG_ENTER("mysqlx_doc_result::getWarnings");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_doc_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_DOC_RESULT_FROM_ZVAL(object, object_zv);

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

	util::zend::ensure_is_array(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


static const zend_function_entry mysqlx_doc_result_methods[] = {
	PHP_ME(mysqlx_doc_result, __construct,			nullptr,																ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_doc_result, fetchOne,				arginfo_mysqlx_doc_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_doc_result, fetchAll,				arginfo_mysqlx_doc_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_doc_result, getWarningsCount,		arginfo_mysqlx_doc_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_doc_result, getWarnings,			arginfo_mysqlx_doc_result__get_warnings, 			ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_doc_result_handlers;
static HashTable mysqlx_doc_result_properties;

const struct st_mysqlx_property_entry mysqlx_doc_result_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

static void
mysqlx_doc_result_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_doc_result* inner_obj = (st_mysqlx_doc_result*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_stmt_result_free(inner_obj->result, nullptr, nullptr);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


static zend_object *
php_mysqlx_doc_result_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_doc_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_doc_result>(
		class_type,
		&mysqlx_object_doc_result_handlers,
		&mysqlx_doc_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


void
mysqlx_register_doc_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_doc_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_doc_result_handlers.free_obj = mysqlx_doc_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "DocResult", mysqlx_doc_result_methods);
		tmp_ce.create_object = php_mysqlx_doc_result_object_allocator;

		mysqlx_doc_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_doc_result_class_entry, 1, mysqlx_base_result_interface_entry);

		mysqlx_register_doc_result_iterator(mysqlx_doc_result_class_entry);
	}

	zend_hash_init(&mysqlx_doc_result_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_doc_result_properties, mysqlx_doc_result_property_entries);
}
/* }}} */


void
mysqlx_unregister_doc_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_doc_result_properties);
}
/* }}} */


void
mysqlx_new_doc_result(zval * return_value, XMYSQLND_STMT_RESULT * result)
{
	DBG_ENTER("mysqlx_new_doc_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_doc_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_doc_result* const object = (st_mysqlx_doc_result*) mysqlx_object->ptr;
		if (object) {
			object->result = result;
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


void fetch_one_from_doc_result(zval* return_value) {
	DBG_ENTER("fetch_one_from_doc_result");

	if (Z_TYPE_P(return_value) != IS_OBJECT) {
		RETVAL_NULL();
		DBG_VOID_RETURN;
	}

	st_mysqlx_doc_result& doc_result = util::fetch_data_object<st_mysqlx_doc_result>(return_value);

	if (TRUE == doc_result.result->m.eof(doc_result.result)) {
		RETVAL_NULL();
		DBG_VOID_RETURN;
	}

	zval row;
	ZVAL_UNDEF(&row);
	if (PASS == doc_result.result->m.fetch_current(doc_result.result, &row, nullptr, nullptr)) {
		xmysqlnd_utils_decode_doc_row(&row, return_value);
		zval_ptr_dtor(&row);
		doc_result.result->m.next(doc_result.result, nullptr, nullptr);
	} else {
		RETVAL_NULL();
	}

	DBG_VOID_RETURN;
}
/* }}} */

} // namespace devapi

} // namespace mysqlx
