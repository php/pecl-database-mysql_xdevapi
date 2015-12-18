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
#include <php.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_zval2any.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"

#include <xmysqlnd/xmysqlnd_wireprotocol.h>

#include <new>
#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"
#include "mysqlx_message__error.h"


zend_class_entry *mysqlx_message__capabilities_get_class_entry;

struct st_mysqlx_message__capabilities_get
{
	Mysqlx::Connection::Capabilities response;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__CAPABILITIES_GET__FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_message__capabilities_get *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__capabilities_get__send, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__capabilities_get__read_response, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()

/* {{{ proto bool mysqlx_message__capabilities_get::send(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__capabilities_get, send)
{
	zval * message_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__capabilities_get * message;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlx_message__capabilities_get::send");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOO",
												&message_zv, mysqlx_message__capabilities_get_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__CAPABILITIES_GET__FROM_ZVAL(message, message_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	ret = xmysqlnd_send__capabilities_get(connection->vio, codec->pfc, connection->stats, connection->error_info);

	RETVAL_BOOL(ret == PASS);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto capabilities_to_zv */
void
capabilities_to_zv(const Mysqlx::Connection::Capabilities & message, zval * return_value)
{
	DBG_ENTER("capabilities_to_zv");
	array_init_size(return_value, message.capabilities_size());
	for (unsigned int i = 0; i < message.capabilities_size(); ++i) {
		zval zv = {0};
		any2zval(message.capabilities(i).value(), &zv);
		if (Z_REFCOUNTED(zv)) {
			Z_ADDREF(zv);
		}
		add_assoc_zval_ex(return_value, message.capabilities(i).name().c_str(), message.capabilities(i).name().size(), &zv);
		zval_ptr_dtor(&zv);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto zv_to_capabilities */
void
zv_to_capabilities(const zval * from, Mysqlx::Connection::Capabilities & message)
{
	zval * entry;
	zend_ulong num_idx;
	zend_string *str_idx;

	DBG_ENTER("zv_to_capabilities");
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(from), num_idx, str_idx, entry) {
		Mysqlx::Connection::Capability * new_capability = message.add_capabilities();
		if (str_idx) {
			DBG_INF_FMT("cap_name=%s", ZSTR_VAL(str_idx));
			new_capability->set_name(ZSTR_VAL(str_idx), ZSTR_LEN(str_idx));
		} else {
			char * idx;
			size_t idx_len = spprintf(&idx, 0, ZEND_ULONG_FMT, num_idx);
			new_capability->set_name(idx, idx_len);
			DBG_INF_FMT("cap_name=%s", idx);
			efree(idx);
		}
		Mysqlx::Datatypes::Any any;
		ZVAL_DEREF(entry);
		zval2any(entry, any);
		new_capability->mutable_value()->CopyFrom(any); /* maybe Swap() as the internal value will be empty anyway */
	} ZEND_HASH_FOREACH_END();
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_message__capabilities_get::read_response(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__capabilities_get, read_response)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__capabilities_get * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	size_t ret = 0;

	DBG_ENTER("mysqlx_message__capabilities_get::read_response");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOO",
												&object_zv, mysqlx_message__capabilities_get_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__CAPABILITIES_GET__FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	RETVAL_FALSE;

	ret = xmysqlnd_read__capabilities_get(return_value, connection->vio, codec->pfc, connection->stats, connection->error_info);
	if (FAIL == ret) {
		mysqlx_new_message__error(return_value, connection->error_info->error, connection->error_info->sqlstate, connection->error_info->error_no);
	}
	
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__capabilities_get_methods[] */
static const zend_function_entry mysqlx_message__capabilities_get_methods[] = {
	PHP_ME(mysqlx_message__capabilities_get, send,				mysqlx_message__capabilities_get__send,				ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_message__capabilities_get, read_response,		mysqlx_message__capabilities_get__read_response,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__capabilities_get_handlers;
static HashTable mysqlx_message__capabilities_get_properties;


/* {{{ mysqlx_message__capabilities_get_free_storage */
static void
mysqlx_message__capabilities_get_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__capabilities_get * message = (struct st_mysqlx_message__capabilities_get  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_message__capabilities_get_object_allocator */
static zend_object *
php_mysqlx_message__capabilities_get_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_message__capabilities_get * message = new (std::nothrow) struct st_mysqlx_message__capabilities_get;

	DBG_ENTER("php_mysqlx_message__capabilities_get_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_message__capabilities_get_handlers;
	mysqlx_object->properties = &mysqlx_message__capabilities_get_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_message__capabilities_get_class */
extern "C" void
mysqlx_register_message__capabilities_get_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__capabilities_get_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__capabilities_get_handlers.free_obj = mysqlx_message__capabilities_get_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__capabilities_get", mysqlx_message__capabilities_get_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "node_pfc", mysqlx_message__capabilities_get_methods);
		tmp_ce.create_object = php_mysqlx_message__capabilities_get_object_allocator;
		mysqlx_message__capabilities_get_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__capabilities_get_properties, 0, NULL, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__capabilities_get_class */
extern "C" void
mysqlx_unregister_message__capabilities_get_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__capabilities_get_properties);
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
