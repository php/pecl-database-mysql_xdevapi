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
#include <xmysqlnd/xmysqlnd_wireprotocol.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"

#include <new>
#include "xmysqlnd/proto_gen/mysqlx_connection.pb.h"

#include "mysqlx_message__ok.h"
#include "mysqlx_message__error.h"
#include "mysqlx_message__auth_start.h"
#include "mysqlx_message__auth_continue.h"
#include "mysqlx_message__auth_ok.h"

static zend_class_entry *mysqlx_message__auth_start_class_entry;

struct st_mysqlx_message__auth_start
{
	struct st_xmysqlnd_msg__auth_start msg;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__AUTH_START_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_message__auth_start *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__auth_start__send, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_TYPE_INFO(0, auth_mechanism, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, auth_data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__auth_start__read_response, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


/* {{{ proto long mysqlx_message__auth_start::send(object messsage, string auth_mechanism, string auth_data, object pfc, object connection) */
PHP_METHOD(mysqlx_message__auth_start, send)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	char * auth_mech_name = NULL;
	size_t auth_mech_name_len = 0;
	char * auth_data = NULL;
	size_t auth_data_len = 0;
	struct st_mysqlx_message__auth_start * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;

	DBG_ENTER("mysqlx_message__auth_start::send");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OssOO",
												&object_zv, mysqlx_message__auth_start_class_entry,
												&auth_mech_name, &auth_mech_name_len,
												&auth_data, &auth_data_len,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__AUTH_START_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	const XMYSQLND_L3_IO io = {connection->vio, codec->pfc};
	const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&io, connection->stats, connection->error_info);
	object->msg = msg_factory.get__auth_start(&msg_factory);

	const MYSQLND_CSTRING mech_name = {auth_mech_name, auth_mech_name_len};
	const MYSQLND_CSTRING mech_data = {auth_data, auth_data_len};
	enum_func_status ret = object->msg.send_request(&object->msg, mech_name, mech_data);
	RETVAL_BOOL(ret == PASS);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_message__auth_start::read_response(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__auth_start, read_response)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__auth_start * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;

	DBG_ENTER("mysqlx_message__auth_start::read_response");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOO",
												&object_zv, mysqlx_message__auth_start_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__AUTH_START_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	RETVAL_FALSE;
	enum_func_status ret = object->msg.read_response(&object->msg, return_value);
	if (FAIL == ret) {
		mysqlx_new_message__error(return_value, connection->error_info->error, connection->error_info->sqlstate, connection->error_info->error_no);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__auth_start_methods[] */
static const zend_function_entry mysqlx_message__auth_start_methods[] = {
	PHP_ME(mysqlx_message__auth_start, send,				mysqlx_message__auth_start__send,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_message__auth_start, read_response,		mysqlx_message__auth_start__read_response,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__auth_start_handlers;
static HashTable mysqlx_message__auth_start_properties;

/* {{{ mysqlx_message__auth_start_free_storage */
static void
mysqlx_message__auth_start_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__auth_start * message = (struct st_mysqlx_message__auth_start  *) mysqlx_object->ptr;

	if (message) {
		delete message;
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_message__auth_start_object_allocator */
static zend_object *
php_mysqlx_message__auth_start_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_message__auth_start * message = new (std::nothrow) struct st_mysqlx_message__auth_start();

	DBG_ENTER("php_mysqlx_message__auth_start_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_message__auth_start_handlers;
	mysqlx_object->properties = &mysqlx_message__auth_start_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_message__auth_start_class */
extern "C" void
mysqlx_register_message__auth_start_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__auth_start_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__auth_start_handlers.free_obj = mysqlx_message__auth_start_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__auth_start", mysqlx_message__auth_start_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "node_pfc", mysqlx_message__auth_start_methods);
		tmp_ce.create_object = php_mysqlx_message__auth_start_object_allocator;
		mysqlx_message__auth_start_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__auth_start_properties, 0, NULL, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__auth_start_class */
extern "C" void
mysqlx_unregister_message__auth_start_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__auth_start_properties);
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
