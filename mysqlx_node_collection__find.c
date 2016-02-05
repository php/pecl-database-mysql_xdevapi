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
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_node_collection__find.h"

static zend_class_entry *mysqlx_node_collection__find_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__values, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_collection__find
{
	XMYSQLND_NODE_COLLECTION * collection;
};


#define MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_collection__find::__construct */
static
PHP_METHOD(mysqlx_node_collection__find, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
static
PHP_METHOD(mysqlx_node_collection__find, execute)
{
	struct st_mysqlx_node_collection__find * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_collection__find::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_collection__find_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->collection) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__find_methods[] */
static const zend_function_entry mysqlx_node_collection__find_methods[] = {
	PHP_ME(mysqlx_node_collection__find, 	__construct,	NULL,												ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__find,	execute,		arginfo_mysqlx_node_collection__find__execute,	ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

#if 0
/* {{{ mysqlx_node_collection__find_property__name */
static zval *
mysqlx_node_collection__find_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_collection__find * object = (const struct st_mysqlx_node_collection__find *) (obj->ptr);
	DBG_ENTER("mysqlx_node_collection__find_property__name");
	if (object->collection && object->collection->data->collection_name.s) {
		ZVAL_STRINGL(return_value, object->collection->data->collection_name.s, object->collection->data->collection_name.l);
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
#endif

static zend_object_handlers mysqlx_object_node_collection__find_handlers;
static HashTable mysqlx_node_collection__find_properties;

const struct st_mysqlx_property_entry mysqlx_node_collection__find_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_node_collection__find_property__name,	NULL},
#endif
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection__find_free_storage */
static void
mysqlx_node_collection__find_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_collection__find * inner_obj = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_node_collection_free(inner_obj->collection, NULL, NULL);
			inner_obj->collection = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_collection__find_object_allocator */
static zend_object *
php_mysqlx_node_collection__find_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_collection__find * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_collection__find));

	DBG_ENTER("php_mysqlx_node_collection__find_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_collection__find_handlers;
	mysqlx_object->properties = &mysqlx_node_collection__find_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__find_class */
void
mysqlx_register_node_collection__find_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_collection__find_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_collection__find_handlers.free_obj = mysqlx_node_collection__find_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeCollectionFind", mysqlx_node_collection__find_methods);
		tmp_ce.create_object = php_mysqlx_node_collection__find_object_allocator;
		mysqlx_node_collection__find_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_collection__find_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_collection__find_properties, mysqlx_node_collection__find_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_collection__find_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__find_class */
void
mysqlx_unregister_node_collection__find_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_collection__find_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__find */
void
mysqlx_new_node_collection__find(zval * return_value, XMYSQLND_NODE_COLLECTION * collection, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_node_collection__find");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_collection__find_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_collection__find * const object = (struct st_mysqlx_node_collection__find *) mysqlx_object->ptr;
		if (object) {
			object->collection = clone? collection->data->m.get_reference(collection) : collection;
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

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
