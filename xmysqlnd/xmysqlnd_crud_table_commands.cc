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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#include <ext/json/php_json_parser.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_any2expr.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include <vector>
#include <string>

#include "xmysqlnd_crud_table_commands.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"
#include "mysqlx_expression.h"
#include "mysqlx_exception.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_crud_table__bind_value */
enum_func_status
xmysqlnd_crud_table__bind_value(std::vector<std::string> & placeholders,
									 std::vector<Mysqlx::Datatypes::Scalar*> & bound_values,
									 const MYSQLND_CSTRING & name,
									 zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table__bind_value");
	DBG_INF_FMT("name=%*s", name.l, name.s);

	const std::string var_name(name.s, name.l);
	const std::vector<std::string>::iterator begin = placeholders.begin();
	const std::vector<std::string>::iterator end = placeholders.end();
	const std::vector<std::string>::const_iterator index = std::find(begin, end, var_name);
	if (index == end) {
		DBG_ERR("No such variable in the expression");
		DBG_RETURN(FAIL);
	}

	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(value, any)) {
		DBG_ERR("Error converting the zval to scalar");
		DBG_RETURN(FAIL);
	}
	any2log(any);

	DBG_INF_FMT("offset=%u", index - begin);
	if (bound_values[index - begin]) {
		delete bound_values[index - begin];
	}
	bound_values[index - begin] = any.release_scalar();

	scalar2log(*bound_values[index - begin]);

	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__add_orderby */
enum_func_status
xmysqlnd_crud_table__add_orderby(google::protobuf::RepeatedPtrField< Mysqlx::Crud::Order > * mutable_order,
								   const Mysqlx::Crud::DataModel data_model,
								   const MYSQLND_CSTRING & orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__add_orderby");
	DBG_INF_FMT("orderby=%*s", orderby.l, orderby.s);
	try {
		const std::string source(orderby.s, orderby.l);
		parser::Orderby_parser parser(source, data_model == Mysqlx::Crud::DOCUMENT);
		parser.parse(*mutable_order);
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table__finalize_bind */
enum_func_status
xmysqlnd_crud_table__finalize_bind(google::protobuf::RepeatedPtrField< ::Mysqlx::Datatypes::Scalar >* mutable_args,
										std::vector<Mysqlx::Datatypes::Scalar*> & bound_values)
{
	DBG_ENTER("xmysqlnd_crud_table__finalize_bind");

	const Mysqlx::Datatypes::Scalar* null_value = NULL;
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator begin = bound_values.begin();
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator end = bound_values.end();
	const std::vector<Mysqlx::Datatypes::Scalar*>::const_iterator index = std::find(begin, end, null_value);
	if (index == end) {
		std::vector<Mysqlx::Datatypes::Scalar*>::iterator it = begin;
		for (; it != end; ++it) {
			mutable_args->AddAllocated(*it);
		}
	}
	DBG_RETURN(index == end? PASS : FAIL);
}
/* }}} */


/****************************** TABLE.INSERT() *******************************************************/
struct st_xmysqlnd_crud_table_op__insert
{
	Mysqlx::Crud::Insert message;

	std::vector<std::string> column_names;
	std::vector<zval > rows_zv;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_table_op__insert(
		const MYSQLND_CSTRING & schema,
		const MYSQLND_CSTRING & object_name,
		zval * columns_zv,
		const int num_of_columns)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::TABLE);

		add_columns(columns_zv,num_of_columns);
	}

	~st_xmysqlnd_crud_table_op__insert() {}

	void add_columns(zval * columns_zv, const int num_of_columns);
	void add_column(zval * column_zv);

	void add_row(zval* row_zv);

	void bind_columns();
	void bind_column(const std::string& column_name);

	void bind_rows();
	void bind_row(zval* values_zv, ::Mysqlx::Crud::Insert_TypedRow* row);
	void bind_row_field(zval* value_zv, ::Mysqlx::Crud::Insert_TypedRow* row);

};

/* {{{ st_xmysqlnd_crud_table_op__insert::add_columns */
void st_xmysqlnd_crud_table_op__insert::add_columns(zval * columns_zv,
											const int num_of_columns)
{
	DBG_ENTER("st_xmysqlnd_crud_table_op__insert::add_columns");
	enum_func_status ret = FAIL;
	int i = 0;

	do{
		switch (Z_TYPE(columns_zv[i]))
		{
		case IS_STRING:
			{
				add_column(&columns_zv[i]);
				ret = PASS;
			}
			break;
		case IS_ARRAY:
			{
				zval * entry;
				ret = PASS;
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(columns_zv[i]), entry)
				{
					if (Z_TYPE_P(entry) == IS_STRING) {
						add_column(entry);
					}
					else {
						ret = FAIL;
						break;
					}
				} ZEND_HASH_FOREACH_END();
			}
			break;
		default:
			break;
		}
	} while( ( ++i < num_of_columns ) && ret != FAIL );
	return;
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::add_column */
void st_xmysqlnd_crud_table_op__insert::add_column(zval * column_zv)
{
	const MYSQLND_CSTRING columns_zv_str = {Z_STRVAL_P(column_zv), Z_STRLEN_P(column_zv)};
	const std::string column_name(columns_zv_str.s, columns_zv_str.l);
	column_names.push_back(column_name);
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::add_row */
void st_xmysqlnd_crud_table_op__insert::add_row(zval* row_zv)
{
	zval new_row_zv;
	ZVAL_COPY_VALUE(&new_row_zv, row_zv);
	rows_zv.push_back(new_row_zv);
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::bind_columns */
void st_xmysqlnd_crud_table_op__insert::bind_columns()
{
	for (auto& column_name : column_names)
	{
		bind_column(column_name);
	}
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::bind_column */
void st_xmysqlnd_crud_table_op__insert::bind_column(const std::string& column_name)
{
	Mysqlx::Crud::Column* column = message.add_projection();
	column->set_name(column_name);
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::bind_rows */
void st_xmysqlnd_crud_table_op__insert::bind_rows()
{
	for (auto& values_zv : rows_zv)
	{
		::Mysqlx::Crud::Insert_TypedRow* row = message.add_row();
		bind_row(&values_zv, row);
	}
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::bind_row */
void st_xmysqlnd_crud_table_op__insert::bind_row(zval* values_zv, ::Mysqlx::Crud::Insert_TypedRow* row)
{
	switch (Z_TYPE_P(values_zv))
	{
		case IS_ARRAY: {
			zval * entry;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(values_zv), entry)
			{
				bind_row_field(entry, row);
			} ZEND_HASH_FOREACH_END();
			break;
		}

		default: {
			bind_row_field(values_zv, row);
			break;
		}
	}
}
/* }}} */


/* {{{ st_xmysqlnd_crud_table_op__insert::bind_row_field */
void st_xmysqlnd_crud_table_op__insert::bind_row_field(zval* value_zv, ::Mysqlx::Crud::Insert_TypedRow* row)
{
	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(value_zv, any)) {
		//TODO throw
//		DBG_ERR("Error converting the zval to scalar");
		//DBG_RETURN(FAIL);
	}
	any2log(any);

	Mysqlx::Expr::Expr * field = row->add_field();
	any2expr(any, field);
}
/* }}} */


/****************************** xmysqlnd_crud_table_insert *******************************************************/


/* {{{ xmysqlnd_crud_table_insert__create */
XMYSQLND_CRUD_TABLE_OP__INSERT *
xmysqlnd_crud_table_insert__create(const MYSQLND_CSTRING schema,
							const MYSQLND_CSTRING table_name,
							zval * columns,
							const int num_of_columns)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__create");
	DBG_INF_FMT("schema=%*s table_name=%*s", schema.l, schema.s, table_name.l, table_name.s);
	XMYSQLND_CRUD_TABLE_OP__INSERT * ret = new struct st_xmysqlnd_crud_table_op__insert(schema,
																table_name, columns,num_of_columns);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_insert__destroy */
void
xmysqlnd_crud_table_insert__destroy(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_table_insert__destroy */
enum_func_status
xmysqlnd_crud_table_insert__add_row(XMYSQLND_CRUD_TABLE_OP__INSERT * obj, zval * values_zv)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__add_row");
	enum_func_status ret = PASS;
	obj->add_row(values_zv);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_insert__finalize_bind */
enum_func_status
xmysqlnd_crud_table_insert__finalize_bind(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__finalize_bind");
	const enum_func_status ret = PASS;
	obj->bind_columns();
	obj->bind_rows();
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_insert__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_insert__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_INSERT };
	return ret;
}
/* }}} */


/* {{{ xmysqlnd_crud_table_insert__is_initialized */
zend_bool
xmysqlnd_crud_table_insert__is_initialized(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_insert__finalize_bind");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/****************************** TABLE.DELETE() *******************************************************/
struct st_xmysqlnd_crud_table_op__delete
{
	Mysqlx::Crud::Delete message;

	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_table_op__delete(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::TABLE);
	}

	~st_xmysqlnd_crud_table_op__delete() {}
};


/* {{{ xmysqlnd_crud_table_delete__create */
XMYSQLND_CRUD_TABLE_OP__DELETE *
xmysqlnd_crud_table_delete__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_TABLE_OP__DELETE * ret = new struct st_xmysqlnd_crud_table_op__delete(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__destroy */
void
xmysqlnd_crud_table_delete__destroy(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__set_criteria */
enum_func_status
xmysqlnd_crud_table_delete__set_criteria(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		parser::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * exprCriteria = parser.expr();
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__set_limit */
enum_func_status
xmysqlnd_crud_table_delete__set_limit(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__set_offset */
enum_func_status
xmysqlnd_crud_table_delete__set_offset(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__bind_value */
enum_func_status
xmysqlnd_crud_table_delete__bind_value(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__add_orderby */
enum_func_status
xmysqlnd_crud_table_delete__add_orderby(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__add_orderby");
	const enum_func_status ret = xmysqlnd_crud_table__add_orderby(obj->message.mutable_order(), obj->message.data_model(), orderby);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__is_initialized */
zend_bool
xmysqlnd_crud_table_delete__is_initialized(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_delete__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__finalize_bind */
enum_func_status
xmysqlnd_crud_table_delete__finalize_bind(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	const enum_func_status ret = xmysqlnd_crud_table__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_delete__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_delete__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_DELETE };
	return ret;
}
/* }}} */


/****************************** TABLE.UPDATE() *******************************************************/

struct st_xmysqlnd_crud_table_op__update
{
	Mysqlx::Crud::Update message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_table_op__update(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::TABLE);
	}

	~st_xmysqlnd_crud_table_op__update() {}
};


/* {{{ xmysqlnd_crud_table_update__create */
XMYSQLND_CRUD_TABLE_OP__UPDATE *
xmysqlnd_crud_table_update__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_table_update__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_TABLE_OP__UPDATE * ret = new struct st_xmysqlnd_crud_table_op__update(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__destroy */
void
xmysqlnd_crud_table_update__destroy(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_update__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__set_criteria */
enum_func_status
xmysqlnd_crud_table_update__set_criteria(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		parser::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * exprCriteria = parser.expr();
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__set_limit */
enum_func_status
xmysqlnd_crud_table_update__set_limit(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__set_offset */
enum_func_status
xmysqlnd_crud_table_update__set_offset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__bind_value */
enum_func_status
xmysqlnd_crud_table_update__bind_value(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_update__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__add_orderby */
enum_func_status
xmysqlnd_crud_table_update__add_orderby(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_update__add_orderby");
	const enum_func_status ret = xmysqlnd_crud_table__add_orderby(obj->message.mutable_order(), obj->message.data_model(), orderby);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__add_operation */
static enum_func_status
xmysqlnd_crud_table_update__add_operation(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
											   const Mysqlx::Crud::UpdateOperation_UpdateType op_type,
											   const MYSQLND_CSTRING path,
											   const zval * const value,
											   const zend_bool is_expression,
											   const zend_bool is_document,
											   const zend_bool validate_array)
{
	DBG_ENTER("xmysqlnd_crud_table_update__add_operation");
	DBG_INF_FMT("operation=%s", Mysqlx::Crud::UpdateOperation::UpdateType_Name(op_type).c_str());
	DBG_INF_FMT("path=%*s  value=%p  is_expr=%u  is_document=%u  validate_array=%u", path.l, path.s, value, is_expression, is_document, validate_array);

	if (value) {
		DBG_INF_FMT("value_type=%u", Z_TYPE_P(value));
		switch (Z_TYPE_P(value)) {
			case IS_ARRAY:
			case IS_OBJECT:
			case IS_RESOURCE:
				DBG_ERR("Wrong value type");
				DBG_RETURN(FAIL);
		}
	}

	Mysqlx::Crud::UpdateOperation * operation = obj->message.mutable_operation()->Add();
	operation->set_operation(op_type);

	std::auto_ptr<Mysqlx::Expr::Expr> docpath(NULL);

	try {
		const std::string source(path.l ? path.s : "$", path.l ? path.l : sizeof("$") - 1);
		parser::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
		docpath.reset(parser.column_field());
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_ERR("Parser error for document field");
		DBG_RETURN(FAIL);
	}

	Mysqlx::Expr::ColumnIdentifier identifier(docpath->identifier());
	operation->mutable_source()->CopyFrom(identifier);

	if (value) {
		if (Z_TYPE_P(value) == IS_STRING && (is_expression || is_document)) {
			try {
				const std::string source(Z_STRVAL_P(value), Z_STRLEN_P(value));
				parser::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
				operation->set_allocated_value(parser.expr());
			} catch (parser::Parser_error &e) {
				DBG_ERR_FMT("%s", e.what());
				DBG_ERR("Parser error for document field");
				DBG_RETURN(FAIL);
			}
		} else {
			Mysqlx::Datatypes::Any any;
			if (FAIL == zval2any(value, any)) {
				DBG_ERR("Error converting the zval to scalar");
				DBG_RETURN(FAIL);
			}
			any2log(any);

			operation->mutable_value()->set_type(Mysqlx::Expr::Expr::LITERAL);
			operation->mutable_value()->set_allocated_literal(any.release_scalar());
		}
	}

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__unset */
enum_func_status
xmysqlnd_crud_table_update__unset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_table_update__unset");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, NULL, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__set */
enum_func_status
xmysqlnd_crud_table_update__set(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
									 const MYSQLND_CSTRING path,
									 const zval * const value,
									 const zend_bool is_expression,
									 const zend_bool is_document)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::SET;
	DBG_ENTER("xmysqlnd_crud_table_update__set");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, is_expression, is_document, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__replace */
enum_func_status
xmysqlnd_crud_table_update__replace(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
										 const MYSQLND_CSTRING path,
										 const zval * const value,
										 const zend_bool is_expression,
										 const zend_bool is_document)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REPLACE;
	DBG_ENTER("xmysqlnd_crud_table_update__replace");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, is_expression, is_document, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__merge */
enum_func_status
xmysqlnd_crud_table_update__merge(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
									   const MYSQLND_CSTRING path,
									   const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_MERGE;
	DBG_ENTER("xmysqlnd_crud_table_update__merge");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, FALSE, TRUE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__array_insert */
enum_func_status
xmysqlnd_crud_table_update__array_insert(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
											  const MYSQLND_CSTRING path,
											  const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_INSERT;
	DBG_ENTER("xmysqlnd_crud_table_update__array_insert");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, FALSE, FALSE, TRUE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__array_append */
enum_func_status
xmysqlnd_crud_table_update__array_append(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
											  const MYSQLND_CSTRING path,
											  const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_APPEND;
	DBG_ENTER("xmysqlnd_crud_table_update__array_append");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__is_initialized */
zend_bool
xmysqlnd_crud_table_update__is_initialized(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_update__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__finalize_bind */
enum_func_status
xmysqlnd_crud_table_update__finalize_bind(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_update__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	const enum_func_status ret = xmysqlnd_crud_table__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_update__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_update__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_UPDATE };
	return ret;
}
/* }}} */

/****************************** TABLE.SELECT() *******************************************************/

struct st_xmysqlnd_crud_table_op__select
{
	Mysqlx::Crud::Find message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_table_op__select(
		const MYSQLND_CSTRING & schema,
		const MYSQLND_CSTRING & object_name,
		zval * columns,
		const int num_of_columns)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::TABLE);

		add_columns(columns,num_of_columns);
	}

	~st_xmysqlnd_crud_table_op__select() {}

	void add_columns(const zval * columns, const int num_of_columns);
};


/* {{{ st_xmysqlnd_crud_table_op__select::add_columns */
void st_xmysqlnd_crud_table_op__select::add_columns(const zval * columns,
											const int num_of_columns)
{
	zend_bool is_expression = FALSE;
	enum_func_status ret = PASS;
	int i = 0;

	DBG_ENTER("mysqlx_node_collection__find::columns");

	do{
		if(Z_TYPE(columns[i]) == IS_OBJECT) {
			devapi::RAISE_EXCEPTION(err_msg_invalid_type);
		}

		if (Z_TYPE(columns[i]) == IS_STRING) {
			const MYSQLND_CSTRING column_str = { Z_STRVAL(columns[i]), Z_STRLEN(columns[i]) };
			ret = xmysqlnd_crud_table_select__set_column(this, column_str, FALSE, TRUE);
		} else if (Z_TYPE_P(columns) == IS_ARRAY) {
			const zval * entry;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL(columns[i]), entry) {
				if (Z_TYPE_P(entry) != IS_STRING) {
					devapi::RAISE_EXCEPTION(err_msg_wrong_param_1);
				}
				const MYSQLND_CSTRING column_str = { Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
				ret = xmysqlnd_crud_table_select__set_column(this, column_str, FALSE, TRUE);
			} ZEND_HASH_FOREACH_END();
		}
	}while( ( ++i < num_of_columns ) && ret != FAIL );

	if ( FAIL == ret ) {
		DBG_ERR_FMT("Parsing failure for item nbr. %d",
					i - 1);
		devapi::RAISE_EXCEPTION(err_msg_add_sort_fail);
	}
}
/* }}} */

/****************************** TABLE.SELECT() xmysqlnd_crud_table_select__ **************************/

/* {{{ xmysqlnd_crud_table_select__create */
XMYSQLND_CRUD_TABLE_OP__SELECT *
xmysqlnd_crud_table_select__create(const MYSQLND_CSTRING schema,
				const MYSQLND_CSTRING object_name,
				zval * columns, const int num_of_columns)
{
	XMYSQLND_CRUD_TABLE_OP__SELECT * ret = NULL;
	DBG_ENTER("xmysqlnd_crud_table_select__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	ret = new struct st_xmysqlnd_crud_table_op__select(schema, object_name, columns, num_of_columns);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__destroy */
void
xmysqlnd_crud_table_select__destroy(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_select__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__set_criteria */
enum_func_status
xmysqlnd_crud_table_select__set_criteria(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		parser::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * exprCriteria = parser.expr();
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__set_limit */
enum_func_status
xmysqlnd_crud_table_select__set_limit(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__set_offset */
enum_func_status
xmysqlnd_crud_table_select__set_offset(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__bind_value */
enum_func_status
xmysqlnd_crud_table_select__bind_value(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_select__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__add_orderby */
enum_func_status
xmysqlnd_crud_table_select__add_orderby(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING orderby)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_crud_table_select__add_orderby");
	ret = xmysqlnd_crud_table__add_orderby(obj->message.mutable_order(), obj->message.data_model(), orderby);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__add_grouping */
enum_func_status
xmysqlnd_crud_table_select__add_grouping(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING search_field)
{
	DBG_ENTER("xmysqlnd_crud_table_select__add_grouping");
	try {
		const static bool is_document = false; /*should be false, no comparison with data_model */
		const std::string source(search_field.s, search_field.l);
		parser::Expression_parser parser(source, is_document, false, &obj->placeholders);
		Mysqlx::Expr::Expr * exprCriteria = parser.expr();
		obj->message.mutable_grouping()->AddAllocated(exprCriteria);

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__set_column */
enum_func_status
xmysqlnd_crud_table_select__set_column(XMYSQLND_CRUD_TABLE_OP__SELECT * obj,
										  const MYSQLND_CSTRING column,
										  const zend_bool is_expression,
										  const zend_bool allow_alias)
{
	const bool is_document = (obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
	const std::string source(column.s, column.l);
	DBG_ENTER("xmysqlnd_crud_table_select__set_column");
	if (allow_alias) {
		try {
			parser::Projection_parser parser(source, is_document, allow_alias);
			parser.parse(*obj->message.mutable_projection());
		} catch (parser::Parser_error &e) {
			DBG_ERR_FMT("%s", e.what());
			DBG_INF("Parser error");
			DBG_RETURN(FAIL);
		}

		DBG_INF("PASS");
		DBG_RETURN(PASS);
	}

	try {
		parser::Expression_parser parser(source);
		Mysqlx::Expr::Expr * criteria = parser.expr();

		// Parsing is done just to validate it is a valid JSON expression
		if (criteria->type() != Mysqlx::Expr::Expr_Type_OBJECT) {
			delete criteria;
			DBG_RETURN(FAIL);
		}
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}

	try {
		parser::Expression_parser parser(source, is_document, false, &obj->placeholders);
		obj->message.mutable_projection()->Add()->set_allocated_source(parser.expr());

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}

	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__set_having */
enum_func_status
xmysqlnd_crud_table_select__set_having(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_having");
	try {
		const static zend_bool is_document = FALSE; /*should be TRUE, no comparison with data_model */
		const std::string source(criteria.s, criteria.l);
		parser::Expression_parser parser(source, is_document, false, &obj->placeholders);
		Mysqlx::Expr::Expr * exprCriteria = parser.expr();
		obj->message.set_allocated_grouping_criteria(exprCriteria);

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__is_initialized */
zend_bool
xmysqlnd_crud_table_select__is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_select__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__finalize_bind */
enum_func_status
xmysqlnd_crud_table_select__finalize_bind(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_select__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	enum_func_status ret = xmysqlnd_crud_table__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_table_select__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_select__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_FIND };
	return ret;
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
