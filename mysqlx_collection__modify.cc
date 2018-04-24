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
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_skippable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__modify.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry* collection_modify_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__limit, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, rows, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__skip, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, position, IS_LONG, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__bind, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, placeholder_values, IS_ARRAY, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__set, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__unset, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, variables)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__replace, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__merge, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, document, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__patch, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, document, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__array_insert, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__array_append, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__array_delete, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


/* {{{ Collection_modify::init() */
bool Collection_modify::init(
	zval* obj_zv,
	XMYSQLND_COLLECTION* coll,
	const util::string_view& search_expression)
{
	if (!obj_zv || !coll || search_expression.empty()) return false;

	object_zv = obj_zv;
	collection = coll->data->m.get_reference(coll);
	modify_op = xmysqlnd_crud_collection_modify__create(
		mnd_str2c(collection->data->schema->data->schema_name),
		mnd_str2c(collection->data->collection_name));

	if (!modify_op) return false;

	return xmysqlnd_crud_collection_modify__set_criteria(
		modify_op, search_expression.to_std_string()) == PASS;
}
/* }}} */


/* {{{ Collection_modify::~Collection_modify() */
Collection_modify::~Collection_modify()
{
	if (modify_op) {
		xmysqlnd_crud_collection_modify__destroy(modify_op);
	}

	if (collection) {
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
}
/* }}} */


/* {{{ Collection_modify::sort() */
void Collection_modify::sort(
	zval* sort_expr,
	int num_of_expr,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::sort");

	if (!sort_expr) {
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	for( int i{0}; i < num_of_expr ; ++i ) {
		switch (Z_TYPE(sort_expr[i])) {
		case IS_STRING:
			{
				const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL(sort_expr[i]),
												Z_STRLEN(sort_expr[i]) };
				if (PASS == xmysqlnd_crud_collection_modify__add_sort(modify_op,
															sort_expr_str)) {
					ZVAL_COPY(return_value, object_zv);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sort_expr[i]), entry) {
					const MYSQLND_CSTRING sort_expr_str = { Z_STRVAL_P(entry),
													Z_STRLEN_P(entry) };
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					if (FAIL == xmysqlnd_crud_collection_modify__add_sort(modify_op,
														sort_expr_str)) {
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


/* {{{ Collection_modify::limit() */
void Collection_modify::limit(
	zend_long rows,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_modify__set_limit(modify_op, rows)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::skip() */
void Collection_modify::skip(
	zend_long position,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::skip");

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	if (PASS == xmysqlnd_crud_collection_modify__set_skip(modify_op, position)) {
		ZVAL_COPY(return_value, object_zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::bind() */
void Collection_modify::bind(
	HashTable* bind_variables,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::bind");

	RETVAL_FALSE;

	zend_string* key{nullptr};
	zval* val{nullptr};
	ZEND_HASH_FOREACH_STR_KEY_VAL(bind_variables, key, val) {
		if (key) {
			const MYSQLND_CSTRING variable = { ZSTR_VAL(key), ZSTR_LEN(key) };
			if (FAIL == xmysqlnd_crud_collection_modify__bind_value(modify_op, variable, val)) {
				RAISE_EXCEPTION(err_msg_bind_fail);
				DBG_VOID_RETURN;
			}
		}
	} ZEND_HASH_FOREACH_END();
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::add_operation */
void Collection_modify::add_operation(
	Operation operation,
	const util::string_view& path,
	const bool is_document,
	zval* raw_value,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::add_operation");

	RETVAL_FALSE;

	zend_bool is_expression{FALSE};

	zval converted_value;
	ZVAL_UNDEF(&converted_value);

	const zval* value = raw_value;

	switch (Z_TYPE_P(value)) {
		case IS_OBJECT:
			if (operation == Collection_modify::Operation::Set) {
				if (is_a_mysqlx_expression(value)) {
					/* get the string */
					value = get_mysqlx_expression(value);
					is_expression = TRUE;
				}
				break;
			}
			/* fall-through */
		case IS_STRING:
		case IS_DOUBLE:
		case IS_TRUE:
		case IS_FALSE:
		case IS_LONG:
		case IS_NULL:
			break;

		case IS_ARRAY:
			util::json::to_zv_string(raw_value, &converted_value);
			value = &converted_value;
			break;

		default:{
			RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_VOID_RETURN;
		}

	}

	RETVAL_FALSE;

	enum_func_status ret{FAIL};
	const MYSQLND_CSTRING& path_nd = path.to_nd_cstr();
	switch (operation) {
		case Operation::Set:
			ret = xmysqlnd_crud_collection_modify__set(modify_op, path_nd, value, is_expression, is_document);
			break;

		case Operation::Replace:
			ret = xmysqlnd_crud_collection_modify__replace(modify_op, path_nd, value, is_expression, is_document);
			break;

		case Operation::Array_insert:
			ret = xmysqlnd_crud_collection_modify__array_insert(modify_op, path_nd, value);
			break;

		case Operation::Array_append:
			ret = xmysqlnd_crud_collection_modify__array_append(modify_op, path_nd, value);
			break;

		default:
			assert(!"unknown collection_modify field operation");
	}

	if (PASS == ret) {
		ZVAL_COPY(return_value, object_zv);
	}

	zval_dtor(&converted_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::set() */
void Collection_modify::set(
	const util::string_view& path,
	const bool is_document,
	zval* value,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::set");
	add_operation(Operation::Set, path, is_document, value, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::unset() */
void Collection_modify::unset(
	zval* variables,
	int num_of_variables,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::unset");

	RETVAL_FALSE;

	if (num_of_variables <= 0) {
		DBG_VOID_RETURN;
	}

	for (int i{0}; i < num_of_variables; ++i) {
		switch (Z_TYPE(variables[i]))
		{
		case IS_STRING:
			{
				const MYSQLND_CSTRING variable = { Z_STRVAL(variables[i]),
										Z_STRLEN(variables[i]) };
				if (FAIL == xmysqlnd_crud_collection_modify__unset(modify_op,
																variable)) {
						RAISE_EXCEPTION(err_msg_unset_fail);
				}
			}
			break;
		case IS_ARRAY:
			{
				zval* entry{nullptr};
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(variables[i]), entry) {
					if (Z_TYPE_P(entry) != IS_STRING) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_VOID_RETURN;
					}
					const MYSQLND_CSTRING variable = { Z_STRVAL_P(entry),
											Z_STRLEN_P(entry) };
					if (FAIL == xmysqlnd_crud_collection_modify__unset(modify_op,
																	variable)) {
						RAISE_EXCEPTION(err_msg_unset_fail);
						DBG_VOID_RETURN;
					}
				} ZEND_HASH_FOREACH_END();
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			break;
		}
	}

	ZVAL_COPY(return_value, object_zv);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::replace() */
void Collection_modify::replace(
	const util::string_view& path,
	zval* value,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::replace");
	add_operation(Operation::Replace, path, false, value, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::merge() */
void Collection_modify::merge(
	const util::string_view& document_contents,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::merge");

	RETVAL_FALSE;

	MYSQLND_CSTRING emptyDocPath = { nullptr, 0 };
	zval zvDocumentContents;
	document_contents.to_zval(&zvDocumentContents);
	if (FAIL == xmysqlnd_crud_collection_modify__merge(modify_op, emptyDocPath, &zvDocumentContents)) {
		RAISE_EXCEPTION(err_msg_merge_fail);
	}
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::patch() */
void Collection_modify::patch(
	const util::string_view &document_contents,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::patch");

	RETVAL_FALSE;

	MYSQLND_CSTRING emptyDocPath = { nullptr, 0 };
	zval zvDocumentContents;
	document_contents.to_zval(&zvDocumentContents);
	if (FAIL == xmysqlnd_crud_collection_modify__patch(modify_op, emptyDocPath, &zvDocumentContents)) {
		RAISE_EXCEPTION(err_msg_merge_fail);
	}
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::arrayInsert() */
void Collection_modify::arrayInsert(
	const util::string_view& path,
	zval* value,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::arrayInsert");
	add_operation(Operation::Array_insert, path, true, value, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::arrayAppend() */
void Collection_modify::arrayAppend(
	const util::string_view& path,
	zval* value,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::arrayAppend");
	add_operation(Operation::Array_append, path, false, value, return_value);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::arrayDelete() */
void Collection_modify::arrayDelete(
	const util::string_view& array_index_path,
	zval* return_value)
{
	DBG_ENTER("Collection_modify::arrayDelete");

	RETVAL_FALSE;

	if (FAIL == xmysqlnd_crud_collection_modify__array_delete(modify_op, array_index_path.to_nd_cstr())) {
		RAISE_EXCEPTION(err_msg_arridx_del_fail);
	}
	ZVAL_COPY(return_value, object_zv);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Collection_modify::execute() */
void Collection_modify::execute(
	zval* return_value)
{
	DBG_ENTER("Collection_modify::execute");

	RETVAL_FALSE;

	DBG_INF_FMT("modify_op=%p collection=%p", modify_op, collection);
	if (FALSE == xmysqlnd_crud_collection_modify__is_initialized(modify_op)) {
		RAISE_EXCEPTION(err_msg_modify_fail);
	} else {
		XMYSQLND_STMT* stmt = collection->data->m.modify(collection, modify_op);
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

	DBG_VOID_RETURN;
}
/* }}} */


//------------------------------------------------------------------------------


/* {{{ mysqlx_collection__modify::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::sort() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, sort)
{
	zval* object_zv{nullptr};
	zval* sort_expr{nullptr};
	int num_of_expr{0};

	DBG_ENTER("mysqlx_collection__modify::sort");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
									&object_zv,
									collection_modify_class_entry,
									&sort_expr,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.sort(sort_expr, num_of_expr, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, limit)
{
	zval* object_zv{nullptr};
	zend_long rows{0};

	DBG_ENTER("mysqlx_collection__modify::limit");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
												&object_zv, collection_modify_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.limit(rows, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::skip() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, skip)
{
	zval* object_zv{nullptr};
	zend_long position{0};

	DBG_ENTER("mysqlx_collection__modify::skip");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
												&object_zv, collection_modify_class_entry,
												&position))
	{
		DBG_VOID_RETURN;
	}

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.skip(position, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, bind)
{
	zval* object_zv{nullptr};
	HashTable* bind_variables{nullptr};

	DBG_ENTER("mysqlx_collection__modify::bind");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Oh",
												&object_zv, collection_modify_class_entry,
												&bind_variables))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.bind(bind_variables, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_collection__modify__2_param_op */
static void
mysqlx_collection__modify__2_param_op(
	INTERNAL_FUNCTION_PARAMETERS,
	const Collection_modify::Operation operation,
	const bool is_document = false)
{
	zval* object_zv{nullptr};
	zval* value{nullptr};
	util::string_view path;

	DBG_ENTER("mysqlx_collection__modify__2_param_op");

	RETVAL_FALSE;

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Osz",
												&object_zv, collection_modify_class_entry,
												&(path.str), &(path.len),
												&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.add_operation(operation, path, is_document, value, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::set() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, set)
{
	DBG_ENTER("mysqlx_collection__modify::set");
	mysqlx_collection__modify__2_param_op(
		INTERNAL_FUNCTION_PARAM_PASSTHRU, Collection_modify::Operation::Set);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::replace() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, replace)
{
	DBG_ENTER("mysqlx_collection__modify::replace");
	mysqlx_collection__modify__2_param_op(
		INTERNAL_FUNCTION_PARAM_PASSTHRU, Collection_modify::Operation::Replace);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::merge() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, merge)
{
	zval* object_zv{nullptr};
	util::string_view document_contents;

	DBG_ENTER("mysqlx_collection__modify::merge");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, collection_modify_class_entry,
		&(document_contents.str), &(document_contents.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.merge(document_contents, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::patch() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, patch)
{
	zval* object_zv{nullptr};
	util::string_view document_contents;

	DBG_ENTER("mysqlx_collection__modify::patch");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, collection_modify_class_entry,
		&(document_contents.str), &(document_contents.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.patch(document_contents, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::arrayInsert() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, arrayInsert)
{
	DBG_ENTER("mysqlx_collection__modify::arrayInsert");
	mysqlx_collection__modify__2_param_op(
		INTERNAL_FUNCTION_PARAM_PASSTHRU, Collection_modify::Operation::Array_insert, true);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::arrayAppend() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, arrayAppend)
{
	DBG_ENTER("mysqlx_collection__modify::arrayAppend");
	mysqlx_collection__modify__2_param_op(
		INTERNAL_FUNCTION_PARAM_PASSTHRU, Collection_modify::Operation::Array_append);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::arrayDelete() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, arrayDelete)
{
	zval* object_zv{nullptr};
	util::string_view array_index_path;

	DBG_ENTER("mysqlx_collection__modify::arrayDelete");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, collection_modify_class_entry,
		&(array_index_path.str), &(array_index_path.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.arrayDelete(array_index_path, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::unset() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, unset)
{
	zval* object_zv{nullptr};
	zval* variables{nullptr};
	int num_of_variables{0};

	DBG_ENTER("mysqlx_collection__modify::unset");

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "O+",
		&object_zv,
		collection_modify_class_entry,
		&variables,
		&num_of_variables))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.unset(variables, num_of_variables, return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, execute)
{
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_collection__modify::execute");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv, collection_modify_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	coll_modify.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_collection__modify_methods[] */
static const zend_function_entry mysqlx_collection__modify_methods[] = {
	PHP_ME(mysqlx_collection__modify, 	__construct,	nullptr,												ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_collection__modify,	bind,		arginfo_mysqlx_collection__modify__bind,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	sort,		arginfo_mysqlx_collection__modify__sort,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	limit,		arginfo_mysqlx_collection__modify__limit,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	skip,		arginfo_mysqlx_collection__modify__skip,			ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection__modify,	set,		arginfo_mysqlx_collection__modify__set,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	unset,		arginfo_mysqlx_collection__modify__unset,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	replace,	arginfo_mysqlx_collection__modify__replace,		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	merge,		arginfo_mysqlx_collection__modify__merge,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	patch,		arginfo_mysqlx_collection__modify__patch,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayInsert,arginfo_mysqlx_collection__modify__array_insert,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayAppend,arginfo_mysqlx_collection__modify__array_append,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayDelete,arginfo_mysqlx_collection__modify__array_delete,	ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_collection__modify,	execute,	arginfo_mysqlx_collection__modify__execute,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */


static zend_object_handlers collection_modify_handlers;
static HashTable collection_modify_properties;

const st_mysqlx_property_entry collection_modify_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_collection__modify_free_storage */
static void
mysqlx_collection__modify_free_storage(zend_object* object)
{
	util::free_object<Collection_modify>(object);
}
/* }}} */


/* {{{ php_mysqlx_collection__modify_object_allocator */
static zend_object *
php_mysqlx_collection__modify_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__modify_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_modify>(
		class_type,
		&collection_modify_handlers,
		&collection_modify_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_collection__modify_class */
void
mysqlx_register_collection__modify_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_modify_class_entry,
		"CollectionModify",
		mysqlx_std_object_handlers,
		collection_modify_handlers,
		php_mysqlx_collection__modify_object_allocator,
		mysqlx_collection__modify_free_storage,
		mysqlx_collection__modify_methods,
		collection_modify_properties,
		collection_modify_property_entries,
		mysqlx_executable_interface_entry,
		mysqlx_crud_operation_bindable_interface_entry,
		mysqlx_crud_operation_limitable_interface_entry,
		mysqlx_crud_operation_skippable_interface_entry,
		mysqlx_crud_operation_sortable_interface_entry);
}
/* }}} */


/* {{{ mysqlx_unregister_collection__modify_class */
void
mysqlx_unregister_collection__modify_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_modify_properties);
}
/* }}} */


/* {{{ mysqlx_new_collection__modify */
void
mysqlx_new_collection__modify(
	zval* return_value,
	const util::string_view& search_expression,
	XMYSQLND_COLLECTION* collection)
{
	DBG_ENTER("mysqlx_new_collection__modify");

	if (SUCCESS == object_init_ex(return_value, collection_modify_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		Collection_modify* const coll_modify = static_cast<Collection_modify*>(mysqlx_object->ptr);
		if (!coll_modify || !coll_modify->init(return_value, collection, search_expression)) {
			DBG_ERR("Error");
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
