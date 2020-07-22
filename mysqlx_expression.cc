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
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_expression.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zend_class_entry * mysqlx_expression_class_entry;

struct st_mysqlx_expression : public util::custom_allocable
{
	~st_mysqlx_expression()
	{
		zval_ptr_dtor(&expression);
	}
	zval expression;
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_expression__construct, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, expression, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_expression, __construct)
{
	UNUSED(return_value);
	zval* object_zv{nullptr};
	util::param_string expression;

	DBG_ENTER("mysqlx_expression::__construct");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "Os",
												&object_zv, mysqlx_expression_class_entry,
												&(expression.str), &(expression.len)))
	{
		DBG_VOID_RETURN;
	}

	{
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(object_zv);
		st_mysqlx_expression* object = (st_mysqlx_expression*) mysqlx_object->ptr;
		if (!object) {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			DBG_VOID_RETURN;
		}
		DBG_INF_FMT("expression=[%*s]", expression.length(), expression.data());
		ZVAL_STRINGL(&object->expression, expression.data(), expression.length());
	}


	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_expression_methods[] = {
	PHP_ME(mysqlx_expression, __construct,		arginfo_mysqlx_expression__construct,	ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

MYSQL_XDEVAPI_PHP_FUNCTION(mysql_xdevapi__expression)
{
	util::param_string expression;

	DBG_ENTER("mysql_xdevapi__Expression");
	if (FAILURE == util::zend::parse_function_parameters(execute_data, "s",
										 &expression.str, &expression.len))
	{
		DBG_VOID_RETURN;
	}
	mysqlx_new_expression(return_value, expression.to_view());

	DBG_VOID_RETURN;
}

static zend_object_handlers mysqlx_object_expression_handlers;
static HashTable mysqlx_expression_properties;

const st_mysqlx_property_entry mysqlx_expression_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_expression_free_storage(zend_object* object)
{
	util::free_object<st_mysqlx_expression>(object);
}

static zend_object *
php_mysqlx_expression_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_expression_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_expression>(
		class_type,
		&mysqlx_object_expression_handlers,
		&mysqlx_expression_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_expression_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_expression_class_entry,
		"Expression",
		mysqlx_std_object_handlers,
		mysqlx_object_expression_handlers,
		php_mysqlx_expression_object_allocator,
		mysqlx_expression_free_storage,
		mysqlx_expression_methods,
		mysqlx_expression_properties,
		mysqlx_expression_property_entries);

	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_expression_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
}

void
mysqlx_unregister_expression_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_expression_properties);
}

void
mysqlx_new_expression(zval* return_value, const util::string_view& expression)
{
	DBG_ENTER("mysqlx_new_expression");

	st_mysqlx_expression& data_object{
		util::init_object<st_mysqlx_expression>(mysqlx_expression_class_entry, return_value) };
	ZVAL_STRINGL(&data_object.expression, expression.data(), expression.length());

	DBG_VOID_RETURN;
}

zend_bool
is_a_mysqlx_expression(const zval * const value)
{
	return (instanceof_function(Z_OBJCE_P(value), mysqlx_expression_class_entry));
}

zval*
get_mysqlx_expression(const zval * const object_zv)
{
	zval* ret{nullptr};
	DBG_ENTER("get_mysqlx_expression");
	if (instanceof_function(Z_OBJCE_P(object_zv), mysqlx_expression_class_entry)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(object_zv);
		st_mysqlx_expression* object = (st_mysqlx_expression*) mysqlx_object->ptr;
		if (!object) {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
		} else {
			ret = &object->expression;
		}

	}
	DBG_RETURN(ret);
}

} // namespace devapi

} // namespace mysqlx
