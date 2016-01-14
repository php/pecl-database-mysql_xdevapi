/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_connection.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "ext/mysqlnd/mysqlnd_wireprotocol.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h" // XMYSQLND_INC_SESSION_STATISTIC_W_VALUE3
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_stmt_result_meta.h"


/* {{{ xmysqlnd_result_field_meta::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, init)(XMYSQLND_RESULT_FIELD_META * const field, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_type */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_type)(XMYSQLND_RESULT_FIELD_META * const field, enum xmysqlnd_field_type type)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_type");
	field->type = type;
	field->type_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_set_mysqlnd_string */
static inline enum_func_status
xmysqlnd_set_mysqlnd_string(MYSQLND_STRING * str, const char * const value, const size_t value_len, const zend_bool persistent MYSQLND_MEM_D)
{
	if (value) {
		str->s = value_len? mnd_pestrndup(value, value_len, persistent) : (char *) mysqlnd_empty_string;
		str->l = value_len;
		return str->s? PASS:FAIL;
	}
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_name */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	zend_ulong idx;

	DBG_ENTER("xmysqlnd_result_field_meta::set_name");
	if (len) {
		field->zend_hash_key.sname = zend_string_init(str, len, field->persistent);
		field->name.s = ZSTR_VAL(field->zend_hash_key.sname);
	} else {
		field->zend_hash_key.sname = ZSTR_EMPTY_ALLOC();
		field->name.s = (char*) mysqlnd_empty_string;
	}
	field->name.l = len;

	if (field->zend_hash_key.is_numeric == ZEND_HANDLE_NUMERIC(field->zend_hash_key.sname, idx)) {
		field->zend_hash_key.key = idx;
	}

	DBG_RETURN(field->name.s? PASS:FAIL);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_original_name */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_original_name");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->original_name, str, len, field->persistent MYSQLND_MEM_C));
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_table */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_table");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->table, str, len, field->persistent MYSQLND_MEM_C));
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_original_table */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_original_table");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->original_table, str, len, field->persistent MYSQLND_MEM_C));
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_schema */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_schema)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_schema");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->schema, str, len, field->persistent MYSQLND_MEM_C));
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_catalog */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_catalog)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_catalog");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->catalog, str, len, field->persistent MYSQLND_MEM_C));
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_collation */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_collation)(XMYSQLND_RESULT_FIELD_META * const field, const uint64_t collation)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_collation");
	field->collation = collation;
	field->collation_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_fractional_digits */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_fractional_digits)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t digits)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_fractional_digits");
	field->fractional_digits = digits;
	field->fractional_digits_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_length */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_length)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t length)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_length");
	field->length = length;
	field->length_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_flags */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_flags)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t flags)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_flags");
	field->flags = flags;
	field->flags_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::set_content_type */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_content_type)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t content_type)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_content_type");
	field->content_type = content_type;
	field->content_type_set = TRUE;
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_result_field_meta, free_contents)(XMYSQLND_RESULT_FIELD_META * const field)
{
	const zend_bool persistent = field->persistent;
	DBG_ENTER("xmysqlnd_result_field_meta::free_contents");

	/* Don't free field->name.s as it is a pointer to field->zend_hash_key.sname */
	field->name.s = NULL;
	field->name.l = 0;

	if (field->original_name.s && field->original_name.s != mysqlnd_empty_string) {
		mnd_pefree(field->original_name.s, persistent);
		field->original_name.s = NULL;
		field->original_name.l = 0;
	}
	if (field->table.s && field->table.s != mysqlnd_empty_string) {
		mnd_pefree(field->table.s, persistent);
		field->table.s = NULL;
		field->table.l = 0;
	}
	if (field->original_table.s && field->original_table.s != mysqlnd_empty_string) {
		mnd_pefree(field->original_table.s, persistent);
		field->original_table.s = NULL;
		field->original_table.l = 0;
	}
	if (field->schema.s && field->schema.s != mysqlnd_empty_string) {
		mnd_pefree(field->schema.s, persistent);
		field->schema.s = NULL;
		field->schema.l = 0;
	}
	if (field->catalog.s && field->catalog.s != mysqlnd_empty_string) {
		mnd_pefree(field->catalog.s, persistent);
		field->catalog.s = NULL;
		field->catalog.l = 0;
	}
	if (field->zend_hash_key.sname) {
		zend_string_release(field->zend_hash_key.sname);
		field->zend_hash_key.sname = NULL;
	}
	field->zend_hash_key.is_numeric = FALSE;
	field->zend_hash_key.key = 0;

	field->collation_set =
		field->fractional_digits_set =
			field->length_set =
				field->flags_set =
					field->content_type_set = FALSE;
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ xmysqlnd_result_field_meta::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_result_field_meta, dtor)(XMYSQLND_RESULT_FIELD_META * const field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_result_field_meta::dtor");
	if (field) {
		field->m->free_contents(field);
		mnd_pefree(field, field->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_result_field_meta)
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, init),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_type),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_name),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_name),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_table),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_table),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_schema),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_catalog),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_collation),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_fractional_digits),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_length),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_flags),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_content_type),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, free_contents),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, dtor),
MYSQLND_CLASS_METHODS_END;


PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_result_field_meta);

/* {{{ xmysqlnd_result_field_meta_create */
PHPAPI XMYSQLND_RESULT_FIELD_META *
xmysqlnd_result_field_meta_create(const zend_bool persistent,
								  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								  MYSQLND_STATS * stats,
								  MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_RESULT_FIELD_META * object = NULL;
	DBG_ENTER("xmysqlnd_result_field_meta_create");
	object = object_factory->get_result_field_meta(object_factory, persistent, stats, error_info);
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_result_field_meta_free */
PHPAPI void
xmysqlnd_result_field_meta_free(XMYSQLND_RESULT_FIELD_META * const object, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_result_field_meta_free");
	if (object) {
		object->m->dtor(object, stats, error_info);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/*******************************************************************************************************************************************/


/* {{{ xmysqlnd_node_stmt_result_meta::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, init)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info)
{
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta::add_field */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, add_field)(XMYSQLND_NODE_STMT_RESULT_META * const meta, XMYSQLND_RESULT_FIELD_META * field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_meta::add_field");
	if (!meta->fields || meta->field_count == meta->fields_size) {
		meta->fields_size += 8;
		meta->fields = mnd_perealloc(meta->fields, meta->fields_size * sizeof(field), meta->persistent);
		if (!meta->fields) {
			SET_OOM_ERROR(error_info);
			DBG_RETURN(FAIL);
		}
	}
	meta->fields[meta->field_count++] = field;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta::count */
static unsigned int
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, count)(const XMYSQLND_NODE_STMT_RESULT_META * const meta)
{
	return (meta->field_count);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta::get_field */
static const XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, get_field)(const XMYSQLND_NODE_STMT_RESULT_META * const meta, unsigned int field)
{
	return((meta->field_count > 0 && field < meta->field_count)? meta->fields[field] : NULL);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, free_contents)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_meta::free_contents");
	if (meta->fields) {
		unsigned int i = 0;
		for (; i < meta->field_count; ++i) {
			meta->fields[i]->m->dtor(meta->fields[i], stats, error_info);
		}
		mnd_pefree(meta->fields, meta->persistent);
		meta->fields = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, dtor)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_meta::dtor");
	if (meta) {
		meta->m->free_contents(meta, stats, error_info);
		mnd_pefree(meta, meta->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_stmt_result_meta)
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, init),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, add_field),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, count),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, get_field),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_stmt_result_meta, dtor),
MYSQLND_CLASS_METHODS_END;


PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_stmt_result_meta);

/* {{{ xmysqlnd_node_stmt_result_meta_create */
PHPAPI XMYSQLND_NODE_STMT_RESULT_META *
xmysqlnd_node_stmt_result_meta_create(const zend_bool persistent,
									  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
									  MYSQLND_STATS * stats,
									  MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_NODE_STMT_RESULT_META * object = NULL;
	DBG_ENTER("xmysqlnd_node_stmt_result_meta_create");
	object = object_factory->get_node_stmt_result_meta(object_factory, persistent, stats, error_info);
	DBG_RETURN(object);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_meta_free */
PHPAPI void
xmysqlnd_node_stmt_result_meta_free(XMYSQLND_NODE_STMT_RESULT_META * const object, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_stmt_result_meta_free");
	if (object) {
		object->m->dtor(object, stats, error_info);
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
