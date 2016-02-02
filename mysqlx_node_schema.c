/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#include <php.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_schema.h"

static zend_class_entry *mysqlx_node_schema_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_schema__get_collection, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(NO_PASS_BY_REF, name, IS_STRING, DONT_ALLOW_NULL)
ZEND_END_ARG_INFO()



struct st_mysqlx_node_schema
{
	XMYSQLND_NODE_SCHEMA * schema;
};


#define MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_schema *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->schema) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_schema::__construct */
static
PHP_METHOD(mysqlx_node_schema, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_schema::getCollection(string name) */
static
PHP_METHOD(mysqlx_node_schema, getCollection)
{
	struct st_mysqlx_node_schema * object;
	zval * object_zv;
	MYSQLND_CSTRING collection_name = { NULL, 0 };

	DBG_ENTER("mysqlx_node_schema::getCollection");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_schema_class_entry,
												&(collection_name.s), &(collection_name.l)))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_NODE_SCHEMA_FROM_ZVAL(object, object_zv);
	RETVAL_FALSE;
	if (collection_name.s && collection_name.l && object->schema) {
		struct st_xmysqlnd_node_collection * const collection = object->schema->data->m.create_collection_object(object->schema, collection_name);
		mysqlx_new_node_collection(return_value, collection);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_schema_methods[] */
static const zend_function_entry mysqlx_node_schema_methods[] = {
	PHP_ME(mysqlx_node_schema, __construct,		NULL,										ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_schema, getCollection,	arginfo_mysqlx_node_schema__get_collection,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_node_schema_property__name */
static zval *
mysqlx_node_schema_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_schema * object = (const struct st_mysqlx_node_schema *) (obj->ptr);
	DBG_ENTER("mysqlx_node_schema_property__name");
	if (object->schema && object->schema->data->schema_name.s) {
		ZVAL_STRINGL(return_value, object->schema->data->schema_name.s, object->schema->data->schema_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */


static zend_object_handlers mysqlx_object_node_schema_handlers;
static HashTable mysqlx_node_schema_properties;

const struct st_mysqlx_property_entry mysqlx_node_schema_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_node_schema_property__name,	NULL},
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_schema_free_storage */
static void
mysqlx_node_schema_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_schema * inner_obj = (struct st_mysqlx_node_schema *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->schema) {
			xmysqlnd_node_schema_free(inner_obj->schema, NULL, NULL);
			inner_obj->schema = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_schema_object_allocator */
static zend_object *
php_mysqlx_node_schema_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_schema * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_schema));

	DBG_ENTER("php_mysqlx_node_schema_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_schema_handlers;
	mysqlx_object->properties = &mysqlx_node_schema_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_schema_class */
void
mysqlx_register_node_schema_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_schema_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_schema_handlers.free_obj = mysqlx_node_schema_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeSchema", mysqlx_node_schema_methods);
		tmp_ce.create_object = php_mysqlx_node_schema_object_allocator;
		mysqlx_node_schema_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_schema_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_schema_properties, mysqlx_node_schema_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_schema_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_node_schema_class */
void
mysqlx_unregister_node_schema_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_schema_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_schema */
void
mysqlx_new_node_schema(zval * return_value, XMYSQLND_NODE_SCHEMA * schema)
{
	struct st_mysqlx_object * mysqlx_object;
	struct st_mysqlx_node_schema * object = NULL;
	DBG_ENTER("mysqlx_new_node_schema");

	object_init_ex(return_value, mysqlx_node_schema_class_entry);

	mysqlx_object = Z_MYSQLX_P(return_value);
	object = (struct st_mysqlx_node_schema *) mysqlx_object->ptr;
	if (!object) {
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		DBG_VOID_RETURN;
	}

	object->schema = schema;

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
