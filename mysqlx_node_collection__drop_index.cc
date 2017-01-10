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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include <xmysqlnd/xmysqlnd_index_collection_commands.h>
#include "php/exceptions.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__drop_index.h"

#include "php/allocator.h"
#include "php/exceptions.h"
#include "php/object.h"

namespace mysql
//namespace mysqlx
{

namespace api
{

namespace
{

static zend_class_entry* collection_drop_index_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_collection_drop_index__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct collection_drop_index_data : php::custom_allocable
{
	~collection_drop_index_data()
	{
		if (collection) {
			xmysqlnd_node_collection_free(collection, nullptr, nullptr);
		}

		if (index_op) {
			xmysqlnd_collection_drop_index__destroy(index_op);
		}
	}

	drv::XMYSQLND_COLLECTION_OP__DROP_INDEX* index_op = nullptr;
	XMYSQLND_NODE_COLLECTION* collection = nullptr;
};


/* {{{ mysqlx_node_collection__drop_index::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__drop_index, __construct)
{
}
/* }}} */


/* {{{ mysqlx_node_collection_drop_index_on_error */
static const enum_hnd_func_status
mysqlx_node_collection_drop_index_on_error(
	void * context,
	XMYSQLND_NODE_SESSION * session,
	st_xmysqlnd_node_stmt * const stmt,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_collection_drop_index_on_error");
	throw mysql::php::xdevapi_exception(code, mysql::php::string(sql_state.s, sql_state.l), mysql::php::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__drop_index::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__drop_index, execute)
{
	DBG_ENTER("mysqlx_node_collection__drop_index::execute");

	RETVAL_FALSE;

	zval * object_zv;
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, collection_drop_index_class_entry))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = php::fetch_data_object<collection_drop_index_data>(object_zv);

	if (!drv::xmysqlnd_collection_drop_index__is_initialized(data_object.index_op)) {
		const unsigned int ErrCode = 10008;
		throw mysql::php::xdevapi_exception(ErrCode, "HY000", "DropIndex not completely initialized");
	}

	const st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_node_collection_drop_index_on_error, NULL };
	if (PASS == drv::xmysqlnd_collection_drop_index__execute(data_object.collection->data->schema->data->session, data_object.index_op, on_error)) {
		RETVAL_TRUE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ collection_drop_index_methods[] */
static const zend_function_entry collection_drop_index_methods[] = {
	PHP_ME(mysqlx_node_collection__drop_index, __construct,	NULL, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__drop_index, execute,	arginfo_collection_drop_index__execute, ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers collection_drop_index_handlers;
static HashTable collection_drop_index_properties;

const struct st_mysqlx_property_entry collection_drop_index_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection__drop_index_free_storage */
static void
mysqlx_node_collection__drop_index_free_storage(zend_object * object)
{
	php::free_object<collection_drop_index_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__drop_index_object_allocator */
static zend_object *
php_mysqlx_node_collection__drop_index_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_collection__drop_index_object_allocator");
	st_mysqlx_object* mysqlx_object = php::alloc_object<collection_drop_index_data>(
		class_type,
		&collection_drop_index_handlers,
		&collection_drop_index_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace

} // namespace api

//} // namespace mysqlx
} // namespace mysql

extern "C"
{

//TODO ds: temporarily till we rename most of *.c into *.cc
using namespace mysql;
using namespace mysql::api;

/* {{{ mysqlx_register_node_collection__drop_index_class */
void
mysqlx_register_node_collection__drop_index_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_drop_index_class_entry,
		"NodeCollectionDropIndex",
		mysqlx_std_object_handlers,
		collection_drop_index_handlers,
		php_mysqlx_node_collection__drop_index_object_allocator,
		mysqlx_node_collection__drop_index_free_storage,
		collection_drop_index_methods,
		collection_drop_index_properties,
		collection_drop_index_property_entries,
		mysqlx_executable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__drop_index_class */
void
mysqlx_unregister_node_collection__drop_index_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_drop_index_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__drop_index */
void
mysqlx_new_node_collection__drop_index(
	zval * return_value,
	const MYSQLND_CSTRING index_name,
	XMYSQLND_NODE_COLLECTION * collection)
{
	DBG_ENTER("mysqlx_new_node_collection__drop_index");
	//TODO temporarily try/catch, port files from *.c to *.cc to apply exceptions/destructors everywhere :-}
	MYSQL_XDEVAPI_TRY {
		auto& data_object = php::init_object<collection_drop_index_data>(collection_drop_index_class_entry, return_value);

		data_object.collection = collection->data->m.get_reference(collection);
		data_object.index_op = drv::xmysqlnd_collection_drop_index__create(
			mnd_str2c(data_object.collection->data->schema->data->schema_name),
			mnd_str2c(data_object.collection->data->collection_name));
		if (!data_object.index_op) {
			goto err;
		}
		if (index_name.s &&
			index_name.l &&
			FAIL == drv::xmysqlnd_collection_drop_index__set_index_name(data_object.index_op, index_name))
		{
			goto err;
		}
		goto end;
err:
		DBG_ERR("Error");
		if (data_object.collection) {
			data_object.collection->data->m.free_reference(data_object.collection, NULL, NULL);
		}
		zval_ptr_dtor(return_value);
		ZVAL_NULL(return_value);
		throw php::docref_exception(php::docref_exception::Kind::Warning, collection_drop_index_class_entry);
	} MYSQL_XDEVAPI_CATCH
end:
	DBG_VOID_RETURN;
}
/* }}} */

} // extern "C"

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
