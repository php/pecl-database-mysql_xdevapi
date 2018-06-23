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
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_warning.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_warning_class_entry;

struct st_mysqlx_warning
{
	MYSQLND_STRING msg;
	unsigned int level;
	unsigned int code;
	zend_bool persistent;
};


#define MYSQLX_FETCH_WARNING_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_warning*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
}


/* {{{ mysqlx_warning::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_warning, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */

/* {{{ mysqlx_warning_methods[] */
static const zend_function_entry mysqlx_warning_methods[] = {
	PHP_ME(mysqlx_warning, __construct,	nullptr,	ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_warning_property__message */
static zval *
mysqlx_warning_property__message(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_warning* object = (const st_mysqlx_warning* ) (obj->ptr);
	DBG_ENTER("mysqlx_warning_property__message");
	if (object->msg.s) {
		ZVAL_STRINGL(return_value, object->msg.s, object->msg.l);
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


/* {{{ mysqlx_warning_property__level */
static zval *
mysqlx_warning_property__level(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_warning* object = (const st_mysqlx_warning* ) (obj->ptr);
	DBG_ENTER("mysqlx_warning_property__level");
	ZVAL_LONG(return_value, object->level);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_warning_property__code */
static zval *
mysqlx_warning_property__code(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_warning* object = (const st_mysqlx_warning* ) (obj->ptr);
	DBG_ENTER("mysqlx_warning_property__code");
	/* code is 32 bit unsigned and on 32bit system won't fit into 32 bit signed zend_long, but this won't happen in practice*/
	ZVAL_LONG(return_value, object->code);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_warning_property_entries[] =
{
	{{"message",			sizeof("message") - 1},		mysqlx_warning_property__message,	nullptr},
	{{"level",				sizeof("level") - 1},		mysqlx_warning_property__level,		nullptr},
	{{"code",				sizeof("code") - 1},		mysqlx_warning_property__code,		nullptr},
	{{nullptr, 				0},							nullptr, 								nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_warning_handlers;
static HashTable mysqlx_warning_properties;


/* {{{ mysqlx_warning_free_storage */
static void
mysqlx_warning_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_warning* message = (st_mysqlx_warning*) mysqlx_object->ptr;

	if (message) {
		const zend_bool pers = message->persistent;
		if (message->msg.s) {
			mnd_pefree(message->msg.s, pers);
			message->msg.s = nullptr;
		}
		mnd_pefree(message, message->persistent);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_warning_object_allocator */
static zend_object *
php_mysqlx_warning_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	st_mysqlx_object* mysqlx_object = (st_mysqlx_object*) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	st_mysqlx_warning* message = (st_mysqlx_warning*)  mnd_pecalloc(1, sizeof(struct st_mysqlx_warning), persistent);

	DBG_ENTER("php_mysqlx_warning_object_allocator");
	if ( mysqlx_object && message) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_warning_handlers;
		mysqlx_object->properties = &mysqlx_warning_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (message) {
		mnd_pefree(message, persistent);
	}
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_warning_class */
void
mysqlx_register_warning_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_warning_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_warning_handlers.free_obj = mysqlx_warning_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "Warning", mysqlx_warning_methods);
		tmp_ce.create_object = php_mysqlx_warning_object_allocator;
		mysqlx_warning_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_warning_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_warning_properties, mysqlx_warning_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_warning_class_entry, "message",	sizeof("message") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_warning_class_entry, "level",		sizeof("level") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_warning_class_entry, "code",		sizeof("code") - 1,			ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_warning_class */
void
mysqlx_unregister_warning_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_warning_properties);
}
/* }}} */


/* {{{ mysqlx_new_warning */
void
mysqlx_new_warning(zval * return_value, const MYSQLND_CSTRING msg, unsigned int level, const unsigned int code)
{
	DBG_ENTER("mysqlx_new_warning");
	if (SUCCESS == object_init_ex(return_value, mysqlx_warning_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_warning* const object = (st_mysqlx_warning*) mysqlx_object->ptr;
		if (object) {
			object->msg = mnd_dup_cstring(msg, object->persistent);
			object->level = level;
			object->code = code;
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
