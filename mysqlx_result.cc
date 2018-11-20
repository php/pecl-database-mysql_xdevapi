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

#define MYSQLX_FETCH_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_result*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_result::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/* {{{ proto mixed mysqlx_result::getAffectedItemsCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getAffectedItemsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_result* object{nullptr};

	DBG_ENTER("mysqlx_result::getAffectedItemsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ proto mixed mysqlx_result::getAutoIncrementValue(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getAutoIncrementValue)
{
	zval* object_zv{nullptr};
	st_mysqlx_result* object{nullptr};

	DBG_ENTER("mysqlx_result::getAutoIncrementValue");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->result->exec_state) {
		const XMYSQLND_STMT_EXECUTION_STATE * const exec_state = object->result->exec_state;
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
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_result::getGeneratedIds(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getGeneratedIds)
{
    DBG_ENTER("mysqlx_result::getGeneratedIds");
	zval* object_zv{nullptr};
	st_mysqlx_result* object{nullptr};

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_RESULT_FROM_ZVAL(object, object_zv);
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


/* {{{ proto mixed mysqlx_result::getWarningsCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getWarningsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_result* object{nullptr};

	DBG_ENTER("mysqlx_result::getWarningsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ proto mixed mysqlx_result::getWarnings(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_result, getWarnings)
{
	zval* object_zv{nullptr};
	st_mysqlx_result* object{nullptr};

	DBG_ENTER("mysqlx_result::getWarnings");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ mysqlx_result_methods[] */
static const zend_function_entry mysqlx_result_methods[] = {
	PHP_ME(mysqlx_result, __construct,			nullptr,														ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_result, getAffectedItemsCount,	arginfo_mysqlx_result__get_affected_items_count,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getAutoIncrementValue, 	arginfo_mysqlx_result__get_auto_increment_value,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getGeneratedIds,			arginfo_mysqlx_result__get_generated_ids,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getWarningsCount,			arginfo_mysqlx_result__get_warnings_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_result, getWarnings,				arginfo_mysqlx_result__get_warnings, 				ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_result_handlers;
static HashTable mysqlx_result_properties;

const struct st_mysqlx_property_entry mysqlx_result_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_result_free_storage */
static void
mysqlx_result_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_result* inner_obj = (st_mysqlx_result*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_stmt_result_free(inner_obj->result, nullptr, nullptr);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_result_object_allocator */
static zend_object *
php_mysqlx_result_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_result>(
		class_type,
		&mysqlx_object_result_handlers,
		&mysqlx_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_result_class */
void
mysqlx_register_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_result_handlers.free_obj = mysqlx_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Result", mysqlx_result_methods);
		tmp_ce.create_object = php_mysqlx_result_object_allocator;

		mysqlx_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_result_class_entry, 1, mysqlx_base_result_interface_entry);

		mysqlx_register_result_iterator(mysqlx_result_class_entry);
	}

	zend_hash_init(&mysqlx_result_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_result_properties, mysqlx_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_result_class */
void
mysqlx_unregister_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_result */
void
mysqlx_new_result(zval * return_value, XMYSQLND_STMT_RESULT * result)
{
	DBG_ENTER("mysqlx_new_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_result* const object = (st_mysqlx_result*) mysqlx_object->ptr;
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
