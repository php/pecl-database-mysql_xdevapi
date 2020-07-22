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
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_any2expr.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include <vector>
#include <string>

#include "xmysqlnd_crud_table_commands.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_expression.h"
#include "mysqlx_exception.h"

#include "util/exceptions.h"
#include "util/pb_utils.h"

namespace mysqlx {

namespace drv {

enum_func_status
xmysqlnd_crud_table__bind_value(std::vector<std::string> & placeholders,
									 std::vector<Mysqlx::Datatypes::Scalar*> & bound_values,
									 const util::string_view& name,
									 zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table__bind_value");
	DBG_INF_FMT("name=%*s", name.length(), name.data());

	const std::string var_name(name.data(), name.length());
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

template< typename MSG >
enum_func_status
xmysqlnd_crud_table__add_orderby(MSG& message,
								   const util::string_view& orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__add_orderby");
	DBG_INF_FMT("orderby=%*s", orderby.length(), orderby.data());
	const Mysqlx::Crud::DataModel data_model =
			message.data_model();
	try {
		if( false == mysqlx::devapi::parser::orderby( std::string{ orderby },
											  data_model == Mysqlx::Crud::DOCUMENT,
											  &message) ) {
			DBG_ERR_FMT("Unable to parser the orderby expression");
			DBG_RETURN(FAIL);
		}
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table__finalize_bind(google::protobuf::RepeatedPtrField< ::Mysqlx::Datatypes::Scalar >* mutable_args,
										std::vector<Mysqlx::Datatypes::Scalar*> & bound_values)
{
	DBG_ENTER("xmysqlnd_crud_table__finalize_bind");

	const Mysqlx::Datatypes::Scalar* null_value{nullptr};
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator begin{ bound_values.begin() };
	const std::vector<Mysqlx::Datatypes::Scalar*>::iterator end{ bound_values.end() };
	const std::vector<Mysqlx::Datatypes::Scalar*>::const_iterator index{ std::find(begin, end, null_value) };
	if (index == end) {
		mutable_args->Clear();

		std::vector<Mysqlx::Datatypes::Scalar*>::iterator it{ begin };
		for (; it != end; ++it) {
			Mysqlx::Datatypes::Scalar* arg{ new Mysqlx::Datatypes::Scalar(**it) };
			mutable_args->AddAllocated(arg);
		}
	}
	DBG_RETURN(index == end? PASS : FAIL);
}

/****************************** TABLE.INSERT() *******************************************************/


void st_xmysqlnd_crud_table_op__insert::add_columns(zval * columns_zv,
											const int num_of_columns)
{
	DBG_ENTER("st_xmysqlnd_crud_table_op__insert::add_columns");
	enum_func_status ret{FAIL};
	int i{0};

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
				zval* entry{nullptr};
				ret = PASS;
				MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL(columns_zv[i]), entry)
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

void st_xmysqlnd_crud_table_op__insert::add_column(zval * column_zv)
{
	const std::string column_name(Z_STRVAL_P(column_zv), Z_STRLEN_P(column_zv));
	column_names.push_back(column_name);
}

void st_xmysqlnd_crud_table_op__insert::add_row(zval* row_zv)
{
	zval new_row_zv;
	ZVAL_COPY_VALUE(&new_row_zv, row_zv);
	rows_zv.push_back(new_row_zv);
}

void st_xmysqlnd_crud_table_op__insert::bind_columns()
{
	for (auto& column_name : column_names)
	{
		bind_column(column_name);
	}
}

void st_xmysqlnd_crud_table_op__insert::bind_column(const std::string& column_name)
{
	Mysqlx::Crud::Column* column = message.add_projection();
	column->set_name(column_name);
}

void st_xmysqlnd_crud_table_op__insert::bind_rows()
{
	for (auto& values_zv : rows_zv)
	{
		::Mysqlx::Crud::Insert_TypedRow* row = message.add_row();
		bind_row(&values_zv, row);
	}
}

void st_xmysqlnd_crud_table_op__insert::bind_row(zval* values_zv, ::Mysqlx::Crud::Insert_TypedRow* row)
{
	switch (Z_TYPE_P(values_zv))
	{
		case IS_ARRAY: {
			zval* entry{nullptr};
			MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL_P(values_zv), entry)
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

/****************************** xmysqlnd_crud_table_insert *******************************************************/


XMYSQLND_CRUD_TABLE_OP__INSERT *
xmysqlnd_crud_table_insert__create(const util::string& schema,
							const util::string& table_name,
							zval * columns,
							const int num_of_columns)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__create");
	DBG_INF_FMT("schema=%*s table_name=%*s", schema.length(), schema.c_str(), table_name.length(), table_name.c_str());
	XMYSQLND_CRUD_TABLE_OP__INSERT* ret = new st_xmysqlnd_crud_table_op__insert(schema,
																table_name, columns,num_of_columns);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_table_insert__destroy(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_table_insert__add_row(XMYSQLND_CRUD_TABLE_OP__INSERT * obj, zval * values_zv)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__add_row");
	enum_func_status ret{PASS};
	obj->add_row(values_zv);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_insert__finalize_bind(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_insert__finalize_bind");
	const enum_func_status ret = PASS;
	obj->bind_columns();
	obj->bind_rows();
	DBG_RETURN(ret);
}

struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_insert__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_INSERT };
	return ret;
}

zend_bool
xmysqlnd_crud_table_insert__is_initialized(XMYSQLND_CRUD_TABLE_OP__INSERT * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_insert__finalize_bind");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

/****************************** TABLE.DELETE() *******************************************************/


XMYSQLND_CRUD_TABLE_OP__DELETE *
xmysqlnd_crud_table_delete__create(const util::string& schema, const util::string& object_name)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.c_str(), object_name.length(), object_name.c_str());
	XMYSQLND_CRUD_TABLE_OP__DELETE * ret = new st_xmysqlnd_crud_table_op__delete(schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_table_delete__destroy(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_table_delete__set_criteria(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__set_criteria");
	try {
		Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( std::string{ criteria },
										 obj->message.data_model() == Mysqlx::Crud::DOCUMENT,
										 obj->placeholders );
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_delete__set_limit(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_delete__bind_value(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_delete__add_orderby(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_delete__add_orderby");
	const enum_func_status ret = xmysqlnd_crud_table__add_orderby(obj->message, orderby);
	DBG_RETURN(ret);
}

zend_bool
xmysqlnd_crud_table_delete__is_initialized(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_delete__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

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

struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_delete__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__DELETE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_DELETE };
	return ret;
}

/****************************** TABLE.UPDATE() *******************************************************/

XMYSQLND_CRUD_TABLE_OP__UPDATE *
xmysqlnd_crud_table_update__create(const util::string& schema, const util::string& object_name)
{
	DBG_ENTER("xmysqlnd_crud_table_update__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.c_str(), object_name.length(), object_name.c_str());
	XMYSQLND_CRUD_TABLE_OP__UPDATE * ret = new st_xmysqlnd_crud_table_op__update(schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_table_update__destroy(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_update__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_table_update__set_criteria(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_criteria");
	try {
		Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( std::string{ criteria },
										 obj->message.data_model() == Mysqlx::Crud::DOCUMENT,
										 obj->placeholders );
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_update__set_limit(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_update__set_offset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_table_update__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_update__bind_value(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_update__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_update__add_orderby(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& orderby)
{
	DBG_ENTER("xmysqlnd_crud_table_update__add_orderby");
	const enum_func_status ret = xmysqlnd_crud_table__add_orderby(obj->message, orderby);
	DBG_RETURN(ret);
}

static enum_func_status
xmysqlnd_crud_table_update__add_operation(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
											   const Mysqlx::Crud::UpdateOperation_UpdateType op_type,
											   const util::string_view& path,
											   const zval * const value,
											   const zend_bool is_expression,
											   const zend_bool is_document,
											   const zend_bool validate_array)
{
	DBG_ENTER("xmysqlnd_crud_table_update__add_operation");
	DBG_INF_FMT("operation=%s", Mysqlx::Crud::UpdateOperation::UpdateType_Name(op_type).c_str());
	DBG_INF_FMT("path=%*s  value=%p  is_expr=%u  is_document=%u  validate_array=%u", path.length(), path.data(), value, is_expression, is_document, validate_array);

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

	std::unique_ptr<Mysqlx::Expr::Expr> docpath(nullptr);

	try {
		constexpr std::string_view Empty_path = "$";
		const std::string source(path.empty() ? Empty_path : path);
		old_parser_api::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
		docpath.reset(parser.column_field());
	} catch (old_parser_api::Parser_error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
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
				Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( source,
												 obj->message.data_model() == Mysqlx::Crud::DOCUMENT,
												 obj->placeholders );
				operation->set_allocated_value(exprCriteria);
			} catch (cdk::Error &e) {
				php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
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

enum_func_status
xmysqlnd_crud_table_update__unset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_table_update__unset");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, nullptr, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_update__set(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
									 const util::string_view& path,
									 const zval * const value,
									 const zend_bool is_expression,
									 const zend_bool is_document)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::SET;
	DBG_ENTER("xmysqlnd_crud_table_update__set");
	const enum_func_status ret = xmysqlnd_crud_table_update__add_operation(obj, op_type, path, value, is_expression, is_document, FALSE);
	DBG_RETURN(ret);
}

zend_bool
xmysqlnd_crud_table_update__is_initialized(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_update__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

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

struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_update__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_UPDATE };
	return ret;
}

/****************************** TABLE.SELECT() *******************************************************/

void st_xmysqlnd_crud_table_op__select::add_columns(const zval * columns,
											const int num_of_columns)
{
	enum_func_status ret{PASS};
	int i{0};

	DBG_ENTER("mysqlx_table__select::columns");

	do{
		if(Z_TYPE(columns[i]) == IS_OBJECT) {
			devapi::RAISE_EXCEPTION(err_msg_invalid_type);
			DBG_VOID_RETURN;
		}

		if (Z_TYPE(columns[i]) == IS_STRING) {
			const util::string_view column_str{ Z_STRVAL(columns[i]), Z_STRLEN(columns[i]) };
			ret = xmysqlnd_crud_table_select__set_column(this, column_str, FALSE, TRUE);
		} else if (Z_TYPE_P(columns) == IS_ARRAY) {
			const zval* entry{nullptr};
			MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL(columns[i]), entry) {
				if (Z_TYPE_P(entry) != IS_STRING) {
					devapi::RAISE_EXCEPTION(err_msg_wrong_param_1);
					DBG_VOID_RETURN;
				}
				const util::string_view column_str{ Z_STRVAL_P(entry), Z_STRLEN_P(entry) };
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

/****************************** TABLE.SELECT() xmysqlnd_crud_table_select__ **************************/

XMYSQLND_CRUD_TABLE_OP__SELECT *
xmysqlnd_crud_table_select__create(const util::string& schema,
				const util::string& object_name,
				zval * columns, const int num_of_columns)
{
	XMYSQLND_CRUD_TABLE_OP__SELECT* ret{nullptr};
	DBG_ENTER("xmysqlnd_crud_table_select__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.c_str(), object_name.length(), object_name.c_str());
	ret = new st_xmysqlnd_crud_table_op__select(schema, object_name, columns, num_of_columns);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_table_select__destroy(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	DBG_ENTER("xmysqlnd_crud_table_select__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_table_select__set_criteria(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_criteria");
	try {
		Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( std::string{ criteria },
										 obj->message.data_model() == Mysqlx::Crud::DOCUMENT,
										 obj->placeholders );
		obj->message.set_allocated_criteria(exprCriteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__set_limit(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__set_offset(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__bind_value(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_table_select__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	enum_func_status ret = xmysqlnd_crud_table__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_select__add_orderby(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& orderby)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_crud_table_select__add_orderby");
	ret = xmysqlnd_crud_table__add_orderby(obj->message, orderby);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_table_select__add_grouping(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& search_field)
{
	DBG_ENTER("xmysqlnd_crud_table_select__add_grouping");
	try {
		const static bool is_document = false; /*should be false, no comparison with data_model */
		Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( std::string{ search_field },
										 is_document,
										 obj->placeholders );
		obj->message.mutable_grouping()->AddAllocated(exprCriteria);

		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__set_column(XMYSQLND_CRUD_TABLE_OP__SELECT * obj,
										  const util::string_view& column,
										  const zend_bool /*is_expression*/,
										  const zend_bool allow_alias)
{
	const bool is_document = (obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
	const std::string source(column);
	DBG_ENTER("xmysqlnd_crud_table_select__set_column");
	if (allow_alias) {
		try {
			mysqlx::devapi::parser::projection(source,
													is_document,
													&obj->message);
		} catch (cdk::Error &e) {
			php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
			DBG_ERR_FMT("%s", e.what());
			DBG_INF("Parser error");
			DBG_RETURN(FAIL);
		}

		DBG_INF("PASS");
		DBG_RETURN(PASS);
	}

	try {
		Mysqlx::Expr::Expr * criteria = mysqlx::devapi::parser::parse( source,
										 is_document );

		// Parsing is done just to validate it is a valid JSON expression
		if (criteria->type() != Mysqlx::Expr::Expr_Type_OBJECT) {
			delete criteria;
			DBG_RETURN(FAIL);
		}
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}

	try {
		Mysqlx::Expr::Expr * criteria = mysqlx::devapi::parser::parse( source,
										 is_document,
										 obj->placeholders );
		obj->message.mutable_projection()->Add()->set_allocated_source(criteria);

		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}

	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__set_having(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_table_select__set_having");
	try {
		const static zend_bool is_document = FALSE; /*should be TRUE, no comparison with data_model */
		Mysqlx::Expr::Expr * exprCriteria = mysqlx::devapi::parser::parse( std::string{ criteria },
										 is_document,
										 obj->placeholders );
		obj->message.set_allocated_grouping_criteria(exprCriteria);

		obj->bound_values.resize(obj->placeholders.size(), nullptr); /* fill with NULLs */
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}

zend_bool
xmysqlnd_crud_table_select__is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_table_select__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_table_select_verify_is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT* obj)
{
	if (xmysqlnd_crud_table_select__is_initialized(obj)) return;

	util::pb::verify_limit_offset(obj->message);

	throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
}

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

enum_func_status
xmysqlnd_crud_table_select__enable_lock_shared(XMYSQLND_CRUD_TABLE_OP__SELECT* obj)
{
	DBG_ENTER("xmysqlnd_crud_table_select__enable_lock_shared");
	obj->message.set_locking(::Mysqlx::Crud::Find_RowLock_SHARED_LOCK);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select__enable_lock_exclusive(XMYSQLND_CRUD_TABLE_OP__SELECT* obj)
{
	DBG_ENTER("xmysqlnd_crud_table_select__enable_lock_exclusive");
	obj->message.set_locking(::Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_table_select_set_lock_waiting_option(
	XMYSQLND_CRUD_TABLE_OP__SELECT* obj,
	int lock_waiting_option)
{
	DBG_ENTER("xmysqlnd_crud_table_select_set_lock_waiting_option");
	switch (lock_waiting_option)
	{
		case MYSQLX_LOCK_DEFAULT:
			obj->message.clear_locking_options();
			break;

		case MYSQLX_LOCK_NOWAIT:
			obj->message.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_NOWAIT);
			break;

		case MYSQLX_LOCK_SKIP_LOCKED:
			obj->message.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_SKIP_LOCKED);
			break;

		default:
			throw util::xdevapi_exception(util::xdevapi_exception::Code::unknown_lock_waiting_option);
	}
	DBG_RETURN(PASS);
}

struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_table_select__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__SELECT * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_FIND };
	return ret;
}

} // namespace drv

} // namespace mysqlx
