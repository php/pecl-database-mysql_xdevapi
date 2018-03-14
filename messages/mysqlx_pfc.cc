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
  | Authors: Andrey Hristov <andrey@php.net>                             |
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
#include "xmysqlnd/xmysqlnd_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "mysqlx_connection.h"
#include "mysqlx_pfc.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_pfc_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_pfc__send, 0, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 1)
	ZEND_ARG_TYPE_INFO(0, message_type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_pfc__receive, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, node_connection, IS_OBJECT, 1)
ZEND_END_ARG_INFO()


/* {{{ proto bool mysqlx_pfc::send(object pfc, object connection, long packet_type, string payload) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_pfc, send)
{
	zval* codec_zv{nullptr};
	zval* connection_zv{nullptr};
	st_mysqlx_connection* connection{nullptr};
	st_mysqlx_pfc* codec{nullptr};
	MYSQLND_CSTRING payload = {nullptr, 0};
	zend_ulong packet_type;
	size_t bytes_sent;
	enum_func_status ret{FAIL};

	DBG_ENTER("mysqlx_pfc::send");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OOls",
												&codec_zv, mysqlx_pfc_class_entry,
												&connection_zv, mysqlx_connection_class_entry,
												&packet_type,
												&(payload.s), &(payload.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);
	ret = codec->pfc->data->m.send(codec->pfc, connection->vio,
								   (zend_uchar) packet_type,
								   (const zend_uchar*) payload.s, payload.l,
								   &bytes_sent,
								   connection->stats,
								   connection->error_info);

	RETVAL_BOOL(ret == PASS);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_pfc::receive(object pfc, object connection) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_pfc, receive)
{
	zval* codec_zv{nullptr};
	zval* connection_zv{nullptr};
	st_mysqlx_connection* connection{nullptr};
	st_mysqlx_pfc* codec{nullptr};

	DBG_ENTER("mysqlx_pfc::receive");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OO",
												&codec_zv, mysqlx_pfc_class_entry,
												&connection_zv, mysqlx_connection_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_PFC_FROM_ZVAL(codec, codec_zv);
	MYSQLX_FETCH_NODE_CONNECTION_FROM_ZVAL(connection, connection_zv);
	{
		size_t count;
		zend_uchar packet_type;
		zend_uchar* read_buffer{nullptr};
		if (PASS == codec->pfc->data->m.receive(codec->pfc, connection->vio,
												nullptr, 0, /* prealloc buffer */
												&packet_type,
												&read_buffer,
												&count,
												connection->stats,
												connection->error_info))
		{
			array_init_size(return_value, 2);
			add_assoc_long_ex(return_value, "packet_type", sizeof("packet_type") - 1, (zend_ulong) packet_type);
			add_assoc_stringl_ex(return_value, "packet_type", sizeof("packet_type") - 1, (char*) read_buffer, count);
		}
		if (read_buffer) {
			mnd_efree(read_buffer);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_pfc_methods[] */
static const zend_function_entry mysqlx_pfc_methods[] = {
	PHP_ME(mysqlx_pfc, send,		arginfo_mysqlx_pfc__send,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_pfc, receive,	arginfo_mysqlx_pfc__receive,	ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_pfc_handlers;
static HashTable mysqlx_pfc_properties;


namespace {

/* {{{ mysqlx_pfc_free_storage */
void
mysqlx_pfc_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_pfc* codec = (st_mysqlx_pfc*) mysqlx_object->ptr;

	if (codec) {
		const zend_bool pers = codec->persistent;
		util::zend::free_error_info_list(codec->error_info, pers);
		xmysqlnd_pfc_free(codec->pfc, codec->stats, codec->error_info);
		mysqlnd_stats_end(codec->stats, pers);
		mnd_pefree(codec, pers);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_pfc_object_allocator */
zend_object*
php_mysqlx_pfc_object_allocator(zend_class_entry * class_type)
{
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory = MYSQLND_CLASS_METHODS_INSTANCE_NAME(xmysqlnd_object_factory);
	const zend_bool persistent = FALSE;
	st_mysqlx_object * mysqlx_object = static_cast<st_mysqlx_object*>(mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent));
	st_mysqlx_pfc * codec = static_cast<st_mysqlx_pfc*>(mnd_pecalloc(1, sizeof(struct st_mysqlx_pfc), persistent));

	DBG_ENTER("php_mysqlx_pfc_object_allocator");
	if ( mysqlx_object && codec ) {
		mysqlx_object->ptr = codec;

		if (PASS == mysqlnd_error_info_init(&codec->error_info_impl,

											persistent)) {
			codec->error_info = &codec->error_info_impl;
			mysqlnd_stats_init(&codec->stats, STAT_LAST, persistent);
			codec->pfc = xmysqlnd_pfc_create(persistent,
											 factory,
											 codec->stats,
											 codec->error_info);

			if ( nullptr != codec->pfc ) {
				codec->persistent = persistent;
				zend_object_std_init(&mysqlx_object->zo, class_type);
				object_properties_init(&mysqlx_object->zo, class_type);

				mysqlx_object->zo.handlers = &mysqlx_object_node_pfc_handlers;
				mysqlx_object->properties = &mysqlx_pfc_properties;

				DBG_RETURN(&mysqlx_object->zo);
			}
		}

	}
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	if (codec) {
		mnd_pefree(codec, persistent);
	}
	DBG_RETURN(nullptr);
}
/* }}} */

} // anonymous namespace

/* {{{ mysqlx_register_pfc_class */
void
mysqlx_register_pfc_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_pfc_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_pfc_handlers.free_obj = mysqlx_pfc_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "ProtocolFrameCodec", mysqlx_pfc_methods);
		tmp_ce.create_object = php_mysqlx_pfc_object_allocator;
		mysqlx_pfc_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_pfc_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_node_pfc_class */
void
mysqlx_unregister_node_pfc_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_pfc_properties);
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
