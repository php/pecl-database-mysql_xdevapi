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
#include <zend_smart_str.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include "mysqlx_resultset__column_metadata.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_column_metadata_class_entry;

/* {{{ mysqlx_column_metadata_free_storage */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_metadata, __construct)
{
}
/* }}} */


/* {{{ mysqlx_column_metadata_methods[] */
static const zend_function_entry mysqlx_column_metadata_methods[] = {
	PHP_ME(mysqlx_column_metadata, __construct,				nullptr,			ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_column_meta_property__type */
static zval *
mysqlx_column_meta_property__type(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__type");
	if (object->message.has_type()) {
		ZVAL_LONG(return_value, object->message.type());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__type_name */
static zval *
mysqlx_column_meta_property__type_name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__type_name");
	if (object->message.has_type()) {
		const std::string & field = Mysqlx::Resultset::ColumnMetaData::FieldType_Name(object->message.type());
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__name */
static zval *
mysqlx_column_meta_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__name");
	if (object->message.has_name()) {
		const std::string & field = object->message.name();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__original_name */
static zval *
mysqlx_column_meta_property__original_name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__original_name");
	if (object->message.has_original_name()) {
		const std::string & field = object->message.original_name();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__table */
static zval *
mysqlx_column_meta_property__table(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__table");
	if (object->message.has_table()) {
		const std::string & field = object->message.table();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__original_table */
static zval *
mysqlx_column_meta_property__original_table(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__original_table");
	if (object->message.has_original_table()) {
		const std::string & field = object->message.original_table();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__schema */
static zval *
mysqlx_column_meta_property__schema(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__schema");
	if (object->message.has_schema()) {
		const std::string & field = object->message.schema();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__catalog */
static zval *
mysqlx_column_meta_property__catalog(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__catalog");
	if (object->message.has_catalog()) {
		const std::string & field = object->message.catalog();
		ZVAL_STRINGL(return_value, field.c_str(), field.size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__collation */
static zval *
mysqlx_column_meta_property__collation(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__collation");
	if (object->message.has_collation()) {
		ZVAL_LONG(return_value, object->message.collation());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__fractional_digits */
static zval *
mysqlx_column_meta_property__fractional_digits(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__fractional_digits");
	if (object->message.has_fractional_digits()) {
		ZVAL_LONG(return_value, object->message.fractional_digits());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__length */
static zval *
mysqlx_column_meta_property__length(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__length");
	if (object->message.has_length()) {
		ZVAL_LONG(return_value, object->message.length());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__flags */
static zval *
mysqlx_column_meta_property__flags(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__flags");

	if (object->message.has_flags()) {
		ZVAL_LONG(return_value, object->message.flags());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property__content_type */
static zval *
mysqlx_column_meta_property__content_type(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_column_metadata * object = static_cast<struct st_mysqlx_column_metadata *>(obj->ptr);
	DBG_ENTER("mysqlx_column_meta_property__content_type");
	if (object->message.has_content_type()) {
		ZVAL_LONG(return_value, object->message.content_type());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_column_meta_property_entries[] =
{
	{{"type",				sizeof("type") - 1},				mysqlx_column_meta_property__type,			nullptr},
	{{"type_name",			sizeof("type_name") - 1},			mysqlx_column_meta_property__type_name, 	nullptr},
	{{"name",				sizeof("name") - 1},				mysqlx_column_meta_property__name,			nullptr},
	{{"original_name",		sizeof("original_name") - 1},		mysqlx_column_meta_property__original_name,	nullptr},
	{{"table",				sizeof("table") - 1},				mysqlx_column_meta_property__table,			nullptr},
	{{"original_table",		sizeof("original_table") - 1},		mysqlx_column_meta_property__original_table,nullptr},
	{{"schema",				sizeof("schema") - 1},				mysqlx_column_meta_property__schema,		nullptr},
	{{"catalog",			sizeof("catalog") - 1},				mysqlx_column_meta_property__catalog,		nullptr},
	{{"collation",			sizeof("collation") - 1},			mysqlx_column_meta_property__collation,		nullptr},
	{{"fractional_digits",	sizeof("fractional_digits") - 1},	mysqlx_column_meta_property__fractional_digits,nullptr},
	{{"length",				sizeof("length") - 1},				mysqlx_column_meta_property__length,		nullptr},
	{{"flags",				sizeof("flags") - 1},				mysqlx_column_meta_property__flags,			nullptr},
	{{"content_type",		sizeof("content_type") - 1},		mysqlx_column_meta_property__content_type,	nullptr},
	{{nullptr, 				0},									nullptr, 										nullptr}
};
/* }}} */



static zend_object_handlers mysqlx_object_column_metadata_handlers;
static HashTable mysqlx_column_metadata_properties;

/* {{{ mysqlx_column_metadata_free_storage */
static void
mysqlx_column_metadata_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_column_metadata * message = (struct st_mysqlx_column_metadata  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_column_metadata_object_allocator */
static zend_object *
php_mysqlx_column_metadata_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_column_metadata * message = new (std::nothrow) struct st_mysqlx_column_metadata();

	DBG_ENTER("php_mysqlx_column_metadata_object_allocator");
	if ( mysqlx_object && message) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_column_metadata_handlers;
		mysqlx_object->properties = &mysqlx_column_metadata_properties;

		DBG_RETURN(&mysqlx_object->zo);
	}
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_column_metadata_class */
void
mysqlx_register_column_metadata_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_column_metadata_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_column_metadata_handlers.free_obj = mysqlx_column_metadata_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_column_metadata", mysqlx_column_metadata_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "node_pfc", mysqlx_column_metadata_methods);
		tmp_ce.create_object = php_mysqlx_column_metadata_object_allocator;
		mysqlx_column_metadata_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_column_metadata_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_column_metadata_properties, mysqlx_column_meta_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "type",			sizeof("type") - 1,				ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "type_name",		sizeof("type_name") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "name",			sizeof("name") - 1,				ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "original_name",	sizeof("original_name") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "table",			sizeof("table") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "original_table",sizeof("original_table") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "schema",		sizeof("schema") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "catalog",		sizeof("catalog") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "collation",		sizeof("collation") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "fractional_digits",	sizeof("fractional_digits") - 1, ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "length",		sizeof("length") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "flags",			sizeof("flags") - 1,			ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_column_metadata_class_entry, "content_type",	sizeof("content_type") - 1,		ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_column_metadata_class */
void
mysqlx_unregister_column_metadata_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_column_metadata_properties);
}
/* }}} */


/* {{{ mysqlx_new_column_metadata */
void
mysqlx_new_column_metadata(zval * return_value, const Mysqlx::Resultset::ColumnMetaData & message)
{
	struct st_mysqlx_column_metadata * obj;
	DBG_ENTER("mysqlx_new_column_metadata");
	object_init_ex(return_value, mysqlx_column_metadata_class_entry);
	MYSQLX_FETCH_MESSAGE__COLUMN_METADATA_FROM_ZVAL(obj, return_value);
	obj->message.CopyFrom(message);
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace msg

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
