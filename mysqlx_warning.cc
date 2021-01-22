/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#include "util/functions.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_warning_class_entry;

struct st_mysqlx_warning : public util::custom_allocable
{
	util::string msg;
	unsigned int level;
	unsigned int code;
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_warning__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_warning, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

static const zend_function_entry mysqlx_warning_methods[] = {
	PHP_ME(mysqlx_warning, __construct, arginfo_mysqlx_warning__construct, ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};

static zval*
mysqlx_warning_property__message(const st_mysqlx_object* obj, zval* return_value)
{
	DBG_ENTER("mysqlx_warning_property__message");
	const st_mysqlx_warning& data_object{ util::fetch_data_object<st_mysqlx_warning>(obj) };
	if (!data_object.msg.empty()) {
		ZVAL_STRINGL(return_value, data_object.msg.data(), data_object.msg.length());
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

static zval*
mysqlx_warning_property__level(const st_mysqlx_object* obj, zval* return_value)
{
	DBG_ENTER("mysqlx_warning_property__level");
	const st_mysqlx_warning& data_object{ util::fetch_data_object<st_mysqlx_warning>(obj) };
	ZVAL_LONG(return_value, data_object.level);
	DBG_RETURN(return_value);
}

static zval*
mysqlx_warning_property__code(const st_mysqlx_object* obj, zval* return_value)
{
	DBG_ENTER("mysqlx_warning_property__code");
	const st_mysqlx_warning& data_object{ util::fetch_data_object<st_mysqlx_warning>(obj) };
	/* code is 32 bit unsigned and on 32bit system won't fit into 32 bit signed zend_long, but this won't happen in practice*/
	ZVAL_LONG(return_value, data_object.code);
	DBG_RETURN(return_value);
}

static const st_mysqlx_property_entry mysqlx_warning_property_entries[] =
{
	{std::string_view("message"), mysqlx_warning_property__message,	nullptr},
	{std::string_view("level"), mysqlx_warning_property__level,		nullptr},
	{std::string_view("code"), mysqlx_warning_property__code,		nullptr},
	{std::string_view{}, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_warning_handlers;
static HashTable mysqlx_warning_properties;


static void
mysqlx_warning_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_warning>(object);
}

static zend_object *
php_mysqlx_warning_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_warning_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_warning>(
		class_type,
		&mysqlx_object_warning_handlers,
		&mysqlx_warning_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_warning_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_warning_class_entry,
		"Warning",
		mysqlx_std_object_handlers,
		mysqlx_object_warning_handlers,
		php_mysqlx_warning_object_allocator,
		mysqlx_warning_free_storage,
		mysqlx_warning_methods,
		mysqlx_warning_properties,
		mysqlx_warning_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_warning_class_entry, "message",	sizeof("message") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_warning_class_entry, "level",		sizeof("level") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_warning_class_entry, "code",		sizeof("code") - 1,			ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_warning_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_warning_properties);
}

util::zvalue
create_warning(const util::string& msg, unsigned int level, const unsigned int code)
{
	DBG_ENTER("create_warning");
	util::zvalue warning_obj;
	st_mysqlx_warning& data_object{
		util::init_object<st_mysqlx_warning>(mysqlx_warning_class_entry, warning_obj) };
	data_object.msg = msg;
	data_object.level = level;
	data_object.code = code;
	DBG_RETURN(warning_obj);
}

} // namespace devapi

} // namespace mysqlx
