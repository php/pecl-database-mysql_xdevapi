/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_wireprotocol.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "util/allocator.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_field_metadata_class_entry;

struct st_mysqlx_field_metadata : public util::permanent_allocable
{
	XMYSQLND_RESULT_FIELD_META * field_meta;
	zend_bool persistent;
};

#define TYPE_NAME_ENABLED 1


#define MYSQLX_FETCH__FIELD_METADATA_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_field_metadata*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_field_metadata::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_field_metadata, __construct)
{
}
/* }}} */


/* {{{ mysqlx_field_metadata_methods[] */
static const zend_function_entry mysqlx_field_metadata_methods[] = {
	PHP_ME(mysqlx_field_metadata, __construct,				nullptr,			ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_field_meta_property__type */
static zval *
mysqlx_field_meta_property__type(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__type");
	ZVAL_LONG(return_value, object->field_meta->type);
	DBG_RETURN(return_value);
}
/* }}} */


#if TYPE_NAME_ENABLED
/* {{{ mysqlx_field_meta_property__type_name */
static zval *
mysqlx_field_meta_property__type_name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	const MYSQLND_CSTRING type_name = xmysqlnd_field_type_name(object->field_meta->type);
	DBG_ENTER("mysqlx_field_meta_property__type_name");
	ZVAL_STRINGL(return_value, type_name.s, type_name.l);
	DBG_RETURN(return_value);
}
/* }}} */
#endif


/* {{{ mysqlx_field_meta_property__name */
static zval *
mysqlx_field_meta_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__name");
	ZVAL_STRINGL(return_value, object->field_meta->name.s, object->field_meta->name.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__original_name */
static zval *
mysqlx_field_meta_property__original_name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__original_name");
	ZVAL_STRINGL(return_value, object->field_meta->original_name.s, object->field_meta->original_name.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__table */
static zval *
mysqlx_field_meta_property__table(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__table");
	ZVAL_STRINGL(return_value, object->field_meta->table.s, object->field_meta->table.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__original_table */
static zval *
mysqlx_field_meta_property__original_table(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__original_table");
	ZVAL_STRINGL(return_value, object->field_meta->original_table.s, object->field_meta->original_table.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__schema */
static zval *
mysqlx_field_meta_property__schema(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__schema");
	ZVAL_STRINGL(return_value, object->field_meta->schema.s, object->field_meta->schema.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__catalog */
static zval *
mysqlx_field_meta_property__catalog(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__catalog");
	ZVAL_STRINGL(return_value, object->field_meta->catalog.s, object->field_meta->catalog.l);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__collation */
static zval *
mysqlx_field_meta_property__collation(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__collation");
	ZVAL_LONG(return_value, object->field_meta->collation);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__fractional_digits */
static zval *
mysqlx_field_meta_property__fractional_digits(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__fractional_digits");
	ZVAL_LONG(return_value, object->field_meta->fractional_digits);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__length */
static zval *
mysqlx_field_meta_property__length(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__length");
	ZVAL_LONG(return_value, object->field_meta->length);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__flags */
static zval *
mysqlx_field_meta_property__flags(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__flags");
	ZVAL_LONG(return_value, object->field_meta->flags);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property__content_type */
static zval *
mysqlx_field_meta_property__content_type(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_field_metadata* object = (st_mysqlx_field_metadata*)(obj->ptr);
	DBG_ENTER("mysqlx_field_meta_property__content_type");
	ZVAL_LONG(return_value, object->field_meta->content_type);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_field_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_field_meta_property_entries[] =
{
	{{"type", sizeof("type") - 1}, mysqlx_field_meta_property__type, nullptr},
#if TYPE_NAME_ENABLED
	{{"type_name", sizeof("type_name") - 1}, mysqlx_field_meta_property__type_name, nullptr},
#endif
	{{"name", sizeof("name") - 1}, mysqlx_field_meta_property__name, nullptr},
	{{"original_name", sizeof("original_name") - 1}, mysqlx_field_meta_property__original_name, nullptr},
	{{"table", sizeof("table") - 1}, mysqlx_field_meta_property__table, nullptr},
	{{"original_table", sizeof("original_table") - 1}, mysqlx_field_meta_property__original_table,nullptr},
	{{"schema", sizeof("schema") - 1}, mysqlx_field_meta_property__schema, nullptr},
	{{"catalog", sizeof("catalog") - 1}, mysqlx_field_meta_property__catalog, nullptr},
	{{"collation", sizeof("collation") - 1}, mysqlx_field_meta_property__collation, nullptr},
	{{"fractional_digits", sizeof("fractional_digits") - 1}, mysqlx_field_meta_property__fractional_digits,nullptr},
	{{"length", sizeof("length") - 1}, mysqlx_field_meta_property__length, nullptr},
	{{"flags", sizeof("flags") - 1}, mysqlx_field_meta_property__flags, nullptr},
	{{"content_type", sizeof("content_type") - 1}, mysqlx_field_meta_property__content_type, nullptr},
	{{nullptr, 0}, nullptr, nullptr}
};
/* }}} */



static zend_object_handlers mysqlx_object_field_metadata_handlers;
static HashTable mysqlx_field_metadata_properties;

/* {{{ mysqlx_field_metadata_free_storage */
static void
mysqlx_field_metadata_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_field_metadata* message = (st_mysqlx_field_metadata*) mysqlx_object->ptr;

	if (message) {
		if (message->field_meta) {
			xmysqlnd_result_field_meta_free(message->field_meta, nullptr, nullptr);
		}
		mnd_pefree(message, message->persistent);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_field_metadata_object_allocator */
static zend_object *
php_mysqlx_field_metadata_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_field_metadata_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_permanent_object<st_mysqlx_field_metadata>(
		class_type,
		&mysqlx_object_field_metadata_handlers,
		&mysqlx_field_metadata_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_field_metadata_class */
void
mysqlx_register_field_metadata_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_field_metadata_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_field_metadata_handlers.free_obj = mysqlx_field_metadata_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "FieldMetadata", mysqlx_field_metadata_methods);
		tmp_ce.create_object = php_mysqlx_field_metadata_object_allocator;
		mysqlx_field_metadata_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_field_metadata_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_field_metadata_properties, mysqlx_field_meta_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "type",			sizeof("type") - 1,				ZEND_ACC_PUBLIC);
#if TYPE_NAME_ENABLED
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "type_name",		sizeof("type_name") - 1,		ZEND_ACC_PUBLIC);
#endif
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "name",			sizeof("name") - 1,				ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "original_name",	sizeof("original_name") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "table",			sizeof("table") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "original_table",	sizeof("original_table") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "schema",			sizeof("schema") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "catalog",		sizeof("catalog") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "collation",		sizeof("collation") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "fractional_digits",sizeof("fractional_digits") - 1, ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "length",			sizeof("length") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "flags",			sizeof("flags") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_field_metadata_class_entry, "content_type",	sizeof("content_type") - 1,		ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_field_metadata_class */
void
mysqlx_unregister_field_metadata_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_field_metadata_properties);
}
/* }}} */


/* {{{ mysqlx_new_field_metadata */
void
mysqlx_new_field_metadata(zval * return_value, const XMYSQLND_RESULT_FIELD_META * const field_meta)
{
	DBG_ENTER("mysqlx_new_field_metadata");
	object_init_ex(return_value, mysqlx_field_metadata_class_entry);

	if (SUCCESS == object_init_ex(return_value, mysqlx_field_metadata_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_field_metadata* const object = (st_mysqlx_field_metadata*) mysqlx_object->ptr;
		if (object) {
			object->field_meta = field_meta->m->clone(field_meta, nullptr, nullptr);
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
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
