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
#include <xmysqlnd/xmysqlnd_node_table.h>
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_node_table__delete.h"
#include "mysqlx_node_table__insert.h"
#include "mysqlx_node_table__select.h"
#include "mysqlx_node_table__update.h"
#include "mysqlx_node_table.h"

static zend_class_entry *mysqlx_node_table_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0


/************************************** INHERITED START ****************************************/
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__drop, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()



ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__get_schema, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
/************************************** INHERITED END   ****************************************/

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__select, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__insert, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__update, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_table__delete, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_table
{
	XMYSQLND_NODE_TABLE * table;
};


#define MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_table *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_node_table::__construct */
static
PHP_METHOD(mysqlx_node_table, __construct)
{
}
/* }}} */


/************************************** INHERITED START ****************************************/
/* {{{ proto mixed mysqlx_node_table::getSession() */
static
PHP_METHOD(mysqlx_node_table, getSession)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::getSession");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::getName() */
static
PHP_METHOD(mysqlx_node_table, getName)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::getName");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	if (object->table) {
		RETVAL_STRINGL(object->table->data->table_name.s, object->table->data->table_name.l);
	} else {
		RETVAL_FALSE;	
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::existsInDatabase() */
static
PHP_METHOD(mysqlx_node_table, existsInDatabase)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::existsInDatabase");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::drop() */
static
PHP_METHOD(mysqlx_node_table, drop)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::existsInDatabase");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {

	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::getSchema() */
static
PHP_METHOD(mysqlx_node_table, getSchema)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::getSchema");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {

	}

	DBG_VOID_RETURN;
}
/* }}} */
/************************************** INHERITED END   ****************************************/


/* {{{ proto mixed mysqlx_node_table::insert() */
static
PHP_METHOD(mysqlx_node_table, insert)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::insert");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

//	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {
		mysqlx_new_node_table__insert(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::select() */
static
PHP_METHOD(mysqlx_node_table, select)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::select");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

//	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {
		mysqlx_new_node_table__select(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::update() */
static
PHP_METHOD(mysqlx_node_table, update)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::update");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

//	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {
		mysqlx_new_node_table__update(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_table::delete() */
static
PHP_METHOD(mysqlx_node_table, delete)
{
	struct st_mysqlx_node_table * object;
	zval * object_zv;

	DBG_ENTER("mysqlx_node_table::delete");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_node_table_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

//	zend_throw_exception(zend_ce_exception, "Not Implemented", 0);

	if (object->table) {
		mysqlx_new_node_table__delete(return_value, object->table, TRUE /* clone */);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_table_methods[] */
static const zend_function_entry mysqlx_node_table_methods[] = {
	PHP_ME(mysqlx_node_table, __construct,		NULL,											ZEND_ACC_PRIVATE)
	/************************************** INHERITED START ****************************************/
	PHP_ME(mysqlx_node_table, getSession,		arginfo_mysqlx_node_table__get_session,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, getName,			arginfo_mysqlx_node_table__get_name,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, existsInDatabase,	arginfo_mysqlx_node_table__exists_in_database,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, drop,				arginfo_mysqlx_node_table__drop,				ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_node_table, getSchema,		arginfo_mysqlx_node_table__get_schema,			ZEND_ACC_PUBLIC)
	/************************************** INHERITED END   ****************************************/

	PHP_ME(mysqlx_node_table, insert, arginfo_mysqlx_node_table__insert, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, select, arginfo_mysqlx_node_table__select, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, update, arginfo_mysqlx_node_table__update, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_table, delete, arginfo_mysqlx_node_table__delete, ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_node_table_property__name */
static zval *
mysqlx_node_table_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_table * object = (const struct st_mysqlx_node_table *) (obj->ptr);
	DBG_ENTER("mysqlx_node_table_property__name");
	if (object->table && object->table->data->table_name.s) {
		ZVAL_STRINGL(return_value, object->table->data->table_name.s, object->table->data->table_name.l);
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


static zend_object_handlers mysqlx_object_node_table_handlers;
static HashTable mysqlx_node_table_properties;

const struct st_mysqlx_property_entry mysqlx_node_table_property_entries[] =
{
	{{"name",	sizeof("name") - 1}, mysqlx_node_table_property__name,	NULL},
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_table_free_storage */
static void
mysqlx_node_table_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_table * inner_obj = (struct st_mysqlx_node_table *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_node_table_free(inner_obj->table, NULL, NULL);
			inner_obj->table = NULL;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_node_table_object_allocator */
static zend_object *
php_mysqlx_node_table_object_allocator(zend_class_entry * class_type)
{
	struct st_mysqlx_object * mysqlx_object = mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	struct st_mysqlx_node_table * object = mnd_ecalloc(1, sizeof(struct st_mysqlx_node_table));

	DBG_ENTER("php_mysqlx_node_table_object_allocator");
	if (!mysqlx_object || !object) {
		DBG_RETURN(NULL);	
	}
	mysqlx_object->ptr = object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_node_table_handlers;
	mysqlx_object->properties = &mysqlx_node_table_properties;


	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_table_class */
void
mysqlx_register_node_table_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_table_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_table_handlers.free_obj = mysqlx_node_table_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "Mysqlx", "NodeTable", mysqlx_node_table_methods);
		tmp_ce.create_object = php_mysqlx_node_table_object_allocator;
		mysqlx_node_table_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_table_class_entry, 1, mysqlx_schema_object_interface_entry);
	}

	zend_hash_init(&mysqlx_node_table_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_table_properties, mysqlx_node_table_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_table_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_node_table_class */
void
mysqlx_unregister_node_table_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_table_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_table */
void
mysqlx_new_node_table(zval * return_value, XMYSQLND_NODE_TABLE * table, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_node_table");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_table_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_table * const object = (struct st_mysqlx_node_table *) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->data->m.get_reference(table) : table;
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
