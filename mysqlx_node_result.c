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
#include "mysqlx_node_result_iterator.h"
#include "mysqlx_node_result.h"
#include "mysqlx_node_base_result.h"
#include "mysqlx_field_metadata.h"

static zend_class_entry *mysqlx_node_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_result__get_affected_items_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_result__get_last_insert_id, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_result__get_last_document_id, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_result__get_warning_count, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_result__get_warnings, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

#define MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_result *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_result::__construct */
static
PHP_METHOD(mysqlx_node_result, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_result::getAffectedItemsCount(object result) */
static
PHP_METHOD(mysqlx_node_result, getAffectedItemsCount)
{
	zval * object_zv;
	struct st_mysqlx_node_result * object;

	DBG_ENTER("mysqlx_node_result::getAffectedItemsCount");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result) {
		const XMYSQLND_STMT_EXECUTION_STATE * exec_state = object->result->exec_state;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (exec_state) {
			const size_t value = exec_state->m->get_affected_items_count(exec_state);
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


/* {{{ proto mixed mysqlx_node_result::getLastInsertId(object result) */
static
PHP_METHOD(mysqlx_node_result, getLastInsertId)
{
	zval * object_zv;
	struct st_mysqlx_node_result * object;

	DBG_ENTER("mysqlx_node_result::getLastInsertId");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->result->exec_state) {
		const XMYSQLND_STMT_EXECUTION_STATE * const exec_state = object->result->exec_state;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (exec_state) {
			const size_t value = exec_state->m->get_last_insert_id(exec_state);
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


/* {{{ proto mixed mysqlx_node_result::getLastDocumentId(object result) */
static
PHP_METHOD(mysqlx_node_result, getLastDocumentId)
{
	zval * object_zv;
	struct st_mysqlx_node_result * object;

	DBG_ENTER("mysqlx_node_result::getLastDocumentId");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;
	if (object->result && object->result->exec_state) {
		const XMYSQLND_STMT_EXECUTION_STATE * const exec_state = object->result->exec_state;
		/* Maybe check here if there was an error and throw an Exception or return a warning */
		if (exec_state) {
			MYSQLND_CSTRING value = exec_state->m->get_last_document_id(exec_state);
			ZVAL_STRINGL(return_value, value.s, value.l);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_result::getWarningCount(object result) */
static
PHP_METHOD(mysqlx_node_result, getWarningCount)
{
	zval * object_zv;
	struct st_mysqlx_node_result * object;

	DBG_ENTER("mysqlx_node_result::getWarningCount");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ proto mixed mysqlx_node_result::getWarnings(object result) */
static
PHP_METHOD(mysqlx_node_result, getWarnings)
{
	zval * object_zv;
	struct st_mysqlx_node_result * object;

	DBG_ENTER("mysqlx_node_result::getWarnings");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_result_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_RESULT_FROM_ZVAL(object, object_zv);

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


/* {{{ mysqlx_node_result_methods[] */
static const zend_function_entry mysqlx_node_result_methods[] = {
	PHP_ME(mysqlx_node_result, __construct,			NULL,																ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_result, getAffectedItemsCount,	arginfo_mysqlx_node_result__get_affected_items_count,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_result, getLastInsertId, 		arginfo_mysqlx_node_result__get_last_insert_id,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_result, getLastDocumentId, 		arginfo_mysqlx_node_result__get_last_document_id,		ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_result, getWarningCount,			arginfo_mysqlx_node_result__get_warning_count,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_result, getWarnings,				arginfo_mysqlx_node_result__get_warnings, 				ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_result_handlers;
static HashTable mysqlx_node_result_properties;

const struct st_mysqlx_property_entry mysqlx_node_result_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_result_free_storage */
static void
mysqlx_node_result_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_result * inner_obj = (struct st_mysqlx_node_result *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->result) {
			xmysqlnd_node_stmt_result_free(inner_obj->result, NULL, NULL);
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_result_object_allocator */
static zend_object *
php_mysqlx_node_result_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_result * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_result));

	DBG_ENTER("php_mysqlx_node_result_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_result_handlers;
	mysqlx_object->properties = &mysqlx_node_result_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_result_class */
void
mysqlx_register_node_result_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_result_handlers.free_obj = mysqlx_node_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeResult", mysqlx_node_result_methods);
		tmp_ce.create_object = php_mysqlx_node_result_object_allocator;

		mysqlx_node_result_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_result_class_entry, 1, mysqlx_node_base_result_interface_entry);

		mysqlx_register_node_result_iterator(mysqlx_node_result_class_entry);
	}

	zend_hash_init(&mysqlx_node_result_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_result_properties, mysqlx_node_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_result_class */
void
mysqlx_unregister_node_result_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_result_properties);
}
/* }}} */


/* {{{ mysqlx_new_result */
void
mysqlx_new_result(zval * return_value, XMYSQLND_NODE_STMT_RESULT * result)
{
	DBG_ENTER("mysqlx_new_result");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_result_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_result * const object = (struct st_mysqlx_node_result *) mysqlx_object->ptr;
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
