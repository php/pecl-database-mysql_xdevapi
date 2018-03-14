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
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_skippable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__find.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/object.h"

#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

zend_class_entry* collection_find_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__fields, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, projection)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__group_by, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__having, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__lock_shared, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__lock_exclusive, 0, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, lock_waiting_option, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__find__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


/* {{{ Collection_find::init() */
bool Collection_find::init(
	zval* obj_zv,
	XMYSQLND_NODE_COLLECTION* coll,
	const util::string_view& search_expression)
{
	if (!obj_zv || !coll) return false;

	object_zv = obj_zv;
	collection = coll->data->m.get_reference(coll);
	find_op = xmysqlnd_crud_collection_find__create(
		mnd_str2c(collection->data->schema->data->schema_name),
		mnd_str2c(collection->data->collection_name));

	if (!find_op) return false;

	if (search_expression.empty()) return true;

	return xmysqlnd_crud_collection_find__set_criteria(
		find_op, search_expression.to_nd_cstr()) == PASS;
}
/* }}} */


/* {{{ Collection_find::~Collection_find() */
Collection_find::~Collection_find()
{
	if (find_op) {
		xmysqlnd_crud_collection_find__destroy(find_op);
	}

	if (collection) {
		xmysqlnd_node_collection_free(collection, nullptr, nullptr);
	}
}
/* }}} */


/* {{{ mysqlx_node_collection__find::fields */
void Collection_find::fields(
	const zval* fields,
	zval* return_value)
{
	DBG_ENTER("Collection_find::fields");

	bool is_expression{false};
	switch (Z_TYPE_P(fields)) {
		case IS_STRING:
		case IS_ARRAY:
			break;
		case IS_OBJECT:
			if (is_a_mysqlx_expression(fields)) {
				/* get the string */
				fields = get_mysqlx_expression(fields);
				is_expression = true;
			}
			/* fall-through */
		default:
			RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	enum_func_status ret{PASS};
	if (Z_TYPE_P(fields) == IS_STRING) {
		const MYSQLND_CSTRING field_str = { Z_STRVAL_P(fields), Z_STRLEN_P(fields) };
		ret = xmysqlnd_crud_collection_find__set_fields(find_op, field_str, is_expression, TRUE);
	} else if (Z_TYPE_P(fields) == IS_ARRAY) {
		const zval* entry{nullptr};
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(fields), entry) {
			is_expression = false;
			if (Z_TYPE_P(entry) == IS_OBJECT) {
				if (is_a_mysqlx_expression(entry)) {
					/* get the string */
					entry = get_mysqlx_expression(entry);
					is_expression = true;
				}
			}
			/* NO else */
			if (Z_TYPE_P(entry) != IS_STRING) {
				RAISE_EXCEPTION(err_msg_wrong_param_1);
				DBG_VOID_RETURN;
			}
			MYSQLND_CSTRING field_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
			ret = xmysqlnd_crud_collection_find__set_fields(find_op, field_str, is_expression, TRUE);
			if(ret==FAIL)
				break;
		} ZEND_HASH_FOREACH_END();
	}
	if (FAIL == ret) {
		RAISE_EXCEPTION(err_msg_add_field);
	}
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_find::add_operation */
void Collection_find::add_operation(
	Collection_find::Operation operation,
	zval* sort_expr,
	int num_of_expr,
	zval* return_value)
{
	DBG_ENTER("Collection_find::add_operation");

	for( int i{0}; i < num_of_expr ; ++i ) {
		if (Z_TYPE(sort_expr[i]) != IS_STRING &&
			Z_TYPE(sort_expr[i]) != IS_OBJECT &&
			Z_TYPE(sort_expr[i]) != IS_ARRAY) {
			php_error_docref(nullptr, E_WARNING, "Only strings, objects and arrays can be added. Type is %u",
							 Z_TYPE(sort_expr[i]));
			DBG_VOID_RETURN;
		}
	}

	RETVAL_FALSE;

	for( int i{0}; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i])) {
		case IS_STRING:
			{
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL(sort_expr[i]), Z_STRLEN(sort_expr[i]) };
				if (Collection_find::Operation::Sort == operation) {
					if (PASS == xmysqlnd_crud_collection_find__add_sort(find_op, sort_expr_str)) {
						ZVAL_COPY(return_value, object_zv);
					}
				} else if (Collection_find::Operation::Group_by == operation) {
					if (PASS == xmysqlnd_crud_collection_find__add_grouping(find_op, sort_expr_str)) {
						ZVAL_COPY(return_value, object_zv);
					}
				}
			}
			break;
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					enum_func_status ret{FAIL};
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (Collection_find::Operation::Sort == operation) {
						ret = xmysqlnd_crud_collection_find__add_sort(find_op, sort_expr_str);
					} else if (Collection_find::Operation::Group_by == operation) {
						ret = xmysqlnd_crud_collection_find__add_grouping(find_op, sort_expr_str);
					}
					if (FAIL == ret) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
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


/* {{{ proto mixed mysqlx_node_collection__find::sort() */
void Collection_find::sort(
	zval* sort_expr,
	int num_of_expr,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::sort");
	add_operation(Operation::Sort, sort_expr, num_of_expr, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::group_by() */
void Collection_find::group_by(
	zval* sort_expr,
	int num_of_expr,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::group_by");
	add_operation(Operation::Group_by, sort_expr, num_of_expr, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::having() */
void Collection_find::having(
	const MYSQLND_CSTRING& search_condition,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::having");

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_find__set_having(find_op, search_condition)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::limit() */
void Collection_find::limit(
	zend_long rows,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_find__set_limit(find_op, rows)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::skip() */
void Collection_find::skip(
	zend_long position,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::skip");

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_find__set_skip(find_op, position)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::bind() */
void Collection_find::bind(
	HashTable* bind_variables,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::bind");

	RETVAL_FALSE;

	zend_string* key{nullptr};
	zval* val{nullptr};
	ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
		if (key) {
			const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
			if (FAIL == xmysqlnd_crud_collection_find__bind_value(find_op, variable, val)) {
				RAISE_EXCEPTION(err_msg_bind_fail);
				DBG_VOID_RETURN;
			}
		}
	} ZEND_HASH_FOREACH_END();
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::lock_shared() */
void Collection_find::lock_shared(zval* return_value, int lock_waiting_option)
{
	DBG_ENTER("mysqlx_node_collection__find::lock_shared");

	RETVAL_FALSE;

	if ((xmysqlnd_crud_collection_find__enable_lock_shared(find_op) == PASS)
		&& (xmysqlnd_crud_collection_find_set_lock_waiting_option(find_op, lock_waiting_option) == PASS))
	{
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::lock_exclusive() */
void Collection_find::lock_exclusive(zval* return_value, int lock_waiting_option)
{
	DBG_ENTER("mysqlx_node_collection__find::lock_exclusive");

	RETVAL_FALSE;

	if ((xmysqlnd_crud_collection_find__enable_lock_exclusive(find_op) == PASS)
		&& (xmysqlnd_crud_collection_find_set_lock_waiting_option(find_op, lock_waiting_option) == PASS))
	{
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
void Collection_find::execute(zval* return_value)
{
	execute(MYSQLX_EXECUTE_FLAG_BUFFERED, return_value);
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
void Collection_find::execute(
	zend_long flags,
	zval* return_value)
{
	DBG_ENTER("mysqlx_node_collection__find::execute");

	RETVAL_FALSE;

	if (FALSE == xmysqlnd_crud_collection_find__is_initialized(find_op)) {
		RAISE_EXCEPTION(err_msg_find_fail);
	} else {
		XMYSQLND_NODE_STMT* stmt = collection->data->m.find(collection, find_op);
		{
			if (stmt) {
				zval stmt_zv;
				ZVAL_UNDEF(&stmt_zv);
				mysqlx_new_node_stmt(&stmt_zv, stmt);
				if (Z_TYPE(stmt_zv) == IS_NULL) {
					xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
				}
				if (Z_TYPE(stmt_zv) == IS_OBJECT) {
					zval zv;
					ZVAL_UNDEF(&zv);
					mysqlx_node_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv), flags, MYSQLX_RESULT_DOC, &zv);

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


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
Mysqlx::Crud::Find* Collection_find::get_stmt()
{
	if (!find_op
		|| (xmysqlnd_crud_collection_find__finalize_bind(find_op) == FAIL)
		|| !xmysqlnd_crud_collection_find__is_initialized(find_op))
	{
		throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
	}

	st_xmysqlnd_pb_message_shell msg_shell = xmysqlnd_crud_collection_find__get_protobuf_message(find_op);
	Mysqlx::Crud::Find* msg = static_cast<Mysqlx::Crud::Find*>(msg_shell.message);
	return msg;
}
/* }}} */

//------------------------------------------------------------------------------


/* {{{ mysqlx_node_collection__find::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, __construct)
{
}
/* }}} */

/* {{{ mysqlx_node_collection__find::fields */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, fields)
{
	zval* object_zv{nullptr};
	const zval* fields{nullptr};

	DBG_ENTER("mysqlx_node_collection__find::fields");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oz",
												&object_zv, collection_find_class_entry,
												(zval *) &fields))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.fields(fields, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__find__add_sort_or_grouping */
static void
mysqlx_node_collection__find__add_sort_or_grouping(
	INTERNAL_FUNCTION_PARAMETERS,
	Collection_find::Operation op_type)
{
	DBG_ENTER("mysqlx_node_collection__find__add_sort_or_grouping");

	zval* object_zv{nullptr};
	zval* sort_expr{nullptr};
	int num_of_expr{0};

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
									&object_zv,
									collection_find_class_entry,
									&sort_expr,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.add_operation(op_type, sort_expr, num_of_expr, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::sort() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, sort)
{
	DBG_ENTER("mysqlx_node_collection__find::sort");
	mysqlx_node_collection__find__add_sort_or_grouping(
		INTERNAL_FUNCTION_PARAM_PASSTHRU,
		Collection_find::Operation::Sort);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::groupBy() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, groupBy)
{
	DBG_ENTER("mysqlx_node_collection__find::groupBy");
	mysqlx_node_collection__find__add_sort_or_grouping(
		INTERNAL_FUNCTION_PARAM_PASSTHRU,
		Collection_find::Operation::Group_by);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::having() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, having)
{
	zval* object_zv{nullptr};
	MYSQLND_CSTRING search_condition = {nullptr, 0};

	DBG_ENTER("mysqlx_node_collection__find::having");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
												&object_zv, collection_find_class_entry,
												&(search_condition.s), &(search_condition.l)))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.having(search_condition, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, limit)
{
	zval* object_zv{nullptr};
	zend_long rows{0};

	DBG_ENTER("mysqlx_node_collection__find::limit");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, collection_find_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.limit(rows, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::skip() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, skip)
{
	zval* object_zv{nullptr};
	zend_long position{0};

	DBG_ENTER("mysqlx_node_collection__find::skip");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
												&object_zv, collection_find_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.skip(position, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, bind)
{
	zval* object_zv{nullptr};
	HashTable* bind_variables{nullptr};

	DBG_ENTER("mysqlx_node_collection__find::bind");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oh",
												&object_zv, collection_find_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.bind(bind_variables, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::lockShared() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, lockShared)
{
	DBG_ENTER("mysqlx_node_collection__find::lockShared");

	zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
		&object_zv, collection_find_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.lock_shared(return_value, static_cast<int>(lock_waiting_option));

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::lockExclusive() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, lockExclusive)
{
	DBG_ENTER("mysqlx_node_collection__find::lockExclusive");

	zval* object_zv{nullptr};
	zend_long lock_waiting_option{MYSQLX_LOCK_DEFAULT};
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
		&object_zv, collection_find_class_entry,
		&lock_waiting_option))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.lock_exclusive(return_value, static_cast<int>(lock_waiting_option));

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__find::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__find, execute)
{
	zval* object_zv{nullptr};
	zend_long flags{MYSQLX_EXECUTE_FLAG_BUFFERED};

	DBG_ENTER("mysqlx_node_collection__find::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O|l",
												&object_zv, collection_find_class_entry,
												&flags))
	{
		DBG_VOID_RETURN;
	}

	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	coll_find.execute(flags, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__find_methods[] */
static const zend_function_entry mysqlx_node_collection__find_methods[] = {
	PHP_ME(mysqlx_node_collection__find, __construct, nullptr, ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__find, fields, arginfo_mysqlx_node_collection__find__fields, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, groupBy, arginfo_mysqlx_node_collection__find__group_by, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, having, arginfo_mysqlx_node_collection__find__having, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, bind, arginfo_mysqlx_node_collection__find__bind, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, sort, arginfo_mysqlx_node_collection__find__sort, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, limit, arginfo_mysqlx_node_collection__find__limit, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, skip, arginfo_mysqlx_node_collection__find__skip, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, lockShared, arginfo_mysqlx_node_collection__find__lock_shared, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, lockExclusive, arginfo_mysqlx_node_collection__find__lock_exclusive, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_collection__find, execute, arginfo_mysqlx_node_collection__find__execute, ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers collection_find_handlers;
static HashTable collection_find_properties;

const st_mysqlx_property_entry collection_find_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_collection__find_free_storage */
static void
mysqlx_node_collection__find_free_storage(zend_object* object)
{
	util::free_object<Collection_find>(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__find_object_allocator */
static zend_object *
php_mysqlx_node_collection__find_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__find_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_find>(
		class_type,
		&collection_find_handlers,
		&collection_find_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__find_class */
void
mysqlx_register_node_collection__find_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_find_class_entry,
		"CollectionFind",
		mysqlx_std_object_handlers,
		collection_find_handlers,
		php_mysqlx_node_collection__find_object_allocator,
		mysqlx_node_collection__find_free_storage,
		mysqlx_node_collection__find_methods,
		collection_find_properties,
		collection_find_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__find_class */
void
mysqlx_unregister_node_collection__find_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_find_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__find */
void
mysqlx_new_node_collection__find(
	zval * return_value,
	const util::string_view& search_expression,
	drv::st_xmysqlnd_node_collection* collection)
{
	DBG_ENTER("mysqlx_new_node_collection__find");
	if (SUCCESS == object_init_ex(return_value, collection_find_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		Collection_find* const coll_find = static_cast<Collection_find*>(mysqlx_object->ptr);
		if (!coll_find || !coll_find->init(return_value, collection, search_expression)) {
			DBG_ERR("Error");
			php_error_docref(nullptr, E_WARNING, "invalid coll_find of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ get_stmt_from_collection_find */
Mysqlx::Crud::Find* get_stmt_from_collection_find(zval* object_zv)
{
	Collection_find& coll_find = util::fetch_data_object<Collection_find>(object_zv);
	return coll_find.get_stmt();
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
