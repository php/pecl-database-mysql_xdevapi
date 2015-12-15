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
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_driver.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

static zend_class_entry *mysqlx_driver_class_entry;


#define MYSQLX_VERSION "v1.0.0"

/* {{{ mysqlx_driver_methods[] */
static const zend_function_entry mysqlx_driver_methods[] = {
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_driver_handlers;
static HashTable mysqlx_driver_properties;


/* {{{ mysqlx_driver_version_read */
static zval *
mysqlx_driver_version_read(const struct st_mysqlx_object * obj, zval * return_value)
{
	DBG_ENTER("mysqlx_driver_version_read");
	ZVAL_NEW_STR(return_value, strpprintf(0, "%s", MYSQLX_VERSION));
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_driver_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_driver_property_entries[] =
{
	{{"version",	sizeof("version") - 1},	mysqlx_driver_version_read, NULL},
	{{NULL, 		0},						NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_driver_free_storage */
static void
mysqlx_driver_free_storage(zend_object * object)
{
	mysqlx_object_free_storage(object); /* calling the free_storage of the parent - the zend_object */
}
/* }}} */


/* {{{ php_mysqlx_driver_object_allocator */
static zend_object *
php_mysqlx_driver_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));

	DBG_ENTER("php_mysqlx_driver_object_allocator");
	if (!mysqlx_object) {
		DBG_RETURN(NULL);
	}
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_driver_handlers;
	mysqlx_object->properties = &mysqlx_driver_properties;

	DBG_RETURN(&mysqlx_object->zo);
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
		tmp_ce.create_object = php_mysqlx_driver_object_allocator;
		mysqlx_driver_class_entry = zend_register_internal_class(&tmp_ce);
		mysqlx_driver_class_entry->ce_flags |= ZEND_ACC_FINAL; /* Forbid extension of the driver */
	}

	zend_hash_init(&mysqlx_driver_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_driver_properties, mysqlx_driver_property_entries);
	/*
	  Now register the properties, per name, to the class_entry. When someone uses this
	  name from PHP then PHP will call read_property/write_property/has_property,
	  and then we will look into the array initialized above (in this case
	  mysqlx_node_session_properties), to find the proper getter/setter for the
	  specific property. Finally we execute the getter/setter.
	*/
	zend_declare_property_null(mysqlx_driver_class_entry, "driver_version", sizeof("driver_version") - 1,		ZEND_ACC_PUBLIC);
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
