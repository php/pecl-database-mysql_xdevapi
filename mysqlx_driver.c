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
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

static zend_class_entry *mysqlx_driver_class_entry;


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_driver__version, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ proto mixed mysqlx_driver::version(object session) */
PHP_METHOD(mysqlx_driver, version)
{
	zval *node_session;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &node_session, mysqlx_driver_class_entry) == FAILURE) {
		return;
	}

	RETURN_STRING("v1.0.0");
}
/* }}} */



/* {{{ mysqlx_driver_methods[] */
const zend_function_entry mysqlx_driver_methods[] = {
	PHP_ME(mysqlx_driver, version, arginfo_mysqlx_driver__version, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_driver_handlers;
static HashTable mysqlx_driver_properties;

static const struct st_mysqlx_property_entry mysqlx_driver_property_entries[] =
{
#ifdef USE_PROPERTIES
	{{"server_version",	sizeof("driver_version") - 1},	link_server_version_read, NULL},
#endif
	{{NULL, 			0},								NULL, NULL}
};

/* {{{ mysqlx_driver_free_storage */
static void
mysqlx_driver_free_storage(zend_object * object)
{
	mysqlx_object_free_storage(object); /* calling the free_storage of the parent - the zend_object */
}
/* }}} */


/* {{{ mysqlx_register_driver_class */
void
mysqlx_register_driver_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_driver_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_driver_handlers.free_obj = mysqlx_driver_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_driver", mysqlx_driver_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "driver", mysqlx_node_session_methods);
		mysqlx_driver_class_entry = zend_register_internal_class(&tmp_ce);
		mysqlx_driver_class_entry->ce_flags |= ZEND_ACC_FINAL; /* Forbid extension of the driver */
	}

	zend_hash_init(&mysqlx_driver_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_driver_properties, mysqlx_driver_property_entries);
#ifdef USE_PROPERTIES
	/*
	  Now register the properties, per name, to the class_entry. When someone uses this
	  name from PHP then PHP will call read_property/write_property/has_property,
	  and then we will look into the array initialized above (in this case
	  mysqlx_node_session_properties), to find the proper getter/setter for the
	  specific property. Finally we execute the getter/setter.
	*/
	zend_declare_property_null(mysqlx_driver_class_entry, "client_info", 		sizeof("client_info") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_driver_class_entry, "client_version", 	sizeof("client_version") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_driver_class */
void
mysqlx_unregister_driver_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_driver_properties);
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
