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
#include <xmysqlnd/xmysqlnd_wireprotocol.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"
#include "mysqlx_message__capability.h"
#include "mysqlx_message__capabilities.h"


#include <new>
#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"

#include "mysqlx_message__capabilities_get.h"
#include "mysqlx_message__ok.h"
#include "mysqlx_message__error.h"


zend_class_entry *mysqlx_message__capabilities_set_class_entry;

struct st_mysqlx_message__capabilities_set
{
	Mysqlx::Connection::Capabilities response;
	Mysqlx::Error error;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__CAPABILITIES_SET_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_message__capabilities_set *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__capabilities_set__send, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, capabilities, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__capabilities_set__read_response, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


/* {{{ proto long mysqlx_message__capabilities_set::send(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__capabilities_set, send)
{
	zval * message_zv;
	zval * codec_zv;
	zval * connection_zv;
	zval * capabilities_zv;
	struct st_mysqlx_message__capabilities * capabilities;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	size_t ret = 0;

	DBG_ENTER("mysqlx_message__capabilities_set::send");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOOO",
												&message_zv, mysqlx_message__capabilities_set_class_entry,
												&capabilities_zv, mysqlx_message__capabilities_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__CAPABILITIES_FROM_ZVAL(capabilities, capabilities_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	Mysqlx::Connection::CapabilitiesSet proto_message;
	{
		zval * entry;
		size_t cap_count = zend_hash_num_elements(&capabilities->capabilities_ht);
		if (!cap_count) {
			php_error_docref(NULL, E_WARNING, "Zero Capabilities");
			DBG_VOID_RETURN;
		}

		ZEND_HASH_FOREACH_VAL(&capabilities->capabilities_ht, entry) {
			if (Z_TYPE_P(entry) == IS_OBJECT && Z_OBJ_P(entry)->ce == mysqlx_message__capability_class_entry) {
				struct st_mysqlx_message__capability * capability_entry = NULL;
				MYSQLX_FETCH_MESSAGE__CAPABILITY_FROM_ZVAL(capability_entry, entry);
				{
					Mysqlx::Connection::Capability* new_object = proto_message.mutable_capabilities()->add_capabilities();
					new_object->set_name(Z_STRVAL(capability_entry->capability_name), Z_STRLEN(capability_entry->capability_name));
					Mysqlx::Datatypes::Any any_entry;
					zval2any(&capability_entry->capability_value, any_entry);
					new_object->mutable_value()->CopyFrom(any_entry);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
	RETVAL_LONG(xmysqlnd_send_protobuf_message(connection, codec, Mysqlx::ClientMessages_Type_CON_CAPABILITIES_SET, proto_message));

	RETVAL_LONG(ret);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_message__capabilities_set::read_response(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__capabilities_set, read_response)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__capabilities_set * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	size_t ret = 0;

	DBG_ENTER("mysqlx_message__capabilities_set::read_response");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOO",
												&object_zv, mysqlx_message__capabilities_set_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__CAPABILITIES_SET_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	RETVAL_FALSE;
	{
		Mysqlx::Connection::Capabilities & proto_message = object->response;
		zend_uchar packet_type;
		size_t payload_size;
		zend_uchar * payload;
		do {
			ret = codec->pfc->data->m.receive(codec->pfc, connection->vio,
											  &packet_type,
											  &payload, &payload_size,
											  connection->stats,
											  connection->error_info);
			if (ret == FAIL) {
				break;
			}
			const Mysqlx::ServerMessages_Type type = (Mysqlx::ServerMessages_Type)(packet_type);
			switch (type) {
				case Mysqlx::ServerMessages_Type_OK: {
					Mysqlx::Ok ok_response;
					ok_response.ParseFromArray(payload, payload_size);
					mysqlx_new_message__ok(return_value, ok_response);
					break;
				}
				case Mysqlx::ServerMessages_Type_ERROR: {
					Mysqlx::Error error;
					error.ParseFromArray(payload, payload_size);
					mysqlx_new_message__error(return_value, error);
					dump_mysqlx_error(error);
					break;
				}
				case Mysqlx::ServerMessages_Type_NOTICE:
					break;
				default:
					php_error_docref(NULL, E_WARNING, "Returned unexpected packet type %s", Mysqlx::ServerMessages_Type_Name(type).c_str());
			}
			mnd_efree(payload);
		} while (packet_type == Mysqlx::ServerMessages_Type_NOTICE);
	}
	
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__capabilities_set_methods[] */
static const zend_function_entry mysqlx_message__capabilities_set_methods[] = {
	PHP_ME(mysqlx_message__capabilities_set, send,				mysqlx_message__capabilities_set__send,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_message__capabilities_set, read_response,		mysqlx_message__capabilities_set__read_response,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__capabilities_set_handlers;
static HashTable mysqlx_message__capabilities_set_properties;

/* {{{ mysqlx_message__capabilities_set_free_storage */
static void
mysqlx_message__capabilities_set_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__capabilities_set * message = (struct st_mysqlx_message__capabilities_set  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_message__capabilities_set_object_allocator */
static zend_object *
php_mysqlx_message__capabilities_set_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_message__capabilities_set * message = new (std::nothrow) struct st_mysqlx_message__capabilities_set();

	DBG_ENTER("php_mysqlx_message__capabilities_set_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_message__capabilities_set_handlers;
	mysqlx_object->properties = &mysqlx_message__capabilities_set_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_message__capabilities_set_class */
extern "C" void
mysqlx_register_message__capabilities_set_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__capabilities_set_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__capabilities_set_handlers.free_obj = mysqlx_message__capabilities_set_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__capabilities_set", mysqlx_message__capabilities_set_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "node_pfc", mysqlx_message__capabilities_set_methods);
		tmp_ce.create_object = php_mysqlx_message__capabilities_set_object_allocator;
		mysqlx_message__capabilities_set_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__capabilities_set_properties, 0, NULL, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__capabilities_set_class */
extern "C" void
mysqlx_unregister_message__capabilities_set_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__capabilities_set_properties);
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
