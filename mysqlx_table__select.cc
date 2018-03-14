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
#include "xmysqlnd/xmysqlnd_table.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_crud_table_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_table__select.h"
#include "util/allocator.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

zend_class_entry* mysqlx_table__select_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__where, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, projection)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__group_by, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__having, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__orderby, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__offset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__lock_shared, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__lock_exclusive, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_table__select__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_table__select : public util::custom_allocable
{
	XMYSQLND_CRUD_TABLE_OP__SELECT * crud_op;
	XMYSQLND_NODE_TABLE * table;
};


#define MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(_to, _from) \
{ \
	const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_table__select*) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->table) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \


/* {{{ mysqlx_table__select::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, __construct)
{
}
/* }}} */


/* {{{ mysqlx_table__select::where */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, where)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	MYSQLND_CSTRING where_expr = {nullptr, 0};

	DBG_ENTER("mysqlx_table__select::where");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_table__select_class_entry,
												&(where_expr.s), &(where_expr.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->table && where_expr.s && where_expr.l)
	{
		if (PASS == xmysqlnd_crud_table_select__set_criteria(object->crud_op, where_expr))
		{
			ZVAL_COPY(return_value, object_zv);
		}
	}
}
/* }}} */


#define ADD_SORT 1
#define ADD_GROUPING 2

/* {{{ mysqlx_table__select__add_sort_or_grouping */
static void
mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAMETERS, const unsigned int op_type)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	zval* sort_expr{nullptr};
	int num_of_expr{0};

	DBG_ENTER("mysqlx_table__select__add_sort_or_grouping");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
												&object_zv,
												mysqlx_table__select_class_entry,
												&sort_expr,
												&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_expr ; ++i ) {
		if (Z_TYPE(sort_expr[i]) != IS_STRING &&
			Z_TYPE(sort_expr[i]) != IS_OBJECT &&
			Z_TYPE(sort_expr[i]) != IS_ARRAY) {
			php_error_docref(nullptr, E_WARNING, "Only strings, objects and arrays can be added. Type is %u",
							 Z_TYPE(sort_expr[i]));
			DBG_VOID_RETURN;
		}
	}


	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (!( object->crud_op && sort_expr ) ) {
		DBG_VOID_RETURN;
	}

	for(int i{0}; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i]))
		{
		case IS_STRING:
			{
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL(sort_expr[i]), Z_STRLEN(sort_expr[i]) };
				if (ADD_SORT == op_type) {
					if (PASS == xmysqlnd_crud_table_select__add_orderby(object->crud_op, sort_expr_str)) {
						ZVAL_COPY(return_value, object_zv);
					}
				} else if (ADD_GROUPING == op_type) {
					if (PASS == xmysqlnd_crud_table_select__add_grouping(object->crud_op, sort_expr_str)) {
						ZVAL_COPY(return_value, object_zv);
					}
				}
				break;
			}
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				enum_func_status ret{FAIL};
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					ret = FAIL;
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (ADD_SORT == op_type) {
						ret = xmysqlnd_crud_table_select__add_orderby(object->crud_op, sort_expr_str);
					} else if (ADD_GROUPING == op_type) {
						ret = xmysqlnd_crud_table_select__add_grouping(object->crud_op, sort_expr_str);
					}
					if (FAIL == ret) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
						DBG_VOID_RETURN;
					}
				} ZEND_HASH_FOREACH_END();
				if( FAIL != ret ) {
					ZVAL_COPY(return_value, object_zv);
				}
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


/* {{{ proto mixed mysqlx_table__select::orderby() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, orderby)
{
	DBG_ENTER("mysqlx_table__select::orderby");
	mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_SORT);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::groupBy() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, groupBy)
{
	DBG_ENTER("mysqlx_table__select::groupBy");
	mysqlx_table__select__add_sort_or_grouping(INTERNAL_FUNCTION_PARAM_PASSTHRU, ADD_GROUPING);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::having() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, having)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	MYSQLND_CSTRING search_condition = {nullptr, 0};

	DBG_ENTER("mysqlx_table__select::having");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, mysqlx_table__select_class_entry,
												&(search_condition.s), &(search_condition.l)))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->table) {
		if (PASS == xmysqlnd_crud_table_select__set_having(object->crud_op, search_condition)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, limit)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	zend_long rows;

	DBG_ENTER("mysqlx_table__select::limit");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_table__select_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->table) {
		if (PASS == xmysqlnd_crud_table_select__set_limit(object->crud_op, rows)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::offset() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, offset)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	zend_long position;

	DBG_ENTER("mysqlx_table__select::offset");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, mysqlx_table__select_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->table) {
		if (PASS == xmysqlnd_crud_table_select__set_offset(object->crud_op, position)) {
			ZVAL_COPY(return_value, object_zv);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, bind)
{
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};
	HashTable * bind_variables;

	DBG_ENTER("mysqlx_table__select::bind");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, mysqlx_table__select_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->table) {
		zend_string * key;
		zval* val{nullptr};
		ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
			if (key) {
				const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
				if (FAIL == xmysqlnd_crud_table_select__bind_value(object->crud_op, variable, val)) {
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


/* {{{ proto mixed mysqlx_table__select::lockShared() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, lockShared)
{
	DBG_ENTER("mysqlx_table__select::lockShared");

	zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
		&object_zv, mysqlx_table__select_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	auto crud_op = data_object.crud_op;
	int waiting_option = static_cast<int>(lock_waiting_option);
	if ((xmysqlnd_crud_table_select__enable_lock_shared(crud_op) == PASS)
		&& (xmysqlnd_crud_table_select_set_lock_waiting_option(crud_op, waiting_option) == PASS))
	{
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::lockExclusive() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, lockExclusive)
{
	DBG_ENTER("mysqlx_table__select::lockExclusive");

	zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
		&object_zv, mysqlx_table__select_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	auto crud_op = data_object.crud_op;
	int waiting_option = static_cast<int>(lock_waiting_option);
	if ((xmysqlnd_crud_table_select__enable_lock_exclusive(crud_op) == PASS)
		&& (xmysqlnd_crud_table_select_set_lock_waiting_option(crud_op, waiting_option) == PASS))
	{
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_table__select::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_table__select, execute)
{
	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};
	st_mysqlx_table__select* object{nullptr};
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_table__select::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv, mysqlx_table__select_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_TABLE_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->crud_op && object->table) {
		if (FALSE == xmysqlnd_crud_table_select__is_initialized(object->crud_op)) {
			RAISE_EXCEPTION(err_msg_find_fail);
		} else {
			XMYSQLND_NODE_STMT * stmt = object->table->data->m.select(object->table, object->crud_op);
			{
				if (stmt) {
					zval stmt_zv;
					ZVAL_UNDEF(&stmt_zv);
					mysqlx_new_stmt(&stmt_zv, stmt);
					if (Z_TYPE(stmt_zv) == IS_NULL) {
						xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
					}
					if (Z_TYPE(stmt_zv) == IS_OBJECT) {
						zval zv;
						ZVAL_UNDEF(&zv);
						mysqlx_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv), flags, MYSQLX_RESULT_ROW, &zv);

						ZVAL_COPY(return_value, &zv);
						zval_dtor(&zv);
					}
					zval_ptr_dtor(&stmt_zv);
				}
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_table__select_methods[] */
static const zend_function_entry mysqlx_table__select_methods[] = {
	PHP_ME(mysqlx_table__select, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_table__select, where, arginfo_mysqlx_table__select__where, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, groupBy, arginfo_mysqlx_table__select__group_by, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, having, arginfo_mysqlx_table__select__having, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, bind, arginfo_mysqlx_table__select__bind, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, orderby, arginfo_mysqlx_table__select__orderby, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, limit, arginfo_mysqlx_table__select__limit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, offset, arginfo_mysqlx_table__select__offset, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, lockShared, arginfo_mysqlx_table__select__lock_shared, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, lockExclusive, arginfo_mysqlx_table__select__lock_exclusive, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_table__select, execute, arginfo_mysqlx_table__select__execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */

#if 0
/* {{{ mysqlx_table__select_property__name */
static zval *
mysqlx_table__select_property__name(const st_mysqlx_object* obj, zval * return_value)
{
	const st_mysqlx_table__select* object = (const st_mysqlx_table__select* ) (obj->ptr);
	DBG_ENTER("mysqlx_table__select_property__name");
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

static zend_object_handlers mysqlx_object_node_table__select_handlers;
static HashTable mysqlx_table__select_properties;

const struct st_mysqlx_property_entry mysqlx_table__select_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_table__select_property__name,	nullptr},
#endif
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_table__select_free_storage */
static void
mysqlx_table__select_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_table__select* inner_obj = (st_mysqlx_table__select*) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->table) {
			xmysqlnd_node_table_free(inner_obj->table, nullptr, nullptr);
			inner_obj->table = nullptr;
		}
		if(inner_obj->crud_op) {
			xmysqlnd_crud_table_select__destroy(inner_obj->crud_op);
			inner_obj->crud_op = nullptr;
		}
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_table__select_object_allocator */
static zend_object *
php_mysqlx_table__select_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_table__select_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_table__select>(
		class_type,
		&mysqlx_object_node_table__select_handlers,
		&mysqlx_table__select_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_table__select_class */
void
mysqlx_register_table__select_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_table__select_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_table__select_handlers.free_obj = mysqlx_table__select_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "TableSelect", mysqlx_table__select_methods);
		tmp_ce.create_object = php_mysqlx_table__select_object_allocator;
		mysqlx_table__select_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_table__select_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_table__select_properties, 0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_table__select_properties, mysqlx_table__select_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_table__select_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_table__select_class */
void
mysqlx_unregister_table__select_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_table__select_properties);
}
/* }}} */


/* {{{ mysqlx_new_table__select */
void
mysqlx_new_table__select(zval * return_value,
				XMYSQLND_NODE_TABLE * table,
				const zend_bool clone,
				zval * columns,
				const int num_of_columns)
{
	DBG_ENTER("mysqlx_new_table__select");

	if (SUCCESS == object_init_ex(return_value, mysqlx_table__select_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		st_mysqlx_table__select* const object = (st_mysqlx_table__select*) mysqlx_object->ptr;
		if (object) {
			object->table = clone? table->data->m.get_reference(table) : table;
			object->crud_op = xmysqlnd_crud_table_select__create(
				mnd_str2c(object->table->data->schema->data->schema_name),
				mnd_str2c(object->table->data->table_name),
				columns,
				num_of_columns);
		} else {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ get_stmt_from_table_select */
Mysqlx::Crud::Find* get_stmt_from_table_select(zval* object_zv)
{
	auto& data_object = util::fetch_data_object<st_mysqlx_table__select>(object_zv);
	XMYSQLND_CRUD_TABLE_OP__SELECT* select_op = data_object.crud_op;
	if (!select_op
		|| (xmysqlnd_crud_table_select__finalize_bind(select_op) == FAIL)
		|| !xmysqlnd_crud_table_select__is_initialized(select_op))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
	}

	st_xmysqlnd_pb_message_shell msg_shell = xmysqlnd_crud_table_select__get_protobuf_message(select_op);
	Mysqlx::Crud::Find* msg = static_cast<Mysqlx::Crud::Find*>(msg_shell.message);
	return msg;
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
