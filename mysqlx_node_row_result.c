/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#include <php.h>
#undef ERROR
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result.h>
#include <xmysqlnd/xmysqlnd_node_stmt_result_meta.h>
#include <xmysqlnd/xmysqlnd_rowset.h>
#include <xmysqlnd/xmysqlnd_rowset_buffered.h>
#include <xmysqlnd/xmysqlnd_rowset_fwd.h>
#include <xmysqlnd/xmysqlnd_warning_list.h>
#include <xmysqlnd/xmysqlnd_stmt_execution_state.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "mysqlx_node_row_result_iterator.h"
#include "mysqlx_node_row_result.h"
#include "mysqlx_node_base_result.h"
#include "mysqlx_field_metadata.h"
#include "mysqlx_node_column_result.h"

static zend_class_entry *mysqlx_node_row_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__fetch_one, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__fetch_all, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__get_warning_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__get_column_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__get_column_names, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_row_result__get_columns, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

#define MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_row_result *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

/*
 * Handy macro used to raise exceptions
 */
#define RAISE_EXCEPTION(errcode, msg) \
	static const MYSQLND_CSTRING sqlstate = { "HY000", sizeof("HY000") - 1 }; \
	static const MYSQLND_CSTRING errmsg = { msg, sizeof(msg) - 1 }; \
	mysqlx_new_exception(errcode, sqlstate, errmsg); \


/* {{{ mysqlx_node_row_result::__construct */
static
PHP_METHOD(mysqlx_node_row_result, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_row_result::fetchOne(object result) */
static
PHP_METHOD(mysqlx_node_row_result, fetchOne)
{
	zval * object_zv;
	struct st_mysqlx_node_row_result * object;

	DBG_ENTER("mysqlx_node_row_result::fetchOne");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object && object->result && FALSE == object->result->m.eof(object->result)) {
		zval row;
		ZVAL_UNDEF(&row);
		if (PASS == object->result->m.fetch_current(object->result, &row, NULL, NULL)) {
			ZVAL_COPY_VALUE(return_value, &row);
			object->result->m.next(object->result, NULL, NULL);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_row_result::fetchAll(object result) */
static
PHP_METHOD(mysqlx_node_row_result, fetchAll)
{
	zval * object_zv;
	struct st_mysqlx_node_row_result * object;

	DBG_ENTER("mysqlx_node_row_result::fetchAll");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object && object->result) {
		zval set;
		ZVAL_UNDEF(&set);
		if (PASS == object->result->m.fetch_all(object->result, &set, NULL, NULL)) {
			ZVAL_COPY_VALUE(return_value, &set);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_row_result::getWarningCount(object result) */
static
PHP_METHOD(mysqlx_node_row_result, getWarningCount)
{
	zval * object_zv;
	struct st_mysqlx_node_row_result * object;

	DBG_ENTER("mysqlx_node_row_result::getWarningCount");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_WARNING_LIST * const warnings = object->result->warnings;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (warnings) {
			const size_t value = warnings->m->count(warnings);
			if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
				ZVAL_NEW_STR(return_value, strpprintf(0, MYSQLND_LLU_SPEC, value));
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


/* {{{ proto mixed mysqlx_node_row_result::getWarnings(object result) */
static
PHP_METHOD(mysqlx_node_row_result, getWarnings)
{
	zval * object_zv;
	struct st_mysqlx_node_row_result * object;

	DBG_ENTER("mysqlx_node_row_result::getWarnings");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_row_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_WARNING_LIST * const warnings = object->result->warnings;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (warnings) {
			const size_t count = warnings->m->count(warnings);
			unsigned int i = 0;
			array_init_size(return_value, count);
			for (; i < count; ++i) {
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
static struct st_xmysqlnd_node_stmt_result_meta*
get_stmt_result_meta(struct st_xmysqlnd_node_stmt_result* stmt_result)
{
	struct st_xmysqlnd_node_stmt_result_meta* meta = 0;
	if (stmt_result && stmt_result->meta)
	{
		meta = stmt_result->meta;
	}
	return meta;
}
/* }}} */


/* {{{ get_node_stmt_result_meta */
static struct st_xmysqlnd_node_stmt_result_meta*
get_node_stmt_result_meta(INTERNAL_FUNCTION_PARAMETERS)
{
	struct st_xmysqlnd_node_stmt_result_meta* meta = NULL;
	zval * object_zv;
	struct st_mysqlx_node_row_result * object;

	DBG_ENTER("get_node_stmt_result_meta");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_row_result_class_entry))
	{
		DBG_RETURN(NULL);
	}

	MYSQLX_FETCH_NODE_ROW_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		meta = get_stmt_result_meta(object->result);
	}

	if(meta == NULL) {
		RAISE_EXCEPTION(10001,"get_node_stmt_result_meta: Unable to extract metadata");
	}

	DBG_RETURN(meta);
	return meta;
}


/* {{{ proto mixed mysqlx_node_row_result::getColumnCount(object result) */
static
PHP_METHOD(mysqlx_node_row_result, getColumnCount)
{
	struct st_xmysqlnd_node_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_node_row_result::getColumnCount");
	meta = get_node_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

	if (meta) {
		const size_t value = meta->m->get_field_count(meta);
		if (UNEXPECTED(value >= ZEND_LONG_MAX)) {
			ZVAL_NEW_STR(return_value, strpprintf(0, MYSQLND_LLU_SPEC, value));
			DBG_INF_FMT("value(S)=%s", Z_STRVAL_P(return_value));
		} else {
			ZVAL_LONG(return_value, value);
			DBG_INF_FMT("value(L)=%lu", Z_LVAL_P(return_value));
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_row_result::getColumns(object result) */
static
PHP_METHOD(mysqlx_node_row_result, getColumns)
{
	struct st_xmysqlnd_node_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_node_row_result::getColumns");
	meta = get_node_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

	if (meta) {
		const size_t count = meta->m->get_field_count(meta);
		unsigned int i = 0;
		array_init_size(return_value, count);
		for (; i < count; ++i) {
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


/* {{{ proto mixed mysqlx_node_row_result::getColumnNames(object result) */
static
PHP_METHOD(mysqlx_node_row_result, getColumnNames)
{
	struct st_xmysqlnd_node_stmt_result_meta* meta;
	DBG_ENTER("mysqlx_node_row_result::getColumnNames");
	meta = get_node_stmt_result_meta(INTERNAL_FUNCTION_PARAM_PASSTHRU);

	if (meta) {
		const size_t count = meta->m->get_field_count(meta);
		unsigned int i = 0;
		array_init_size(return_value, count);
		for (; i < count; ++i) {
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


/* {{{ mysqlx_node_row_result_methods[] */
static const zend_function_entry mysqlx_node_row_result_methods[] = {
	PHP_ME(mysqlx_node_row_result, __construct,			NULL,																ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_row_result, fetchOne,				arginfo_mysqlx_node_row_result__fetch_one,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_row_result, fetchAll,				arginfo_mysqlx_node_row_result__fetch_all,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_row_result, getWarningCount,		arginfo_mysqlx_node_row_result__get_warning_count,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_row_result, getWarnings,			arginfo_mysqlx_node_row_result__get_warnings, 			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_row_result, getColumnCount,		arginfo_mysqlx_node_row_result__get_column_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_row_result, getColumnNames,		arginfo_mysqlx_node_row_result__get_column_names,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_row_result, getColumns,			arginfo_mysqlx_node_row_result__get_columns,	 			ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_row_result_handlers;
static HashTable mysqlx_node_row_result_properties;

const struct st_mysqlx_property_entry mysqlx_node_row_result_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_row_result_free_storage */
static void
mysqlx_node_row_result_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_row_result * inner_obj = (struct st_mysqlx_node_row_result *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_node_stmt_result_free(inner_obj->result, NULL, NULL);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_row_result_object_allocator */
static zend_object *
php_mysqlx_node_row_result_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_row_result * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_row_result));

	DBG_ENTER("php_mysqlx_node_row_result_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_row_result_handlers;
	mysqlx_object->properties = &mysqlx_node_row_result_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_row_result_class */
void
mysqlx_register_node_row_result_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_row_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_row_result_handlers.free_obj = mysqlx_node_row_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeRowResult", mysqlx_node_row_result_methods);
		tmp_ce.create_object = php_mysqlx_node_row_result_object_allocator;

		mysqlx_node_row_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_row_result_class_entry, 1, mysqlx_node_base_result_interface_entry);

		mysqlx_register_node_row_result_iterator(mysqlx_node_row_result_class_entry);
	}

	zend_hash_init(&mysqlx_node_row_result_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_row_result_properties, mysqlx_node_row_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_row_result_class */
void
mysqlx_unregister_node_row_result_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_row_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_row_result */
void
mysqlx_new_row_result(zval * return_value, XMYSQLND_NODE_STMT_RESULT * result)
{
	DBG_ENTER("mysqlx_new_row_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_row_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_row_result * const object = (struct st_mysqlx_node_row_result *) mysqlx_object->ptr;
		if (object) {
			object->result = result;
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
