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
#include "xmysqlnd/xmysqlnd_zval2any.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "messages/mysqlx_message__capability.h"
#include "mysqlx_resultset__resultset_metadata.h"
#include "mysqlx_resultset__column_metadata.h"

#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

zend_class_entry *mysqlx_resultset_metadata_class_entry;


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_resultset_metadata__add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, capability, IS_OBJECT, 0)
ZEND_END_ARG_INFO()


/* {{{ proto bool mysqlx_connection::echo(object capability) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_resultset_metadata, add)
{
	zval* resultset_metadata_zv{nullptr};
	st_mysqlx_resultset_metadata* resultset_metadata{nullptr};
	zval* column_metadata_zv{nullptr};

	DBG_ENTER("mysqlx_connection::add");
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "OO",
												&resultset_metadata_zv, mysqlx_resultset_metadata_class_entry,
												&column_metadata_zv, mysqlx_column_metadata_class_entry))
	{
		DBG_VOID_RETURN;
	}
	MYSQLX_FETCH_MESSAGE__RESULTSET_METADATA_FROM_ZVAL(resultset_metadata, resultset_metadata_zv);

	if (Z_REFCOUNTED_P(column_metadata_zv)) {
		Z_ADDREF_P(column_metadata_zv);
	}
	zend_hash_next_index_insert(&resultset_metadata->resultset_metadata_ht, column_metadata_zv);
	MYSQLX_SUPPRESS_MSVC_WARNINGS(4127)
	RETVAL_ZVAL(resultset_metadata_zv, 1 /*copy*/, 0 /*dtor*/);
	MYSQLX_RESTORE_WARNINGS()
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_resultset_metadata_methods[] */
static const zend_function_entry mysqlx_resultset_metadata_methods[] = {
	PHP_ME(mysqlx_resultset_metadata, add,			arginfo_mysqlx_resultset_metadata__add,			ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers mysqlx_object_resultset_metadata_handlers;
static HashTable mysqlx_resultset_metadata_properties;


/* {{{ mysqlx_resultset_metadata_free_storage */
static void
mysqlx_resultset_metadata_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_resultset_metadata* message = (st_mysqlx_resultset_metadata*) mysqlx_object->ptr;

	if (message) {
		zend_hash_destroy(&message->resultset_metadata_ht);
		delete message;
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_resultset_metadata_object_allocator */
static zend_object *
php_mysqlx_resultset_metadata_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	st_mysqlx_object* mysqlx_object = (st_mysqlx_object*) mnd_ecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type));
	st_mysqlx_resultset_metadata* message = new struct st_mysqlx_resultset_metadata;

	DBG_ENTER("php_mysqlx_resultset_metadata_object_allocator");
	if ( mysqlx_object && message) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_hash_init(&message->resultset_metadata_ht, 0, nullptr /*hashfunc*/, ZVAL_PTR_DTOR, persistent);

		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_resultset_metadata_handlers;
		mysqlx_object->properties = &mysqlx_resultset_metadata_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (message) {
		zend_hash_destroy(&message->resultset_metadata_ht);
		delete message;
	}
	if (mysqlx_object) {
		mnd_efree(mysqlx_object);
	}
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_resultset_metadata_class */
void
mysqlx_register_resultset_metadata_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_resultset_metadata_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_resultset_metadata_handlers.free_obj = mysqlx_resultset_metadata_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_resultset_metadata", mysqlx_resultset_metadata_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "resultset_metadata", mysqlx_resultset_metadata_methods);
		tmp_ce.create_object = php_mysqlx_resultset_metadata_object_allocator;
		mysqlx_resultset_metadata_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_resultset_metadata_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_resultset_metadata_class */
void
mysqlx_unregister_resultset_metadata_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_resultset_metadata_properties);
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
