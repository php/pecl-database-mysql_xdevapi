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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_index_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__create_index.h"
#include "mysqlx_object.h"
#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

static zend_class_entry* collection_create_index_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_collection_create_index__field, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, doc_path, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, column_type, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, is_required, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_collection_create_index__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct collection_create_index_data : public phputils::custom_allocable
{
	~collection_create_index_data()
	{
		if (collection) {
			xmysqlnd_node_collection_free(collection, nullptr, nullptr);
		}

		if (index_op) {
			xmysqlnd_collection_create_index__destroy(index_op);
		}
	}

	drv::XMYSQLND_COLLECTION_OP__CREATE_INDEX* index_op{nullptr};
	XMYSQLND_NODE_COLLECTION* collection{nullptr};
};

/* {{{ mysqlx_node_collection__create_index::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__create_index, __construct)
{
}
/* }}} */


/* {{{ mysqlx_node_collection__create_index::field */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__create_index, field)
{
	DBG_ENTER("mysqlx_node_collection__create_index::field");

	RETVAL_FALSE;

	zval * object_zv;
	MYSQLND_CSTRING doc_path = {nullptr, 0};
	MYSQLND_CSTRING column_type = {nullptr, 0};
	zend_bool is_required{FALSE};

	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Ossb",
		&object_zv, collection_create_index_class_entry,
		&(doc_path.s), &(doc_path.l),
		&(column_type.s), &(column_type.l),
		&is_required))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<collection_create_index_data>(object_zv);

	if (drv::xmysqlnd_collection_create_index__add_field(data_object.index_op, doc_path, column_type, is_required) == FAIL) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::add_index_field_err);
	}

	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection_create_index_on_error */
static const enum_hnd_func_status
mysqlx_node_collection_create_index_on_error(
	void * context,
	XMYSQLND_NODE_SESSION * session,
	st_xmysqlnd_node_stmt * const stmt,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("mysqlx_node_collection_create_index_on_error");
	throw phputils::xdevapi_exception(code, phputils::string(sql_state.s, sql_state.l), phputils::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

/* {{{ proto mixed mysqlx_node_collection__create_index::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__create_index, execute)
{
	DBG_ENTER("mysqlx_node_collection__create_index::execute");

	RETVAL_FALSE;

	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};
	zval * object_zv;

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
												&object_zv, collection_create_index_class_entry,
												&flags))
	{
		DBG_VOID_RETURN;
	}

	auto& data_object = phputils::fetch_data_object<collection_create_index_data>(object_zv);

	if (!drv::xmysqlnd_collection_create_index__is_initialized(data_object.index_op)) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::create_index_fail);
	}

	const st_xmysqlnd_node_session_on_error_bind on_error = { mysqlx_node_collection_create_index_on_error, nullptr };
	if (drv::xmysqlnd_collection_create_index__execute(data_object.collection->data->schema->data->session, data_object.index_op, on_error) == PASS) {
		RETVAL_TRUE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__create_index_methods[] */
static const zend_function_entry mysqlx_node_collection__create_index_methods[] = {
	PHP_ME(mysqlx_node_collection__create_index, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__create_index, field, arginfo_collection_create_index__field, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__create_index, execute, arginfo_collection_create_index__execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers collection_create_index_handlers;
static HashTable collection_create_index_properties;

const struct st_mysqlx_property_entry collection_create_index_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_collection__create_index_free_storage */
static void
mysqlx_node_collection__create_index_free_storage(zend_object * object)
{
	phputils::free_object<collection_create_index_data>(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__create_index_object_allocator */
static zend_object *
php_mysqlx_node_collection__create_index_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_collection__create_index_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<collection_create_index_data>(
		class_type,
		&collection_create_index_handlers,
		&collection_create_index_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

} // anonymous namespace

/* {{{ mysqlx_register_node_collection__create_index_class */
void
mysqlx_register_node_collection__create_index_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	//ds: due to macro INIT_NS_CLASS_ENTRY there were problems with moving that part of code into phputils::register_class
	//optionally macro MYSQL_XDEVAPI_REGISTER_CLASS
	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeCollectionCreateIndex", mysqlx_node_collection__create_index_methods);

	collection_create_index_class_entry = phputils::register_class(
		&tmp_ce,
		mysqlx_std_object_handlers,
		&collection_create_index_handlers,
		php_mysqlx_node_collection__create_index_object_allocator,
		//ds: optionally use phputils::free_object<collection_create_index_data>
		//and get rid of mysqlx_node_collection__create_index_free_storage
		mysqlx_node_collection__create_index_free_storage,
		&collection_create_index_properties,
		collection_create_index_property_entries,
		mysqlx_executable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__create_index_class */
void
mysqlx_unregister_node_collection__create_index_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_create_index_properties);
}
/* }}} */

/* {{{ mysqlx_new_node_collection__create_index */
void
mysqlx_new_node_collection__create_index(zval * return_value,
	const MYSQLND_CSTRING index_name,
	const zend_bool is_unique,
	XMYSQLND_NODE_COLLECTION * collection)
{
	bool operation_failed{ true };
	DBG_ENTER("mysqlx_new_node_collection__create_index");
	if( !index_name.s ||
		!index_name.l ) {
		DBG_ERR("Index name should contain a valid name!");
	} else {
		//TODO temporarily try/catch, port files from *.c to *.cc to apply exceptions/destructors everywhere :-}
		MYSQL_XDEVAPI_TRY {
			auto& data_object = phputils::init_object<collection_create_index_data>(collection_create_index_class_entry, return_value);

			data_object.collection = collection->data->m.get_reference(collection);
			data_object.index_op = drv::xmysqlnd_collection_create_index__create(
				mnd_str2c(data_object.collection->data->schema->data->schema_name),
				mnd_str2c(data_object.collection->data->collection_name));

			if (!data_object.index_op) {
				DBG_ERR("Unable to create the index operation!");
			} else {
				if ( (FAIL == drv::xmysqlnd_collection_create_index__set_index_name(data_object.index_op, index_name)) ||
					 (FAIL == drv::xmysqlnd_collection_create_index__set_unique(data_object.index_op, is_unique)) ) {
					DBG_ERR("Unable to setup index attributes!");
				} else {
					operation_failed = false;
				}
			}
			if( true == operation_failed ) {
				if (data_object.collection) {
					data_object.collection->data->m.free_reference(data_object.collection, nullptr, nullptr);
				}
			}
		} MYSQL_XDEVAPI_CATCH
	}
	if( true == operation_failed ) {
		zval_ptr_dtor(return_value);
		ZVAL_NULL(return_value);
		throw phputils::doc_ref_exception(phputils::doc_ref_exception::Severity::warning, collection_create_index_class_entry);
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
