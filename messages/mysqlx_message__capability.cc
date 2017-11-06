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
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
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
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_message__capability.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_message__capability_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_capability__construct, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto bool mysqlx_node_connection::__construct(string name, mixed value) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_message__capability, __construct)
{
	zval* capability_zv{nullptr};
	struct st_mysqlx_message__capability* capability{nullptr};
	char* capability_name{nullptr};
	size_t capability_name_len;
	zval* capability_value{nullptr};

	DBG_ENTER("mysqlx_node_connection::connect");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Osz",
												&capability_zv, mysqlx_message__capability_class_entry,
												&capability_name, &capability_name_len,
												&capability_value))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_MESSAGE__CAPABILITY_FROM_ZVAL(capability, capability_zv);
	{
		ZVAL_STRINGL(&capability->capability_name, capability_name, capability_name_len);
		ZVAL_DUP(&capability->capability_value, capability_value);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlx_node_connection::echo(string name, mixed value) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_message__capability, echo)
{
	zval* capability_zv{nullptr};
	struct st_mysqlx_message__capability* capability{nullptr};

	DBG_ENTER("mysqlx_node_connection::echo");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&capability_zv, mysqlx_message__capability_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_MESSAGE__CAPABILITY_FROM_ZVAL(capability, capability_zv);
	printf("echo Cap_name= [%s]\n", Z_STRVAL(capability->capability_name));
	printf("echo Cap_value [%lu]\n", Z_LVAL(capability->capability_value));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__capability_methods[] */
static const zend_function_entry mysqlx_message__capability_methods[] = {
	PHP_ME(mysqlx_message__capability, __construct,		arginfo_mysqlx_node_capability__construct,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_message__capability, echo,			nullptr,										ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__capability_handlers;
static HashTable mysqlx_message__capability_properties;


/* {{{ mysqlx_message__capability_free_storage */
static void
mysqlx_message__capability_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__capability * message = (struct st_mysqlx_message__capability  *) mysqlx_object->ptr;

	if (message) {
		if (!Z_ISUNDEF(message->capability_name)) {
			zval_dtor(&(message->capability_name));
		}
		if (!Z_ISUNDEF(message->capability_value)) {
			zval_dtor(&(message->capability_value));
		}
		mnd_pefree(message, message->persistent);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_message__capability_object_allocator */
static zend_object *
php_mysqlx_message__capability_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	const std::size_t bytes_count = sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type);
	st_mysqlx_object* mysqlx_object = static_cast<st_mysqlx_object*>(::operator new(bytes_count, phputils::permanent_tag));
	st_mysqlx_message__capability* message = new st_mysqlx_message__capability();

	DBG_ENTER("php_mysqlx_message__capability_object_allocator");
	if ( mysqlx_object && message ) {
		mysqlx_object->ptr = message;

		ZVAL_UNDEF(&(message->capability_name));
		ZVAL_UNDEF(&(message->capability_value));

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_message__capability_handlers;
		mysqlx_object->properties = &mysqlx_message__capability_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	if (message) {
		mnd_pefree(message, persistent);
	}
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_message__capability_class */
void
mysqlx_register_message__capability_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__capability_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__capability_handlers.free_obj = mysqlx_message__capability_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__capability", mysqlx_message__capability_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "message__capability", mysqlx_message__capability_methods);
		tmp_ce.create_object = php_mysqlx_message__capability_object_allocator;
		mysqlx_message__capability_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__capability_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__capability_class */
void
mysqlx_unregister_message__capability_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__capability_properties);
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
