/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
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
#include "util/value.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry* collection_modify_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__sort, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, sort_expressions)
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
	ZEND_ARG_INFO(no_pass_by_ref, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__replace, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, collection_field, IS_STRING, dont_allow_null)
	ZEND_ARG_INFO(no_pass_by_ref, expression_or_literal)
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__modify__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


/* {{{ Collection_modify::init() */
bool Collection_modify::init(
	xmysqlnd_collection* coll,
	const util::string_view& search_expression)
{
	if (!coll || search_expression.empty()) return false;

	collection = coll->get_reference();
	modify_op = xmysqlnd_crud_collection_modify__create(
		mnd_str2c(collection->get_schema()->get_name()),
		mnd_str2c(collection->get_name()));

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
bool Collection_modify::sort(
	zval* sort_expressions,
	int num_of_expr)
{
	DBG_ENTER("Collection_modify::sort");

	if (!sort_expressions) {
		DBG_RETURN(false);
	}

	for( int i{0}; i < num_of_expr; ++i ) {
		const util::zvalue sort_expr(sort_expressions[i]);
		switch (sort_expr.type()) {
		case util::zvalue::Type::String:
			{
				const MYSQLND_CSTRING sort_expr_str{ sort_expr.c_str(), sort_expr.length() };
				if (FAIL == xmysqlnd_crud_collection_modify__add_sort(modify_op,
															sort_expr_str)) {
					RAISE_EXCEPTION(err_msg_add_sort_fail);
					DBG_RETURN(false);
				}
			}
			break;
		case util::zvalue::Type::Array:
			{
				for (auto it{ sort_expr.vbegin() }; it != sort_expr.vend(); ++it) {
					const util::zvalue& sort_expr_entry(*it);
					if (!sort_expr_entry.is_string()) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_RETURN(false);
					}
					const MYSQLND_CSTRING sort_expr_str{
						sort_expr_entry.c_str(), sort_expr_entry.length() };
					if (FAIL == xmysqlnd_crud_collection_modify__add_sort(modify_op,
															sort_expr_str)) {
						RAISE_EXCEPTION(err_msg_add_sort_fail);
						DBG_RETURN(false);
					}
				}
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			DBG_RETURN(false);
			break;
		}
	}
	DBG_RETURN(true);
}
/* }}} */


/* {{{ Collection_modify::limit() */
bool Collection_modify::limit(zend_long rows)
{
	DBG_ENTER("Collection_modify::limit");

	if (rows < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(PASS == xmysqlnd_crud_collection_modify__set_limit(modify_op, rows));
}
/* }}} */


/* {{{ Collection_modify::skip() */
bool Collection_modify::skip(zend_long position)
{
	DBG_ENTER("Collection_modify::skip");

	if (position < 0) {
		RAISE_EXCEPTION(err_msg_wrong_param_2);
		DBG_RETURN(false);
	}

	DBG_RETURN(PASS == xmysqlnd_crud_collection_modify__set_skip(modify_op, position));
}
/* }}} */


/* {{{ Collection_modify::bind() */
bool Collection_modify::bind(const util::zvalue& bind_variables)
{
	DBG_ENTER("Collection_modify::bind");

	for (const auto& variable_value : bind_variables) {
		const util::zvalue& var_name{ variable_value.first };
		if (!var_name.is_string()) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
		const MYSQLND_CSTRING variable{ var_name.c_str(), var_name.length() };
		const util::zvalue& var_value{ variable_value.second };
		if (FAIL == xmysqlnd_crud_collection_modify__bind_value(modify_op, variable, var_value.ptr())) {
			RAISE_EXCEPTION(err_msg_bind_fail);
			DBG_RETURN(false);
		}
	}

	DBG_RETURN(true);
}
/* }}} */


/* {{{ Collection_modify::add_operation */
bool Collection_modify::add_operation(
	Operation operation,
	const util::string_view& path,
	const bool is_document,
	util::zvalue value)
{
	DBG_ENTER("Collection_modify::add_operation");

	zend_bool is_expression{FALSE};

	switch (value.type()) {
		case util::zvalue::Type::Object:
			if (operation == Collection_modify::Operation::Set) {
				if (is_a_mysqlx_expression(value.ptr())) {
					/* get the string */
					value = get_mysqlx_expression(value.ptr());
					is_expression = TRUE;
				}
				break;
			}
			/* fall-through */
		case util::zvalue::Type::String:
		case util::zvalue::Type::Double:
		case util::zvalue::Type::True:
		case util::zvalue::Type::False:
		case util::zvalue::Type::Long:
		case util::zvalue::Type::Null:
			break;

		case IS_ARRAY:
			value = util::json::to_zv_string(value);
			break;

		default:{
			RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_RETURN(false);
		}
	}

	enum_func_status ret{FAIL};
	const MYSQLND_CSTRING& path_nd = path.to_nd_cstr();
	switch (operation) {
		case Operation::Set:
			ret = xmysqlnd_crud_collection_modify__set(modify_op, path_nd, value.ptr(), is_expression, is_document);
			break;

		case Operation::Replace:
			ret = xmysqlnd_crud_collection_modify__replace(modify_op, path_nd, value.ptr(), is_expression, is_document);
			break;

		case Operation::Array_insert:
			ret = xmysqlnd_crud_collection_modify__array_insert(modify_op, path_nd, value.ptr());
			break;

		case Operation::Array_append:
			ret = xmysqlnd_crud_collection_modify__array_append(modify_op, path_nd, value.ptr());
			break;

		default:
			assert(!"unknown collection_modify field operation");
	}

	DBG_RETURN(ret == PASS);
}
/* }}} */


/* {{{ Collection_modify::set() */
bool Collection_modify::set(
	const util::string_view& path,
	const bool is_document,
	zval* value)
{
	DBG_ENTER("Collection_modify::set");
	DBG_RETURN(add_operation(Operation::Set, path, is_document, value));
}
/* }}} */


/* {{{ Collection_modify::unset() */
bool Collection_modify::unset(
	zval* variables,
	int num_of_variables)
{
	DBG_ENTER("Collection_modify::unset");

	if (num_of_variables <= 0) {
		DBG_RETURN(false);
	}

	for (int i{0}; i < num_of_variables; ++i) {
		const util::zvalue variable(variables[i]);
		switch (variable.type())
		{
		case util::zvalue::Type::String:
			{
				const MYSQLND_CSTRING var{ variable.c_str(), variable.length() };
				if (FAIL == xmysqlnd_crud_collection_modify__unset(modify_op, var)) {
					RAISE_EXCEPTION(err_msg_unset_fail);
					DBG_RETURN(false);
				}
			}
			break;
		case util::zvalue::Type::Array:
			{
				for (auto it{ variable.vbegin() }; it != variable.vend(); ++it) {
					const util::zvalue& variable_entry(*it);
					if (!variable_entry.is_string()) {
						RAISE_EXCEPTION(err_msg_wrong_param_1);
						DBG_RETURN(false);
					}
					const MYSQLND_CSTRING variable_str{
						variable_entry.c_str(), variable_entry.length() };
					if (FAIL == xmysqlnd_crud_collection_modify__unset(modify_op, variable_str)) {
						RAISE_EXCEPTION(err_msg_unset_fail);
						DBG_RETURN(false);
					}
				}
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_wrong_param_3);
			DBG_RETURN(false);
			break;
		}
	}

	DBG_RETURN(true);
}
/* }}} */


/* {{{ Collection_modify::replace() */
bool Collection_modify::replace(
	const util::string_view& path,
	zval* value)
{
	DBG_ENTER("Collection_modify::replace");
	DBG_RETURN(add_operation(Operation::Replace, path, false, value));
}
/* }}} */


/* {{{ Collection_modify::patch() */
bool Collection_modify::patch(const util::string_view &document_contents_str)
{
	DBG_ENTER("Collection_modify::patch");

	const MYSQLND_CSTRING Empty_doc_path{ nullptr, 0 };
	util::zvalue document_contents(document_contents_str);
	if (FAIL == xmysqlnd_crud_collection_modify__patch(modify_op, Empty_doc_path, document_contents.ptr())) {
		RAISE_EXCEPTION(err_msg_merge_fail);
		DBG_RETURN(false);
	}

	DBG_RETURN(true);
}
/* }}} */


/* {{{ Collection_modify::arrayInsert() */
bool Collection_modify::arrayInsert(
	const util::string_view& path,
	zval* value)
{
	DBG_ENTER("Collection_modify::arrayInsert");
	DBG_RETURN(add_operation(Operation::Array_insert, path, true, value));
}
/* }}} */


/* {{{ Collection_modify::arrayAppend() */
bool Collection_modify::arrayAppend(
	const util::string_view& path,
	zval* value)
{
	DBG_ENTER("Collection_modify::arrayAppend");
	DBG_RETURN(add_operation(Operation::Array_append, path, false, value));
}
/* }}} */


/* {{{ Collection_modify::execute() */
void Collection_modify::execute(zval* resultset)
{
	DBG_ENTER("Collection_modify::execute");

	DBG_INF_FMT("modify_op=%p collection=%p", modify_op, collection);
	if (FALSE == xmysqlnd_crud_collection_modify__is_initialized(modify_op)) {
		RAISE_EXCEPTION(err_msg_modify_fail);
	} else {
		xmysqlnd_stmt* stmt = collection->modify(modify_op);
		if (stmt) {
			util::zvalue stmt_zv;
			mysqlx_new_stmt(stmt_zv.ptr(), stmt);

			if (stmt_zv.is_null()) {
				xmysqlnd_stmt_free(stmt, nullptr, nullptr);
			} else if (stmt_zv.is_object()) {
				zend_long flags{0};
				mysqlx_statement_execute_read_response(
					Z_MYSQLX_P(stmt_zv.ptr()), flags, MYSQLX_RESULT, resultset);
			}
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


//------------------------------------------------------------------------------


/* {{{ mysqlx_collection__modify::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::sort() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, sort)
{
	DBG_ENTER("mysqlx_collection__modify::sort");

	zval* object_zv{nullptr};
	zval* sort_expressions{nullptr};
	int num_of_expr{0};

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
									&object_zv,
									collection_modify_class_entry,
									&sort_expressions,
									&num_of_expr))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.sort(sort_expressions, num_of_expr)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::limit() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, limit)
{
	DBG_ENTER("mysqlx_collection__modify::limit");

	zval* object_zv{nullptr};
	zend_long rows{0};

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Ol",
												&object_zv, collection_modify_class_entry,
												&rows))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.limit(rows)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::skip() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, skip)
{
	DBG_ENTER("mysqlx_collection__modify::skip");

	zval* object_zv{nullptr};
	zend_long position{0};

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
	if (coll_modify.skip(position)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::bind() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, bind)
{
	DBG_ENTER("mysqlx_collection__modify::bind");

	zval* object_zv{nullptr};
	zval* bind_vars{nullptr};

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Oz",
												&object_zv, collection_modify_class_entry,
												&bind_vars))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	util::zvalue bind_variables(bind_vars);
	if (coll_modify.bind(bind_variables)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

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
	DBG_ENTER("mysqlx_collection__modify__2_param_op");

	zval* object_zv{nullptr};
	util::string_view path;
	zval* value{nullptr};

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Osz",
												&object_zv, collection_modify_class_entry,
												&(path.str), &(path.len),
												&value))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.add_operation(operation, path, is_document, value)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

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

/* {{{ proto mixed mysqlx_collection__modify::patch() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, patch)
{
	DBG_ENTER("mysqlx_collection__modify::patch");

	zval* object_zv{nullptr};
	util::string_view document_contents;

	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os",
		&object_zv, collection_modify_class_entry,
		&(document_contents.str), &(document_contents.len)))
	{
		DBG_VOID_RETURN;
	}

	Collection_modify& coll_modify = util::fetch_data_object<Collection_modify>(object_zv);
	if (coll_modify.patch(document_contents)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

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


/* {{{ proto mixed mysqlx_collection__modify::unset() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, unset)
{
	DBG_ENTER("mysqlx_collection__modify::unset");

	zval* object_zv{nullptr};
	zval* variables{nullptr};
	int num_of_variables{0};

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
	if (coll_modify.unset(variables, num_of_variables)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto mixed mysqlx_collection__modify::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__modify, execute)
{
	DBG_ENTER("mysqlx_collection__modify::execute");

	zval* object_zv{nullptr};

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
	PHP_ME(mysqlx_collection__modify,	patch,		arginfo_mysqlx_collection__modify__patch,			ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayInsert,arginfo_mysqlx_collection__modify__array_insert,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__modify,	arrayAppend,arginfo_mysqlx_collection__modify__array_append,	ZEND_ACC_PUBLIC)

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
mysqlx_register_collection__modify_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
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
mysqlx_unregister_collection__modify_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_modify_properties);
}
/* }}} */


/* {{{ mysqlx_new_collection__modify */
void
mysqlx_new_collection__modify(
	zval* return_value,
	const util::string_view& search_expression,
	xmysqlnd_collection* collection)
{
	DBG_ENTER("mysqlx_new_collection__modify");

	if (SUCCESS == object_init_ex(return_value, collection_modify_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		Collection_modify* const coll_modify = static_cast<Collection_modify*>(mysqlx_object->ptr);
		if (!coll_modify || !coll_modify->init(collection, search_expression)) {
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
