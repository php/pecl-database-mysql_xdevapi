/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_execution_status.h"
#include "util/allocator.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_execution_status_class_entry;

struct st_mysqlx_execution_status : public util::custom_allocable
{
	size_t items_affected;
	size_t items_matched;
	size_t items_found;
	uint64_t last_insert_id;

	zend_bool persistent;
};

#define TYPE_NAME_ENABLED 1


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_execution_status, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

static const zend_function_entry mysqlx_execution_status_methods[] = {
	PHP_ME(mysqlx_execution_status, __construct,				nullptr,			ZEND_ACC_PRIVATE)
	{nullptr, nullptr, nullptr}
};

static zval *
mysqlx_execution_status_property__affected_items(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__affected_items");
	ZVAL_LONG(return_value, object->items_affected);
	DBG_RETURN(return_value);
}

static zval *
mysqlx_execution_status_property__matched_items(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__matched_items");
	ZVAL_LONG(return_value, object->items_matched);
	DBG_RETURN(return_value);
}

static zval *
mysqlx_execution_status_property__found_items(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__found_items");
	ZVAL_LONG(return_value, object->items_found);
	DBG_RETURN(return_value);
}

static zval *
mysqlx_execution_status_property__last_insert_id(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__last_insert_id");
	ZVAL_LONG(return_value, static_cast<zend_long>(object->last_insert_id));
	DBG_RETURN(return_value);
}

static zval *
mysqlx_execution_status_property__last_document_id(const st_mysqlx_object* obj, zval* return_value)
{
	const st_mysqlx_execution_status* object = (st_mysqlx_execution_status*)(obj->ptr);
	DBG_ENTER("mysqlx_execution_status_property__last_document_id");
	ZVAL_LONG(return_value, static_cast<zend_long>(object->last_insert_id));
	DBG_RETURN(return_value);
}

static const st_mysqlx_property_entry mysqlx_execution_status_property_entries[] =
{
	{std::string_view("affectedItems"), mysqlx_execution_status_property__affected_items, nullptr},
	{std::string_view("matchedItems"), mysqlx_execution_status_property__matched_items, nullptr},
	{std::string_view("foundItems"), mysqlx_execution_status_property__found_items, nullptr},
	{std::string_view("lastInsertId"), mysqlx_execution_status_property__last_insert_id, nullptr},
	{std::string_view("lastDocumentId"), mysqlx_execution_status_property__last_document_id, nullptr},
	{std::string_view{}, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_execution_status_handlers;
static HashTable mysqlx_execution_status_properties;

static void
mysqlx_execution_status_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_execution_status>(object);
}

static zend_object *
php_mysqlx_execution_status_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_execution_status_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_execution_status>(
		class_type,
		&mysqlx_object_execution_status_handlers,
		&mysqlx_execution_status_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_execution_status_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_execution_status_class_entry,
		"ExecutionStatus",
		mysqlx_std_object_handlers,
		mysqlx_object_execution_status_handlers,
		php_mysqlx_execution_status_object_allocator,
		mysqlx_execution_status_free_storage,
		mysqlx_execution_status_methods,
		mysqlx_execution_status_properties,
		mysqlx_execution_status_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_execution_status_class_entry, "affectedItems",	sizeof("affectedItems") - 1,	ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "matchedItems",		sizeof("matchedItems") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "foundItems",		sizeof("foundItems") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "lastInsertId",		sizeof("lastInsertId") - 1,		ZEND_ACC_PUBLIC);
	zend_declare_property_null(mysqlx_execution_status_class_entry, "lastDocumentId",	sizeof("lastDocumentId") - 1,	ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_execution_status_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_execution_status_properties);
}

void
mysqlx_new_execution_status(zval* return_value, const XMYSQLND_STMT_EXECUTION_STATE* const status)
{
	DBG_ENTER("mysqlx_new_execution_status");

	st_mysqlx_execution_status& data_object{
		util::init_object<st_mysqlx_execution_status>(mysqlx_execution_status_class_entry, return_value) };
	data_object.items_affected = status->m->get_affected_items_count(status);
	data_object.items_matched = status->m->get_matched_items_count(status);
	data_object.items_found = status->m->get_found_items_count(status);
	data_object.last_insert_id = status->m->get_last_insert_id(status);

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
