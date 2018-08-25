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
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_connection.h"
#include "mysqlx_pfc.h"

#include "xmysqlnd/proto_gen/mysqlx_connection.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_session.pb.h"

#include "mysqlx_message__ok.h"
#include "mysqlx_message__error.h"
#include "mysqlx_message__auth_ok.h"

#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

static zend_class_entry *mysqlx_message__auth_ok_class_entry;

struct st_mysqlx_message__auth_ok
{
	Mysqlx::Session::AuthenticateOk message;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__AUTH_OK_FROM_ZVAL(_to, _from) \
{ \
	st_mysqlx_object* mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_message__auth_ok*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__auth_ok__response, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


/* {{{ proto long mysqlx_message__auth_ok::response(object messsage) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_message__auth_ok, response)
{
	zval* object_zv{nullptr};
	st_mysqlx_message__auth_ok* object{nullptr};

	DBG_ENTER("mysqlx_message__auth_ok::response");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_message__auth_ok_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__AUTH_OK_FROM_ZVAL(object, object_zv);

	if (object->message.has_auth_data()) {
		RETVAL_STRINGL(object->message.auth_data().c_str(), object->message.auth_data().size());
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__auth_ok_methods[] */
static const zend_function_entry mysqlx_message__auth_ok_methods[] = {
	PHP_ME(mysqlx_message__auth_ok, response,	mysqlx_message__auth_ok__response,			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__auth_ok_handlers;
static HashTable mysqlx_message__auth_ok_properties;

/* {{{ mysqlx_message__auth_ok_free_storage */
static void
mysqlx_message__auth_ok_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_message__auth_ok* message = (st_mysqlx_message__auth_ok*) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_message__auth_ok_object_allocator */
static zend_object *
php_mysqlx_message__auth_ok_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	st_mysqlx_object* mysqlx_object = (st_mysqlx_object*) mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	st_mysqlx_message__auth_ok* message = new (std::nothrow) struct st_mysqlx_message__auth_ok();

	DBG_ENTER("php_mysqlx_message__auth_ok_object_allocator");
	if ( mysqlx_object && message ) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_message__auth_ok_handlers;
		mysqlx_object->properties = &mysqlx_message__auth_ok_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (mysqlx_object) {
		mnd_efree(mysqlx_object);
	}
	delete message;
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_message__auth_ok_class */
void
mysqlx_register_message__auth_ok_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__auth_ok_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__auth_ok_handlers.free_obj = mysqlx_message__auth_ok_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__auth_ok", mysqlx_message__auth_ok_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "pfc", mysqlx_message__auth_ok_methods);
		tmp_ce.create_object = php_mysqlx_message__auth_ok_object_allocator;
		mysqlx_message__auth_ok_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__auth_ok_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__auth_ok_class */
void
mysqlx_unregister_message__auth_ok_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__auth_ok_properties);
}
/* }}} */


/* {{{ mysqlx_new_message__auth_ok */
void
mysqlx_new_message__auth_ok(zval * return_value, const Mysqlx::Session::AuthenticateOk & message)
{
	st_mysqlx_message__auth_ok* obj{nullptr};
	DBG_ENTER("mysqlx_new_message__auth_ok");
	object_init_ex(return_value, mysqlx_message__auth_ok_class_entry);
	MYSQLX_FETCH_MESSAGE__AUTH_OK_FROM_ZVAL(obj, return_value);
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
