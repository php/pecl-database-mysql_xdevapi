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
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_table.h"
#include "xmysqlnd/xmysqlnd_crud_table_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_table__delete.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry *mysqlx_table__delete_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__where, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, where_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__orderby, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, orderby_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__offset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__delete__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table__delete : public util::custom_allocable
{
	XMYSQLND_CRUD_TABLE_OP__DELETE * crud_op;
	XMYSQLND_TABLE * table;
};


#define MYSQLX_FETCH_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_table__delete*) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_table__delete::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */



/* {{{ proto mixed mysqlx_table__delete::where() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, where)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};
	zval* where_expr{nullptr};

	DBG_ENTER("mysqlx_table__delete::where");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Oz",
		&object_zv, mysqlx_table__delete_class_entry,
		&where_expr))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && where_expr)
	{

		if(Z_TYPE_P(where_expr) != IS_STRING) {
			RAISE_EXCEPTION(err_msg_wrong_param_4);
		}
		else
		{
			const MYSQLND_CSTRING where_expr_str = {Z_STRVAL_P(where_expr), Z_STRLEN_P(where_expr)};
			if (PASS == xmysqlnd_crud_table_delete__set_criteria(object->crud_op, where_expr_str)) {
				ZVAL_COPY(return_value, object_zv);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__delete::orderby() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, orderby)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};
	zval* orderby_expr{nullptr};
	int num_of_expr{0};

	DBG_ENTER("mysqlx_table__delete::orderby");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
									&object_zv,
									mysqlx_table__delete_class_entry,
									&orderby_expr,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (!( object->crud_op && orderby_expr ) ) {
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(orderby_expr[i]))
		{
		case IS_STRING:
			{
				const MYSQLND_CSTRING orderby_expr_str = {Z_STRVAL(orderby_expr[i]),
													Z_STRLEN(orderby_expr[i])};
				if (PASS == xmysqlnd_crud_table_delete__add_orderby(object->crud_op, orderby_expr_str)) {
					ZVAL_COPY(return_value, object_zv);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL(orderby_expr[i]), entry)
				{
					const MYSQLND_CSTRING orderby_expr_str = {Z_STRVAL_P(entry), Z_STRLEN_P(entry)};
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (FAIL == xmysqlnd_crud_table_delete__add_orderby(object->crud_op, orderby_expr_str))
					{
						RAISE_EXCEPTION(err_msg_add_orderby_fail);
						DBG_VOID_RETURN;
					}
				} ZEND_HASH_FOREACH_END();
				ZVAL_COPY(return_value, object_zv);
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			break;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__delete::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, limit)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};
	zend_long rows;

	DBG_ENTER("mysqlx_table__delete::limit");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
		&object_zv, mysqlx_table__delete_class_entry,
		&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0)
	{
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op)
	{
		if (PASS == xmysqlnd_crud_table_delete__set_limit(object->crud_op, rows)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__delete::offset() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, offset)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};
	zend_long position;

	DBG_ENTER("mysqlx_table__delete::offset");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
		&object_zv, mysqlx_table__delete_class_entry,
		&position))
	{
		DBG_VOID_RETURN;
	}
	if (position < 0)
	{
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op)
	{
		if (PASS == xmysqlnd_crud_table_delete__set_offset(object->crud_op, position)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__delete::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, bind)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};
	HashTable * bind_variables;

	DBG_ENTER("mysqlx_table__delete::bind");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Oh",
		&object_zv, mysqlx_table__delete_class_entry,
		&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op)
	{
		zend_string * key;
		zval* val{nullptr};
		MYSQLX_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val)
		{
			if (key)
			{
				const MYSQLND_CSTRING variable = {ZSTR_VAL(key), ZSTR_LEN(key)};
				if (FAIL == xmysqlnd_crud_table_delete__bind_value(object->crud_op, variable, val))
				{
					RAISE_EXCEPTION(err_msg_bind_fail);
					DBG_VOID_RETURN;
				}
			}
		} ZEND_HASH_FOREACH_END();
		ZVAL_COPY(return_value, object_zv);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__delete::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__delete, execute)
{
	st_mysqlx_table__delete* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table__delete::execute");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, mysqlx_table__delete_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	DBG_INF_FMT("crud_op=%p table=%p", object->crud_op, object->table);
	if (object->crud_op && object->table) {
		if (FALSE == xmysqlnd_crud_table_delete__is_initialized(object->crud_op)) {
			RAISE_EXCEPTION(err_msg_delete_fail);
		} else {
			XMYSQLND_STMT * stmt = object->table->data->m.opdelete(object->table, object->crud_op);
			if (stmt) {
				zval stmt_zv;
				ZVAL_UNDEF(&stmt_zv);
				mysqlx_new_stmt(&stmt_zv, stmt);
				if (Z_TYPE(stmt_zv) == IS_NULL) {
					xmysqlnd_stmt_free(stmt, nullptr, nullptr);
				}
				if (Z_TYPE(stmt_zv) == IS_OBJECT) {
					zval zv;
					ZVAL_UNDEF(&zv);
					zend_long flags{0};
					mysqlx_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv), flags, MYSQLX_RESULT, &zv);

					ZVAL_COPY(return_value, &zv);
					zval_dtor(&zv);
				}
				zval_ptr_dtor(&stmt_zv);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_table__delete_methods[] */
static const zend_function_entry mysqlx_table__delete_methods[] = {
	PHP_ME(mysqlx_table__delete, __construct,	nullptr,									ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_table__delete, where,	arginfo_mysqlx_table__delete__where,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__delete, orderby,	arginfo_mysqlx_table__delete__orderby,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__delete, limit,	arginfo_mysqlx_table__delete__limit,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__delete, offset,	arginfo_mysqlx_table__delete__offset,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__delete, bind,		arginfo_mysqlx_table__delete__bind,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_table__delete, execute,	arginfo_mysqlx_table__delete__execute,	ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */

#if 0
/* {{{ mysqlx_table__delete_property__name */
static zval *
mysqlx_table__delete_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_table__delete* object = (const st_mysqlx_table__delete* ) (obj->ptr);
	DBG_ENTER("mysqlx_table__delete_property__name");
	if (object->table && object->table->data->table_name.s) {
		ZVAL_STRINGL(return_value, object->table->data->table_name.s, object->table->data->table_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */
#endif

static zend_object_handlers mysqlx_object_table__delete_handlers;
static HashTable mysqlx_table__delete_properties;

const struct st_mysqlx_property_entry mysqlx_table__delete_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_table__delete_property__name,	nullptr},
#endif
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_table__delete_free_storage */
static void
mysqlx_table__delete_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_table__delete* inner_obj = (st_mysqlx_table__delete*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_table_free(inner_obj->table, nullptr, nullptr);
			inner_obj->table = nullptr;
		}
		if(inner_obj->crud_op) {
			xmysqlnd_crud_table_delete__destroy(inner_obj->crud_op);
			inner_obj->crud_op = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_table__delete_object_allocator */
static zend_object *
php_mysqlx_table__delete_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_table__delete_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table__delete>(
		class_type,
		&mysqlx_object_table__delete_handlers,
		&mysqlx_table__delete_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_table__delete_class */
void
mysqlx_register_table__delete_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_table__delete_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_table__delete_handlers.free_obj = mysqlx_table__delete_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "TableDelete", mysqlx_table__delete_methods);
		tmp_ce.create_object = php_mysqlx_table__delete_object_allocator;
		mysqlx_table__delete_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_table__delete_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_table__delete_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_table__delete_properties, mysqlx_table__delete_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table__delete_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_table__delete_class */
void
mysqlx_unregister_table__delete_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table__delete_properties);
}
/* }}} */


/* {{{ mysqlx_new_table__delete */
void
mysqlx_new_table__delete(zval * return_value, XMYSQLND_TABLE * table, const zend_bool clone)
{
	DBG_ENTER("mysqlx_new_table__delete");

	if (SUCCESS == object_init_ex(return_value, mysqlx_table__delete_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_table__delete* const object = (st_mysqlx_table__delete*) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->data->m.get_reference(table) : table;
			object->crud_op = xmysqlnd_crud_table_delete__create(
				mnd_str2c(object->table->data->schema->data->schema_name),
				mnd_str2c(object->table->data->table_name));
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
