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
extern "C"
{
#include <php.h>
#undef ERROR
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include <new>
#include "mysqlx_message__error.h"

zend_class_entry *mysqlx_message__error_class_entry;

/* {{{ mysqlx_message__error::__construct */
PHP_METHOD(mysqlx_message__error, __construct)
{
}
/* }}} */

/* {{{ mysqlx_message__error_methods[] */
static const zend_function_entry mysqlx_message__error_methods[] = {
	PHP_ME(mysqlx_message__error, __construct,	NULL,	ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_message__error_property__message */
static zval *
mysqlx_message__error_property__message(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_message__error * object = static_cast<struct st_mysqlx_message__error *>(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__message");
	if (object->message.has_msg()) {
		ZVAL_STRINGL(return_value, object->message.msg().c_str(), object->message.msg().size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_message__error_property__sql_state */
static zval *
mysqlx_message__error_property__sql_state(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_message__error * object = static_cast<struct st_mysqlx_message__error *>(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__sql_state");
	if (object->message.has_sql_state()) {
		ZVAL_STRINGL(return_value, object->message.sql_state().c_str(), object->message.sql_state().size());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_message__error_property__error_code */
static zval *
mysqlx_message__error_property__error_code(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_message__error * object = static_cast<struct st_mysqlx_message__error *>(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__error_code");
	if (object->message.has_code()) {
		/* code is 32 bit unsigned and on 32bit system won't fit into 32 bit signed zend_long, but this won't happen in practice*/
		ZVAL_LONG(return_value, object->message.code());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_column_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_message__error_property_entries[] =
{
	{{"message",			sizeof("message") - 1},		mysqlx_message__error_property__message,	NULL},
	{{"sql_state",			sizeof("sql_state") - 1},	mysqlx_message__error_property__sql_state,	NULL},
	{{"code",				sizeof("code") - 1},		mysqlx_message__error_property__error_code,	NULL},
	{{NULL, 				0},							NULL, 										NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__error_handlers;
static HashTable mysqlx_message__error_properties;


/* {{{ mysqlx_message__error_free_storage */
static void
mysqlx_message__error_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__error * message = (struct st_mysqlx_message__error  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_message__error_object_allocator */
static zend_object *
php_mysqlx_message__error_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_message__error * message = new (std::nothrow) struct st_mysqlx_message__error;

	DBG_ENTER("php_mysqlx_message__error_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_message__error_handlers;
	mysqlx_object->properties = &mysqlx_message__error_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_message__error_class */
extern "C" void
mysqlx_register_message__error_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__error_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__error_handlers.free_obj = mysqlx_message__error_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__error", mysqlx_message__error_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "node_pfc", mysqlx_message__error_methods);
		tmp_ce.create_object = php_mysqlx_message__error_object_allocator;
		mysqlx_message__error_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__error_properties, 0, NULL, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_message__error_properties, mysqlx_message__error_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_message__error_class_entry, "message",	sizeof("message") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_message__error_class_entry, "sql_state",	sizeof("sql_state") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_message__error_class_entry, "code",		sizeof("code") - 1,			ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_message__error_class */
extern "C" void
mysqlx_unregister_message__error_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__error_properties);
}
/* }}} */


/* {{{ mysqlx_new_message__error */
void
mysqlx_new_message__error(zval * return_value, const Mysqlx::Error & message)
{
	struct st_mysqlx_message__error * error;
	DBG_ENTER("mysqlx_new_message__error");
	object_init_ex(return_value, mysqlx_message__error_class_entry);
	MYSQLX_FETCH_MESSAGE__ERROR__FROM_ZVAL(error, return_value);
	error->message.CopyFrom(message);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_new_message__error */
void
mysqlx_new_message__error(zval * return_value, const char * msg, const char * sql_state, const unsigned int code)
{
	struct st_mysqlx_message__error * error;
	DBG_ENTER("mysqlx_new_message__error");
	object_init_ex(return_value, mysqlx_message__error_class_entry);
	MYSQLX_FETCH_MESSAGE__ERROR__FROM_ZVAL(error, return_value);
	error->message.set_msg(msg);
	error->message.set_sql_state(sql_state);
	error->message.set_code(code);
	error->message.set_severity(Mysqlx::Error::FATAL);
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ dump_mysqlx_error */
void
dump_mysqlx_error(const Mysqlx::Error & error)
{
	const char * error_severity = "Uknown Severity";
	uint32_t code = 0;
	const char * sql_state = "00000";
	const char * message = "";
	if (error.has_severity()) {
		if (error.severity() == Mysqlx::Error_Severity_ERROR) {
			error_severity = "ERROR";
		} else if (error.severity() == Mysqlx::Error_Severity_FATAL) {
			error_severity = "FATAL";
		}
	}
	if (error.has_code()) {
		code = error.code();
	}
	if (error.has_sql_state()) {
		sql_state = error.sql_state().c_str();
	}
	if (error.has_msg()) {
		message = error.msg().c_str();
	}
	php_error_docref(NULL, E_WARNING, "[%s][%u][%s] %s", error_severity, code, sql_state, message);
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
