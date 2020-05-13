/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include <zend_exceptions.h>
#include "mysqlnd_api.h"
#include <ext/spl/spl_exceptions.h> /* spl_ce_RuntimeException */
}
#include "xmysqlnd/xmysqlnd.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include "mysqlx_exception.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

zend_class_entry * mysqlx_exception_class_entry;

struct st_mysqlx_exception
{
	MYSQLND_STRING msg;
	unsigned int level;
	unsigned int code;
	zend_bool persistent;
};

void
RAISE_EXCEPTION(const int errcode, const char * const msg)
{
	const MYSQLND_CSTRING sqlstate = { GENERAL_SQL_STATE, sizeof(GENERAL_SQL_STATE) - 1 };
	const MYSQLND_CSTRING errmsg = { msg, sizeof(msg) - 1 };
	mysqlx_new_exception(errcode, sqlstate, errmsg);
}

static const zend_function_entry mysqlx_exception_methods[] = {
	{nullptr, nullptr, nullptr}
};

static zval *
mysqlx_exception_property__message(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_exception* object = (const st_mysqlx_exception* ) (obj->ptr);
	DBG_ENTER("mysqlx_exception_property__message");
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

static zval *
mysqlx_exception_property__level(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_exception* object = (const st_mysqlx_exception* ) (obj->ptr);
	DBG_ENTER("mysqlx_exception_property__level");
	ZVAL_LONG(return_value, object->level);
	DBG_RETURN(return_value);
}

static zval *
mysqlx_exception_property__code(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_exception* object = (const st_mysqlx_exception* ) (obj->ptr);
	DBG_ENTER("mysqlx_exception_property__code");
	/* code is 32 bit unsigned and on 32bit system won't fit into 32 bit signed zend_long, but this won't happen in practice*/
	ZVAL_LONG(return_value, object->code);
	DBG_RETURN(return_value);
}

static const st_mysqlx_property_entry mysqlx_exception_property_entries[] =
{
	{{"message",			sizeof("message") - 1},		mysqlx_exception_property__message,	nullptr},
	{{"level",				sizeof("level") - 1},		mysqlx_exception_property__level,		nullptr},
	{{"code",				sizeof("code") - 1},		mysqlx_exception_property__code,		nullptr},
	{{nullptr, 				0},							nullptr, 								nullptr}
};

static zend_object_handlers mysqlx_object_exception_handlers;
static HashTable mysqlx_exception_properties;


void
mysqlx_register_exception_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_DERIVED_CLASS(
		mysqlx_exception_class_entry,
		spl_ce_RuntimeException,
		"Exception",
		mysqlx_std_object_handlers,
		mysqlx_object_exception_handlers,
		mysqlx_exception_methods,
		mysqlx_exception_properties,
		mysqlx_exception_property_entries);
}

void
mysqlx_unregister_exception_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_exception_properties);
}

void
mysqlx_new_exception(const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message)
{
	char* msg{nullptr};
	DBG_ENTER("mysqlx_new_exception");
	mnd_sprintf(&msg, 0, "[%*s] %*s", sql_state.l, sql_state.s, message.l, message.s);
	if (msg) {
		zend_throw_exception(mysqlx_exception_class_entry, msg, code);
		mnd_efree(msg);
	}
	DBG_VOID_RETURN;
}

void
mysqlx_new_exception_ex(const unsigned int code, const MYSQLND_CSTRING /*sql_state*/, const char * const format, ...)
{
	va_list args;
	char * msg;

	DBG_ENTER("mysqlx_new_exception");
	va_start(args, format);
	mnd_vsprintf(&msg, 0, format, args);
	va_end(args);

	zend_throw_exception(mysqlx_exception_class_entry, msg, code);
	mnd_efree(msg);
	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
