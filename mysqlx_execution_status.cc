/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_execution_status.h"
#include "phputils/allocator.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_execution_status_class_entry;

struct st_mysqlx_execution_status : public phputils::permanent_allocable
{
	size_t items_affected;
	size_t items_matched;
	size_t items_found;
	uint64_t last_insert_id;

	zend_bool persistent;
};

#define TYPE_NAME_ENABLED 1


#define MYSQLX_FETCH__EXECUTION_STATUS_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_execution_status*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_execution_status::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_execution_status, __construct)
{
}
/* }}} */


/* {{{ mysqlx_execution_status_methods[] */
static const zend_function_entry mysqlx_execution_status_methods[] = {
	PHP_ME(mysqlx_execution_status, __construct,				nullptr,			ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_execution_status_property__affected_items */
static zval *
mysqlx_execution_status_property__affected_items(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__affected_items");
	ZVAL_LONG(return_value, object->items_affected);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_execution_status_property__matched_items */
static zval *
mysqlx_execution_status_property__matched_items(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__matched_items");
	ZVAL_LONG(return_value, object->items_matched);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_execution_status_property__found_items */
static zval *
mysqlx_execution_status_property__found_items(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__found_items");
	ZVAL_LONG(return_value, object->items_found);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_execution_status_property__last_insert_id */
static zval *
mysqlx_execution_status_property__last_insert_id(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__last_insert_id");
	ZVAL_LONG(return_value, object->last_insert_id);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_execution_status_property__last_document_id*/
static zval *
mysqlx_execution_status_property__last_document_id(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__last_document_id");
	ZVAL_LONG(return_value, object->last_insert_id);
	DBG_RETURN(return_value);
}
/* }}} */


/* {{{ mysqlx_execution_status_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_execution_status_property_entries[] =
{
	{{"affectedItems", sizeof("affectedItems") - 1}, mysqlx_execution_status_property__affected_items, nullptr},
	{{"matchedItems", sizeof("matchedItems") - 1}, mysqlx_execution_status_property__matched_items, nullptr},
	{{"foundItems", sizeof("foundItems") - 1}, mysqlx_execution_status_property__found_items, nullptr},
	{{"lastInsertId", sizeof("lastInsertId") - 1}, mysqlx_execution_status_property__last_insert_id, nullptr},
	{{"lastDocumentId", sizeof("lastDocumentId") - 1}, mysqlx_execution_status_property__last_document_id, nullptr},
	{{nullptr, 0}, nullptr, nullptr}
};
/* }}} */



static zend_object_handlers mysqlx_object_execution_status_handlers;
static HashTable mysqlx_execution_status_properties;

/* {{{ mysqlx_execution_status_free_storage */
static void
mysqlx_execution_status_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_execution_status* message = (st_mysqlx_execution_status*) mysqlx_object->ptr;

	if (message) {
		mnd_pefree(message, message->persistent);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_execution_status_object_allocator */
static zend_object *
php_mysqlx_execution_status_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_execution_status_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_permanent_object<st_mysqlx_execution_status>(
		class_type,
		&mysqlx_object_execution_status_handlers,
		&mysqlx_execution_status_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_execution_status_class */
void
mysqlx_register_execution_status_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_execution_status_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_execution_status_handlers.free_obj = mysqlx_execution_status_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "ExecutionStatus", mysqlx_execution_status_methods);
		tmp_ce.create_object = php_mysqlx_execution_status_object_allocator;
		mysqlx_execution_status_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_execution_status_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	mysqlx_add_properties(&mysqlx_execution_status_properties, mysqlx_execution_status_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_execution_status_class_entry, "affectedItems",	sizeof("affectedItems") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "matchedItems",		sizeof("matchedItems") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "foundItems",		sizeof("foundItems") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "lastInsertId",		sizeof("lastInsertId") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "lastDocumentId",	sizeof("lastDocumentId") - 1,	ZEND_ACC_PUBLIC);
}
/* }}} */


/* {{{ mysqlx_unregister_execution_status_class */
void
mysqlx_unregister_execution_status_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_execution_status_properties);
}
/* }}} */


/* {{{ mysqlx_new_execution_status */
void
mysqlx_new_execution_status(zval * return_value, const XMYSQLND_STMT_EXECUTION_STATE * const status)
{
	DBG_ENTER("mysqlx_new_execution_status");

	if (SUCCESS == object_init_ex(return_value, mysqlx_execution_status_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_execution_status* const object = (st_mysqlx_execution_status*) mysqlx_object->ptr;
		if (object) {
			object->items_affected = status->m->get_affected_items_count(status);
			object->items_matched = status->m->get_matched_items_count(status);
			object->items_found = status->m->get_found_items_count(status);
			object->last_insert_id = status->m->get_last_insert_id(status);
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

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
