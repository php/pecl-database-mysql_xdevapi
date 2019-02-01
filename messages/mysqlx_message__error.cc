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
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include "mysqlx_message__error.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_message__error_class_entry;

/* {{{ mysqlx_message__error::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_message__error, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */

/* {{{ mysqlx_message__error_methods[] */
static const zend_function_entry mysqlx_message__error_methods[] = {
	PHP_ME(mysqlx_message__error, __construct,	nullptr,	ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_message__error_property__message */
static zval *
mysqlx_message__error_property__message(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_message__error* object = static_cast<st_mysqlx_message__error* >(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__message");
	if (object->message.has_msg()) {
		ZVAL_STRINGL(return_value, object->message.msg().c_str(), object->message.msg().size());
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


/* {{{ mysqlx_message__error_property__sql_state */
static zval *
mysqlx_message__error_property__sql_state(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_message__error* object = static_cast<st_mysqlx_message__error* >(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__sql_state");
	if (object->message.has_sql_state()) {
		ZVAL_STRINGL(return_value, object->message.sql_state().c_str(), object->message.sql_state().size());
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


/* {{{ mysqlx_message__error_property__error_code */
static zval *
mysqlx_message__error_property__error_code(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_message__error* object = static_cast<st_mysqlx_message__error* >(obj->ptr);
	DBG_ENTER("mysqlx_message__error_property__error_code");
	if (object->message.has_code()) {
		/* code is 32 bit unsigned and on 32bit system won't fit into 32 bit signed zend_long, but this won't happen in practice*/
		ZVAL_LONG(return_value, object->message.code());
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
static const struct st_mysqlx_property_entry mysqlx_message__error_property_entries[] =
{
	{{"message",			sizeof("message") - 1},		mysqlx_message__error_property__message,	nullptr},
	{{"sql_state",			sizeof("sql_state") - 1},	mysqlx_message__error_property__sql_state,	nullptr},
	{{"code",				sizeof("code") - 1},		mysqlx_message__error_property__error_code,	nullptr},
	{{nullptr, 				0},							nullptr, 										nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__error_handlers;
static HashTable mysqlx_message__error_properties;


/* {{{ mysqlx_message__error_free_storage */
static void
mysqlx_message__error_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_message__error* message = (st_mysqlx_message__error*) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_message__error_object_allocator */
static zend_object *
php_mysqlx_message__error_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	st_mysqlx_object* mysqlx_object = (st_mysqlx_object*) mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	st_mysqlx_message__error* message = new (std::nothrow) struct st_mysqlx_message__error;

	DBG_ENTER("php_mysqlx_message__error_object_allocator");
	if ( mysqlx_object && message ) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_message__error_handlers;
		mysqlx_object->properties = &mysqlx_message__error_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (mysqlx_object) {
		mnd_efree(mysqlx_object);
	}
	delete message;
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_message__error_class */
void
mysqlx_register_message__error_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__error_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__error_handlers.free_obj = mysqlx_message__error_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__error", mysqlx_message__error_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "pfc", mysqlx_message__error_methods);
		tmp_ce.create_object = php_mysqlx_message__error_object_allocator;
		mysqlx_message__error_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__error_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_message__error_properties, mysqlx_message__error_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_message__error_class_entry, "message",	sizeof("message") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_message__error_class_entry, "sql_state",	sizeof("sql_state") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_message__error_class_entry, "code",		sizeof("code") - 1,			ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_message__error_class */
void
mysqlx_unregister_message__error_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__error_properties);
}
/* }}} */


/* {{{ mysqlx_new_message__error */
void
mysqlx_new_message__error(zval * return_value, const Mysqlx::Error & message)
{
	st_mysqlx_message__error* error{nullptr};
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
	st_mysqlx_message__error* error{nullptr};
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
	uint32_t code{0};
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
	php_error_docref(nullptr, E_WARNING, "[%s][%u][%s] %s", error_severity, code, sql_state, message);
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
