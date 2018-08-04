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
#include "mysqlx_row_result_iterator.h"
#include "mysqlx_row_result.h"
#include "mysqlx_base_result.h"
#include "mysqlx_field_metadata.h"
#include "mysqlx_column_result.h"
#include "mysqlx_exception.h"

#include "util/allocator.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_row_result_class_entry;

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

#define MYSQLX_FETCH_ROW_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_row_result*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_row_result::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/* {{{ proto mixed mysqlx_row_result::fetchOne(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, fetchOne)
{
	zval* object_zv{nullptr};
	st_mysqlx_row_result* object{nullptr};

	DBG_ENTER("mysqlx_row_result::fetchOne");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

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


/* {{{ proto mixed mysqlx_row_result::fetchAll(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, fetchAll)
{
	zval* object_zv{nullptr};
	st_mysqlx_row_result* object{nullptr};

	DBG_ENTER("mysqlx_row_result::fetchAll");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
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


/* {{{ proto mixed mysqlx_row_result::getWarningsCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getWarningsCount)
{
	zval* object_zv{nullptr};
	st_mysqlx_row_result* object{nullptr};

	DBG_ENTER("mysqlx_row_result::getWarningsCount");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_ROW_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ proto mixed mysqlx_row_result::getWarnings(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getWarnings)
{
	zval* object_zv{nullptr};
	st_mysqlx_row_result* object{nullptr};

	DBG_ENTER("mysqlx_row_result::getWarnings");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_ROW_RESULT_FROM_ZVAL(object, object_zv);

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
	if (stmt_result && stmt_result->meta)
	{
		meta = stmt_result->meta;
	}
	return meta;
}
/* }}} */


/* {{{ get_stmt_result_meta */
static st_xmysqlnd_stmt_result_meta* get_stmt_result_meta(INTERNAL_FUNCTION_PARAMETERS)
{
	st_xmysqlnd_stmt_result_meta* meta{nullptr};
	zval* object_zv{nullptr};
	st_mysqlx_row_result* object{nullptr};

	DBG_ENTER("get_stmt_result_meta");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_row_result_class_entry))
	{
		DBG_RETURN(nullptr);
	}

	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(object_zv);
	object = (st_mysqlx_row_result*) mysqlx_object->ptr;
	if (!object) {
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		DBG_RETURN(nullptr);
	}

	RETVAL_FALSE;
	if (object->result) {
		meta = get_stmt_result_meta(object->result);
	}

	if(meta == nullptr) {
		RAISE_EXCEPTION(10001,"get_stmt_result_meta: Unable to extract metadata");
	}

	DBG_RETURN(meta);
}


/* {{{ proto mixed mysqlx_row_result::getColumnCount(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumnCount)
{
	st_xmysqlnd_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_row_result::getColumnCount");
	meta = get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

	if (meta) {
		const size_t value = meta->m->get_field_count(meta);
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
/* }}} */


/* {{{ proto mixed mysqlx_row_result::getColumns(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumns)
{
	st_xmysqlnd_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_row_result::getColumns");
	meta = get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

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
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_row_result::getColumnNames(object result) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_row_result, getColumnNames)
{
	st_xmysqlnd_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_row_result::getColumnNames");
	meta = get_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

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
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_row_result_methods[] */
static const zend_function_entry mysqlx_row_result_methods[] = {
	PHP_ME(mysqlx_row_result, __construct,			nullptr,																ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_row_result, fetchOne,				arginfo_mysqlx_row_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, fetchAll,				arginfo_mysqlx_row_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_row_result, getWarningsCount,		arginfo_mysqlx_row_result__get_warnings_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getWarnings,			arginfo_mysqlx_row_result__get_warnings, 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_row_result, getColumnCount,		arginfo_mysqlx_row_result__get_column_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getColumnNames,		arginfo_mysqlx_row_result__get_column_names,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_row_result, getColumns,			arginfo_mysqlx_row_result__get_columns,	 			ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_row_result_handlers;
static HashTable mysqlx_row_result_properties;

const struct st_mysqlx_property_entry mysqlx_row_result_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_row_result_free_storage */
static void
mysqlx_row_result_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_row_result* inner_obj = (st_mysqlx_row_result*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_stmt_result_free(inner_obj->result, nullptr, nullptr);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_row_result_object_allocator */
static zend_object *
php_mysqlx_row_result_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_row_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_row_result>(
		class_type,
		&mysqlx_object_row_result_handlers,
		&mysqlx_row_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_row_result_class */
void
mysqlx_register_row_result_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_row_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_row_result_handlers.free_obj = mysqlx_row_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "RowResult", mysqlx_row_result_methods);
		tmp_ce.create_object = php_mysqlx_row_result_object_allocator;

		mysqlx_row_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_row_result_class_entry, 1, mysqlx_base_result_interface_entry);

		mysqlx_register_row_result_iterator(mysqlx_row_result_class_entry);
	}

	zend_hash_init(&mysqlx_row_result_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_row_result_properties, mysqlx_row_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_row_result_class */
void
mysqlx_unregister_row_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_row_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_row_result */
void
mysqlx_new_row_result(zval * return_value, XMYSQLND_STMT_RESULT * result)
{
	DBG_ENTER("mysqlx_new_row_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_row_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_row_result* const object = (st_mysqlx_row_result*) mysqlx_object->ptr;
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
