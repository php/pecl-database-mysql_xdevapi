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
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_environment.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "mysqlx_connection.h"
#include "util/object.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_connection_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_connection__connect, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(0, hostname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_connection__send, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, payload, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_connection__receive, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, how_many, IS_LONG, 0)
ZEND_END_ARG_INFO()


/* {{{ get_scheme */
static MYSQLND_STRING
get_scheme(
	MYSQLND_CSTRING hostname,
	MYSQLND_CSTRING socket_or_pipe,
	zend_long port,
	zend_bool * /*unix_socket*/,
	zend_bool * named_pipe)
{
	MYSQLND_STRING transport;
	DBG_ENTER("get_scheme");
#ifdef PHP_WIN32
	if (hostname.l == sizeof(".") - 1 && hostname.s[0] == '.') {
		/* named pipe in socket */
		if (!socket_or_pipe.s) {
			socket_or_pipe.s = "\\\\.\\pipe\\MySQL";
			socket_or_pipe.l = strlen(socket_or_pipe.s);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "pipe://%s", socket_or_pipe.s);
		*named_pipe = TRUE;
	}
	else
#endif
	{
		if (!port) {
			port = drv::Environment::get_as_int(drv::Environment::Variable::Mysql_port);
		}
		transport.l = mnd_sprintf(&transport.s, 0, "tcp://%s:%u",
			hostname.s, static_cast<unsigned int>(port));
	}
	DBG_INF_FMT("transport=%s", transport.s? transport.s:"OOM");
	DBG_RETURN(transport);
}
/* }}} */


/* {{{ proto bool mysqlx_connection::connect(object connection, string hostname, string username, string password) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_connection, connect)
{
	zval* connection_zv{nullptr};
	st_mysqlx_connection* connection{nullptr};
	MYSQLND_CSTRING hostname = {nullptr, 0};
	MYSQLND_CSTRING socket_or_pipe = {nullptr, 0};
	zend_long port = drv::Environment::get_as_int(drv::Environment::Variable::Mysqlx_port);
	enum_func_status ret{FAIL};

	DBG_ENTER("mysqlx_connection::connect");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Os|l",
												&connection_zv, mysqlx_connection_class_entry,
												&(hostname.s), &(hostname.l),
												&port))
	{
		DBG_VOID_RETURN;
	}

	if (!hostname.s || !hostname.l) {
		hostname.s = "127.0.0.1";
		hostname.l = sizeof("127.0.0.1") - 1;
	}

	MYSQLX_FETCH_CONNECTION_FROM_ZVAL(connection, connection_zv);
	if (connection->vio) {
		zend_bool not_needed;
		MYSQLND_STRING transport = get_scheme(hostname, socket_or_pipe, port, &not_needed, &not_needed);
		const MYSQLND_CSTRING scheme = { transport.s, transport.l };

		if (TRUE == connection->vio->data->m.has_valid_stream(connection->vio)) {
			connection->vio->data->m.close_stream(connection->vio, connection->stats, connection->error_info);
		}

		ret = connection->vio->data->m.connect(connection->vio,
											   scheme,
											   connection->persistent,
											   connection->stats,
											   connection->error_info);
		if (transport.s) {
			mnd_sprintf_free(transport.s);
			transport.s = nullptr;
		}
	}
	RETVAL_BOOL(ret == PASS);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_connection::send(object session, string payload) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_connection, send)
{
	zval* connection_zv{nullptr};
	st_mysqlx_connection* connection{nullptr};
	MYSQLND_CSTRING payload = {nullptr, 0};
	size_t ret{0};

	DBG_ENTER("mysqlx_connection::send");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Os",
												&connection_zv, mysqlx_connection_class_entry,
												&(payload.s), &(payload.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_CONNECTION_FROM_ZVAL(connection, connection_zv);
	if (!connection->vio || FALSE == connection->vio->data->m.has_valid_stream(connection->vio)) {
		DBG_VOID_RETURN;
	}
	if (payload.s && payload.l) {
		ret = connection->vio->data->m.network_write(connection->vio,
													 (const zend_uchar*) payload.s, payload.l,
													 connection->stats,
													 connection->error_info);
	}
	RETVAL_LONG(ret);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto long mysqlx_connection::receive(object connection, long bytes) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_connection, receive)
{
	zval* connection_zv{nullptr};
	st_mysqlx_connection* connection{nullptr};
	zend_ulong how_many{0};
	enum_func_status ret;

	DBG_ENTER("mysqlx_connection::receive");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
												&connection_zv, mysqlx_connection_class_entry,
												&how_many))
	{
		DBG_VOID_RETURN;
	}
	if (!how_many) {
		RETVAL_TRUE;
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_CONNECTION_FROM_ZVAL(connection, connection_zv);
	if (connection->vio && TRUE == connection->vio->data->m.has_valid_stream(connection->vio)) {
		zend_uchar * read_buffer = static_cast<zend_uchar*>(mnd_emalloc(how_many + 1));
		if (!read_buffer) {
			const auto& how_many_str{ util::to_string(how_many) };
			php_error_docref(nullptr, E_WARNING, "Couldn't allocate %s bytes", how_many_str.c_str());
			RETVAL_FALSE;
		}
		ret = connection->vio->data->m.network_read(connection->vio,
													read_buffer, how_many,
													connection->stats,
													connection->error_info);
		if (PASS == ret) {
			read_buffer[how_many] = '\0';
			RETVAL_STRINGL((char*) read_buffer, how_many);
		} else {
			const auto& how_many_str{ util::to_string(how_many) };
			php_error_docref(nullptr, E_WARNING, "Error reading %s bytes", how_many_str.c_str());
			RETVAL_FALSE;
		}
		mnd_efree(read_buffer);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_connection_methods[] */
static const zend_function_entry mysqlx_connection_methods[] = {
	PHP_ME(mysqlx_connection, connect,		arginfo_mysqlx_connection__connect,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_connection, send,		arginfo_mysqlx_connection__send,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_connection, receive,		arginfo_mysqlx_connection__receive,	ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_connection_handlers;
static HashTable mysqlx_connection_properties;


/* {{{ mysqlx_connection_free_storage */
static void
mysqlx_connection_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_connection* connection = (st_mysqlx_connection*) mysqlx_object->ptr;
	if (connection) {
		const zend_bool pers = connection->persistent;
		util::zend::free_error_info_list(connection->error_info, pers);
		mysqlnd_vio_free(connection->vio, connection->stats, connection->error_info);
		mysqlnd_stats_end(connection->stats, pers);
		mnd_pefree(connection, pers);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_connection_object_allocator */
static zend_object *
php_mysqlx_connection_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	const std::size_t bytes_count = sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type);
	st_mysqlx_object* mysqlx_object = static_cast<st_mysqlx_object*>(::operator new(bytes_count, util::permanent_tag));
	st_mysqlx_connection * connection = new st_mysqlx_connection();

	DBG_ENTER("php_mysqlx_connection_object_allocator");

	if (mysqlx_object && connection) {

		mysqlx_object->ptr = connection;
		if (PASS == mysqlnd_error_info_init(&connection->error_info_impl,
											persistent)) {

			connection->error_info = &connection->error_info_impl;
			mysqlnd_stats_init(&connection->stats, STAT_LAST, persistent);

			if (nullptr != (connection->vio = mysqlnd_vio_init(persistent,
															nullptr /*factory*/,
															connection->stats,
															connection->error_info))) {
				connection->persistent = persistent;
				zend_object_std_init(&mysqlx_object->zo, class_type);
				object_properties_init(&mysqlx_object->zo, class_type);

				mysqlx_object->zo.handlers = &mysqlx_object_connection_handlers;
				mysqlx_object->properties = &mysqlx_connection_properties;

				DBG_RETURN(&mysqlx_object->zo);
			}
		}
	}

	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	if (connection) {
		mnd_pefree(connection, persistent);
	}
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_connection_class */
void
mysqlx_register_connection_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_connection_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_connection_handlers.free_obj = mysqlx_connection_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NativeConnection", mysqlx_connection_methods);
		tmp_ce.create_object = php_mysqlx_connection_object_allocator;
		mysqlx_connection_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_connection_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_connection_class */
void
mysqlx_unregister_connection_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_connection_properties);
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
