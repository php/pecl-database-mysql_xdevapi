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
#include "zend_smart_str.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_alloc.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "xmysqlnd.h"
#include "xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include <new>
#include "mysqlx_resultset__data_row.h"
#include "mysqlx_resultset__column_metadata.h"
#include "mysqlx_resultset__resultset_metadata.h"

static zend_class_entry *mysqlx_data_row_class_entry;

struct st_mysqlx_data_row
{
	Mysqlx::Resultset::Row message;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_data_row *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_data_row_free_storage */
PHP_METHOD(mysqlx_data_row, __construct)
{
}
/* }}} */

/* {{{ proto long mysqlx_data_row::decode(object messsage, array metadata) */
PHP_METHOD(mysqlx_data_row, decode)
{
	zval * object_zv;
	zval * metadata_zv;
	struct st_mysqlx_data_row * object;
	struct st_mysqlx_resultset_metadata * metadata;

	DBG_ENTER("mysqlx_message__stmt_execute::read_response");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OO",
												&object_zv, mysqlx_data_row_class_entry,
												&metadata_zv, mysqlx_resultset_metadata_class_entry,
												&metadata))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_MESSAGE__RESULTSET_METADATA_FROM_ZVAL(metadata, metadata_zv);

	RETVAL_FALSE;
	{
		zval * entry;
		const size_t column_count = zend_hash_num_elements(&metadata->resultset_metadata_ht);
		if (!column_count) {
			php_error_docref(NULL, E_WARNING, "Zero columns");
			DBG_VOID_RETURN;
		}
		struct st_mysqlx_column_metadata * meta_ar[column_count];
		unsigned int i = 0;
		ZEND_HASH_FOREACH_VAL(&metadata->resultset_metadata_ht, entry) {
			if (Z_TYPE_P(entry) == IS_OBJECT && Z_OBJ_P(entry)->ce == mysqlx_column_metadata_class_entry) {
				struct st_mysqlx_column_metadata * column_entry = NULL;
				MYSQLX_FETCH_MESSAGE__COLUMN_METADATA_FROM_ZVAL(column_entry, entry);
				meta_ar[i++] = column_entry;
			}
		} ZEND_HASH_FOREACH_END();

		/* Here we need to decode the data with the metadata found in `meta_ar` */
	}
	DBG_VOID_RETURN;
}
/* }}} */



/* {{{ mysqlx_data_row_methods[] */
static const zend_function_entry mysqlx_data_row_methods[] = {
	PHP_ME(mysqlx_data_row, __construct,				NULL,			ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_data_row, decode,						NULL,			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


/* {{{ mysqlx_column_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_column_meta_property_entries[] =
{
	{{NULL, 				0},									NULL, 										NULL}
};
/* }}} */



static zend_object_handlers mysqlx_object_data_row_handlers;
static HashTable mysqlx_data_row_properties;

/* {{{ mysqlx_data_row_free_storage */
static void
mysqlx_data_row_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_data_row * message = (struct st_mysqlx_data_row  *) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object); 
}
/* }}} */


/* {{{ php_mysqlx_data_row_object_allocator */
static zend_object *
php_mysqlx_data_row_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	struct st_mysqlx_object * mysqlx_object = (struct st_mysqlx_object *) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	struct st_mysqlx_data_row * message = new (std::nothrow) struct st_mysqlx_data_row();

	DBG_ENTER("php_mysqlx_data_row_object_allocator");
	if (!mysqlx_object || !message) {
		goto err;
	}
	mysqlx_object->ptr = message;

	message->persistent = persistent;
	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = &mysqlx_object_data_row_handlers;
	mysqlx_object->properties = &mysqlx_data_row_properties;

	DBG_RETURN(&mysqlx_object->zo);

err:
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlx_register_data_row_class */
extern "C" void
mysqlx_register_data_row_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_data_row_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_data_row_handlers.free_obj = mysqlx_data_row_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_data_row", mysqlx_data_row_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysqlx", "node_pfc", mysqlx_data_row_methods);
		tmp_ce.create_object = php_mysqlx_data_row_object_allocator;
		mysqlx_data_row_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_data_row_properties, 0, NULL, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_data_row_class */
extern "C" void
mysqlx_unregister_data_row_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_data_row_properties);
}
/* }}} */


/* {{{ mysqlx_new_data_row */
void
mysqlx_new_data_row(zval * return_value, const Mysqlx::Resultset::Row & message)
{
	struct st_mysqlx_data_row * obj;
	DBG_ENTER("mysqlx_new_data_row")
	object_init_ex(return_value, mysqlx_data_row_class_entry);
	MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(obj, return_value);
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
