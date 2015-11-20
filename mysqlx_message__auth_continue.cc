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
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_alloc.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "xmysqlnd.h"
#include "xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"
#include "mysqlx_message__capability.h"
#include "mysqlx_message__capabilities.h"

#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_zval2any.h"

#include <new>
#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_session.pb.h"

#include "mysqlx_message__ok.h"
#include "mysqlx_message__error.h"
#include "mysqlx_message__auth_continue.h"
#include "mysqlx_message__auth_ok.h"

extern "C"
{
#include "ext/mysqlnd/mysqlnd_auth.h" /* php_mysqlnd_scramble */
}

static zend_class_entry *mysqlx_message__auth_continue_class_entry;

struct st_mysqlx_message__auth_continue
{
	Mysqlx::Session::AuthenticateContinue message;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__AUTH_CONTINUE_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_message__auth_continue *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__auth_continue__send, 0, ZEND_RETURN_VALUE, 5)
	ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, schema, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(mysqlx_message__auth_continue__read_response, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, node_pfc, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


static const char hexconvtab[] = "0123456789abcdef";

static size_t
mysqlx_auth_continue_send(const char * schema, size_t schema_len,
						  const char * user, size_t user_len,
						  const char * password, size_t password_len,
						  const char * salt, size_t salt_len,
						  struct st_mysqlx_node_connection * connection,
						  struct st_mysqlx_node_pfc * codec)
{
	Mysqlx::Session::AuthenticateContinue proto_message;
	DBG_ENTER("mysqlx_auth_continue_send");

	std::string response(schema, schema_len);
	response.append(1, '\0');
	response.append(user, user_len);
	response.append(1, '\0'); 
	if (password && password_len) {
		zend_uchar hash[SCRAMBLE_LENGTH];

		php_mysqlnd_scramble(hash, (zend_uchar*) salt, (const zend_uchar*) password, password_len);
		char hexed_hash[SCRAMBLE_LENGTH*2];
		for (unsigned int i = 0; i < SCRAMBLE_LENGTH; i++) {
			hexed_hash[i*2] = hexconvtab[hash[i] >> 4];
			hexed_hash[i*2 + 1] = hexconvtab[hash[i] & 15];
		}
		DBG_INF_FMT("hexed_hash=%s", hexed_hash);
		response.append(1, '*');
		response.append(hexed_hash, SCRAMBLE_LENGTH*2);
	}
	response.append(1, '\0');
	DBG_INF_FMT("response_size=%u", (uint) response.size());
	proto_message.set_auth_data(response.c_str(), response.size());

	DBG_RETURN(xmysqlnd_send_protobuf_message(connection, codec, Mysqlx::ClientMessages_Type_SESS_AUTHENTICATE_CONTINUE, proto_message, false));
}
/* }}} */


/* {{{ proto long mysqlx_message__auth_continue::send(object messsage, string user, string password, string schema, object pfc, object connection) */
PHP_METHOD(mysqlx_message__auth_continue, send)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__auth_continue * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	char * user = NULL;
	size_t user_len = 0;
	char * password = NULL;
	size_t password_len = 0;
	char * schema = NULL;
	size_t schema_len = 0;

	DBG_ENTER("mysqlx_message__auth_continue::send");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OsssOO",
												&object_zv, mysqlx_message__auth_continue_class_entry,
												&user, &user_len,
												&password, &password_len,
												&schema, &schema_len,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__AUTH_CONTINUE_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	if (!object->message.has_auth_data()) {
		php_error_docref(NULL, E_WARNING, "No authentication data from the server");
		DBG_VOID_RETURN;
	}

	RETVAL_LONG(mysqlx_auth_continue_send(schema, schema_len, user, user_len, password, password_len,
										  object->message.auth_data().c_str(), object->message.auth_data().size(),
										  connection, codec));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_message__auth_continue::read_response(object messsage, object pfc, object connection) */
PHP_METHOD(mysqlx_message__auth_continue, read_response)
{
	zval * object_zv;
	zval * codec_zv;
	zval * connection_zv;
	struct st_mysqlx_message__auth_continue * object;
	struct st_mysqlx_node_connection * connection;
	struct st_mysqlx_node_pfc * codec;
	size_t ret = 0;

	DBG_ENTER("mysqlx_message__auth_continue::read_response");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOO",
												&object_zv, mysqlx_message__auth_continue_class_entry,
												&codec_zv, mysqlx_node_pfc_class_entry,
												&connection_zv, mysqlx_node_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__AUTH_CONTINUE_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);

	RETVAL_FALSE;
	{
		zend_uchar packet_type;
		size_t payload_size;
		zend_uchar * payload;
		do {
			ret = codec->pfc->data->m.receive(codec->pfc, connection->vio,
											  &packet_type,
											  &payload, &payload_size,
											  connection->stats,
											  connection->error_info);
			if (ret == PASS) {
				const Mysqlx::ServerMessages_Type type = (Mysqlx::ServerMessages_Type)(packet_type);
				switch (type) {
					case Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_CONTINUE: {
						Mysqlx::Session::AuthenticateContinue auth_continue;
						auth_continue.ParseFromArray(payload, payload_size);
						mysqlx_new_message__auth_continue(return_value, auth_continue);
						break;
					}
					case Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_OK: {
						Mysqlx::Session::AuthenticateOk auth_ok;
						auth_ok.ParseFromArray(payload, payload_size);
						mysqlx_new_message__auth_ok(return_value, auth_ok);
						break;
					}
					case Mysqlx::ServerMessages_Type_ERROR: {
						Mysqlx::Error error;
						error.ParseFromArray(payload, payload_size);
						mysqlx_new_message__error(return_value, error);
						break;
					}
					case Mysqlx::ServerMessages_Type_NOTICE:
						break;
				default:
					php_error_docref(NULL, E_WARNING, "Returned unexpected packet type %s", Mysqlx::ServerMessages_Type_Name(type).c_str());
				}
				mnd_efree(payload);
			}
		} while (packet_type == Mysqlx::ServerMessages_Type_NOTICE);
	}
	
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_message__auth_continue_methods[] */
static const zend_function_entry mysqlx_message__auth_continue_methods[] = {
	PHP_ME(mysqlx_message__auth_continue, send,				mysqlx_message__auth_continue__send,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_message__auth_continue, read_response,	mysqlx_message__auth_continue__read_response,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_message__auth_continue_handlers;
static HashTable mysqlx_message__auth_continue_properties;

/* {{{ mysqlx_message__auth_continue_free_storage */
static void
mysqlx_message__auth_continue_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_message__auth_continue * message = (struct st_mysqlx_message__auth_continue  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_message__auth_continue_object_allocator */
static zend_object *
php_mysqlx_message__auth_continue_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_message__auth_continue * message = new (std::nothrow) struct st_mysqlx_message__auth_continue();

	DBG_ENTER("php_mysqlx_message__auth_continue_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_message__auth_continue_handlers;
	mysqlx_object->properties = &mysqlx_message__auth_continue_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_message__auth_continue_class */
extern "C" void
mysqlx_register_message__auth_continue_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_message__auth_continue_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_message__auth_continue_handlers.free_obj = mysqlx_message__auth_continue_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_message__auth_continue", mysqlx_message__auth_continue_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "node_pfc", mysqlx_message__auth_continue_methods);
		tmp_ce.create_object = php_mysqlx_message__auth_continue_object_allocator;
		mysqlx_message__auth_continue_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_message__auth_continue_properties, 0, NULL, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_message__auth_continue_class */
extern "C" void
mysqlx_unregister_message__auth_continue_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_message__auth_continue_properties);
}
/* }}} */


/* {{{ mysqlx_new_message__auth_continue */
void
mysqlx_new_message__auth_continue(zval * return_value, const Mysqlx::Session::AuthenticateContinue & message)
{
	struct st_mysqlx_message__auth_continue * obj;
	DBG_ENTER("mysqlx_new_message__auth_continue")
	object_init_ex(return_value, mysqlx_message__auth_continue_class_entry);
	MYSQLX_FETCH_MESSAGE__AUTH_CONTINUE_FROM_ZVAL(obj, return_value);
	obj->message.CopyFrom(message);
	DBG_VOID_RETURN;
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
