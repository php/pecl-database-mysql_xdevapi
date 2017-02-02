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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include <vector>
#include <string>

#include "xmysqlnd_crud_collection_commands.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_crud_collection__bind_value */
enum_func_status
xmysqlnd_crud_collection__bind_value(std::vector<std::string> & placeholders,
									 std::vector<Mysqlx::Datatypes::Scalar*> & bound_values,
									 const MYSQLND_CSTRING & name,
									 zval * value)
{
	DBG_ENTER("xmysqlnd_crud_collection__bind_value");
	DBG_INF_FMT("name=%*s", name.l, name.s);

	const std::string var_name(name.s, name.l);
	const std::vector<std::string>::iterator begin = placeholders.begin();
	const std::vector<std::string>::iterator end = placeholders.end();
	const std::vector<std::string>::const_iterator it = std::find(begin, end, var_name);
	if (it == end) {
		DBG_ERR("No such variable in the expression");
		DBG_RETURN(FAIL);
	}

	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(value, any)) {
		DBG_ERR("Error converting the zval to scalar");
		DBG_RETURN(FAIL);
	}
	any2log(any);

	const std::size_t index = static_cast<std::size_t>(it - begin);
	DBG_INF_FMT("offset=%u", index);
	auto& bound_value = bound_values[index];
	if (bound_value) {
		delete bound_value;
	}
	bound_value = any.release_scalar();

	scalar2log(*bound_value);

	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection__add_sort */
enum_func_status
xmysqlnd_crud_collection__add_sort(google::protobuf::RepeatedPtrField< Mysqlx::Crud::Order > * mutable_order,
								   const Mysqlx::Crud::DataModel data_model,
								   const MYSQLND_CSTRING & sort)
{
	DBG_ENTER("xmysqlnd_crud_collection__add_sort");
	DBG_INF_FMT("sort=%*s", sort.l, sort.s);
	try {
		const std::string source(sort.s, sort.l);
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


/* {{{ xmysqlnd_crud_collection__finalize_bind */
enum_func_status
xmysqlnd_crud_collection__finalize_bind(google::protobuf::RepeatedPtrField< ::Mysqlx::Datatypes::Scalar >* mutable_args,
										std::vector<Mysqlx::Datatypes::Scalar*> & bound_values)
{
	DBG_ENTER("xmysqlnd_crud_collection__finalize_bind");

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


/****************************** COLLECTION.ADD() *******************************************************/
struct st_xmysqlnd_crud_collection_op__add
{
	Mysqlx::Crud::Insert message;

	std::vector<zval> docs_zv;

	st_xmysqlnd_crud_collection_op__add(const MYSQLND_CSTRING & schema,
						const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	void add_document(zval* doc);
	void bind_docs();

	~st_xmysqlnd_crud_collection_op__add() {
		for( auto& values_zv : docs_zv ) {
			zval_dtor(&values_zv);
		}
		docs_zv.clear();
	}
};


/* {{{ xmysqlnd_crud_collection_add__create */
XMYSQLND_CRUD_COLLECTION_OP__ADD *
xmysqlnd_crud_collection_add__create(const MYSQLND_CSTRING schema,
						const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l,
				schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_COLLECTION_OP__ADD * ret = new struct st_xmysqlnd_crud_collection_op__add(
				schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_add__destroy */
void
xmysqlnd_crud_collection_add__destroy(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_add__finalize_bind */
enum_func_status
xmysqlnd_crud_collection_add__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__finalize_bind");
	const enum_func_status ret = PASS;
	obj->bind_docs();
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ xmysqlnd_crud_collection_add__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_collection_add__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_INSERT };
	return ret;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_add__add_doc */
enum_func_status
xmysqlnd_crud_collection_add__add_doc(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj,
						zval * values_zv)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__add_doc");
	enum_func_status ret = PASS;
	obj->add_document(values_zv);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ st_xmysqlnd_crud_collection_op__add::add_document */
void st_xmysqlnd_crud_collection_op__add::add_document(zval* doc)
{
	zval new_doc;
	ZVAL_DUP(&new_doc, doc);
	docs_zv.push_back(new_doc);
}
/* }}} */


/* {{{ st_xmysqlnd_crud_collection_op__add::bind_docs */
void st_xmysqlnd_crud_collection_op__add::bind_docs()
{
	for (auto& values_zv : docs_zv)
	{
		::Mysqlx::Crud::Insert_TypedRow* row = message.add_row();
		Mysqlx::Expr::Expr * field = row->add_field();
		field->set_type(Mysqlx::Expr::Expr::LITERAL);

		Mysqlx::Datatypes::Scalar * literal = field->mutable_literal();
		literal->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
		literal->mutable_v_string()->set_value(Z_STRVAL(values_zv),
								Z_STRLEN(values_zv));
	}
}
/* }}} */

/****************************** COLLECTION.REMOVE() *******************************************************/
struct st_xmysqlnd_crud_collection_op__remove
{
	Mysqlx::Crud::Delete message;

	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_collection_op__remove(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__remove() {}
};


/* {{{ xmysqlnd_crud_collection_remove__create */
XMYSQLND_CRUD_COLLECTION_OP__REMOVE *
xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE * ret = new struct st_xmysqlnd_crud_collection_op__remove(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__destroy */
void
xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_criteria */
enum_func_status
xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
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


/* {{{ xmysqlnd_crud_collection_remove__set_limit */
enum_func_status
xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_skip */
enum_func_status
xmysqlnd_crud_collection_remove__set_skip(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_skip");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__bind_value */
enum_func_status
xmysqlnd_crud_collection_remove__bind_value(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_collection__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__add_sort */
enum_func_status
xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__add_sort");
	const enum_func_status ret = xmysqlnd_crud_collection__add_sort(obj->message.mutable_order(), obj->message.data_model(), sort);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__is_initialized */
zend_bool
xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_remove__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__finalize_bind */
enum_func_status
xmysqlnd_crud_collection_remove__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	const enum_func_status ret = xmysqlnd_crud_collection__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_collection_remove__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_DELETE };
	return ret;
}
/* }}} */


/****************************** COLLECTION.MODIFY() *******************************************************/

struct st_xmysqlnd_crud_collection_op__modify
{
	Mysqlx::Crud::Update message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_collection_op__modify(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__modify() {}
};


/* {{{ xmysqlnd_crud_collection_modify__create */
XMYSQLND_CRUD_COLLECTION_OP__MODIFY *
xmysqlnd_crud_collection_modify__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY * ret = new struct st_xmysqlnd_crud_collection_op__modify(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__destroy */
void
xmysqlnd_crud_collection_modify__destroy(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set_criteria */
enum_func_status
xmysqlnd_crud_collection_modify__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
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


/* {{{ xmysqlnd_crud_collection_modify__set_limit */
enum_func_status
xmysqlnd_crud_collection_modify__set_limit(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set_skip */
enum_func_status
xmysqlnd_crud_collection_modify__set_skip(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_skip");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__bind_value */
enum_func_status
xmysqlnd_crud_collection_modify__bind_value(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	const enum_func_status ret = xmysqlnd_crud_collection__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__add_sort */
enum_func_status
xmysqlnd_crud_collection_modify__add_sort(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__add_sort");
	const enum_func_status ret = xmysqlnd_crud_collection__add_sort(obj->message.mutable_order(), obj->message.data_model(), sort);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__add_operation */
static enum_func_status
xmysqlnd_crud_collection_modify__add_operation(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
											   const Mysqlx::Crud::UpdateOperation_UpdateType op_type,
											   const MYSQLND_CSTRING path,
											   const zval * const value,
											   const zend_bool is_expression,
											   const zend_bool is_document,
											   const zend_bool validate_array)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__add_operation");
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
		docpath.reset(parser.document_field());
	} catch (parser::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_ERR("Parser error for document field");
		DBG_RETURN(FAIL);
	}

	Mysqlx::Expr::ColumnIdentifier identifier(docpath->identifier());

	// Validates the source is an array item
	const size_t size = identifier.document_path().size();
	if (size) {
		DBG_INF_FMT("doc_path_size=%u", (uint) size);
		if (validate_array) {
			const Mysqlx::Expr::DocumentPathItem_Type doc_path_type = identifier.document_path().Get(size - 1).type();
			DBG_INF_FMT("type=%s", Mysqlx::Expr::DocumentPathItem::Type_Name(doc_path_type).c_str());
			if (doc_path_type != Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX) {
				DBG_ERR("An array document path must be specified");
				DBG_RETURN(FAIL);
			}
		}
	} else if (op_type != Mysqlx::Crud::UpdateOperation::ITEM_MERGE) {
		DBG_ERR("Invalid document path");
		DBG_RETURN(FAIL);
	}

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


/* {{{ xmysqlnd_crud_collection_modify__unset */
enum_func_status
xmysqlnd_crud_collection_modify__unset(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__unset");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, NULL, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set */
enum_func_status
xmysqlnd_crud_collection_modify__set(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
									 const MYSQLND_CSTRING path,
									 const zval * const value,
									 const zend_bool is_expression,
									 const zend_bool is_document)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_SET;
	DBG_ENTER("xmysqlnd_crud_collection_modify__set");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, value, is_expression, is_document, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__replace */
enum_func_status
xmysqlnd_crud_collection_modify__replace(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
										 const MYSQLND_CSTRING path,
										 const zval * const value,
										 const zend_bool is_expression,
										 const zend_bool is_document)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REPLACE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__replace");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, value, is_expression, is_document, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__merge */
enum_func_status
xmysqlnd_crud_collection_modify__merge(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
									   const MYSQLND_CSTRING path,
									   const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_MERGE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__merge");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, value, FALSE, TRUE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__array_insert */
enum_func_status
xmysqlnd_crud_collection_modify__array_insert(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
											  const MYSQLND_CSTRING path,
											  const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_INSERT;
	DBG_ENTER("xmysqlnd_crud_collection_modify__array_insert");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, value, FALSE, FALSE, TRUE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__array_append */
enum_func_status
xmysqlnd_crud_collection_modify__array_append(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
											  const MYSQLND_CSTRING path,
											  const zval * const value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_APPEND;
	DBG_ENTER("xmysqlnd_crud_collection_modify__array_append");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, value, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__array_delete */
enum_func_status
xmysqlnd_crud_collection_modify__array_delete(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__array_delete");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, NULL, FALSE, FALSE, TRUE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__is_initialized */
zend_bool
xmysqlnd_crud_collection_modify__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__finalize_bind */
enum_func_status
xmysqlnd_crud_collection_modify__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	const enum_func_status ret = xmysqlnd_crud_collection__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_collection_modify__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_UPDATE };
	return ret;
}
/* }}} */

/****************************** COLLECTION.FIND() *******************************************************/

struct st_xmysqlnd_crud_collection_op__find
{
	Mysqlx::Crud::Find message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_collection_op__find(const MYSQLND_CSTRING & schema,
										 const MYSQLND_CSTRING & object_name)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__find() {}
};


/* {{{ xmysqlnd_crud_collection_find__create */
XMYSQLND_CRUD_COLLECTION_OP__FIND *
xmysqlnd_crud_collection_find__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	XMYSQLND_CRUD_COLLECTION_OP__FIND * ret = NULL;
	DBG_ENTER("xmysqlnd_crud_collection_find__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	ret = new struct st_xmysqlnd_crud_collection_op__find(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__destroy */
void
xmysqlnd_crud_collection_find__destroy(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_criteria */
enum_func_status
xmysqlnd_crud_collection_find__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
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


/* {{{ xmysqlnd_crud_collection_find__set_limit */
enum_func_status
xmysqlnd_crud_collection_find__set_limit(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_skip */
enum_func_status
xmysqlnd_crud_collection_find__set_skip(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_skip");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__bind_value */
enum_func_status
xmysqlnd_crud_collection_find__bind_value(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__bind_value");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}
	enum_func_status ret = xmysqlnd_crud_collection__bind_value(obj->placeholders, obj->bound_values, name, value);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__add_sort */
enum_func_status
xmysqlnd_crud_collection_find__add_sort(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING sort)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_crud_collection_find__add_sort");
	ret = xmysqlnd_crud_collection__add_sort(obj->message.mutable_order(), obj->message.data_model(), sort);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__add_grouping */
enum_func_status
xmysqlnd_crud_collection_find__add_grouping(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING search_field)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__add_grouping");
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


/* {{{ xmysqlnd_crud_collection_find__set_fields */
enum_func_status
xmysqlnd_crud_collection_find__set_fields(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj,
										  const MYSQLND_CSTRING field,
										  const zend_bool is_expression,
										  const zend_bool allow_alias)
{
	const bool is_document = (obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
	const std::string source(field.s, field.l);
	DBG_ENTER("xmysqlnd_crud_collection_find__set_fields");
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


/* {{{ xmysqlnd_crud_collection_find__set_having */
enum_func_status
xmysqlnd_crud_collection_find__set_having(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_having");
	try {
		const static zend_bool is_document = TRUE; /*should be TRUE, no comparison with data_model */
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


/* {{{ xmysqlnd_crud_collection_find__is_initialized */
zend_bool
xmysqlnd_crud_collection_find__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_find__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__finalize_bind */
enum_func_status
xmysqlnd_crud_collection_find__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__finalize_bind");
	if (obj->placeholders.size() && !obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	enum_func_status ret = xmysqlnd_crud_collection__finalize_bind(obj->message.mutable_args(), obj->bound_values);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_crud_collection_find__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_FIND };
	return ret;
}
/* }}} */


/****************************** SQL EXECUTE *******************************************************/
struct st_xmysqlnd_stmt_op__execute
{
	zval * params;
	unsigned int params_allocated;

	Mysqlx::Sql::StmtExecute message;

	st_xmysqlnd_stmt_op__execute(const MYSQLND_CSTRING & namespace_,
								 const MYSQLND_CSTRING & stmt,
								 const bool compact_meta)
		: params(NULL), params_allocated(0)
	{
		message.set_namespace_(namespace_.s, namespace_.l);
		message.set_stmt(stmt.s, stmt.l);
		message.set_compact_metadata(compact_meta);
	}

	enum_func_status bind_one_param(const zval * param_zv);
	enum_func_status bind_one_param(const unsigned int param_no, const zval * param_zv);
	enum_func_status finalize_bind();

	~st_xmysqlnd_stmt_op__execute()
	{
		if (params) {
			for(int i = 0 ; i < params_allocated; ++i ) {
				zval_ptr_dtor(&params[i]);
			}
			mnd_efree(params);
		}
	}
};


/* {{{ st_xmysqlnd_stmt_op__execute::bind_one_stmt_param */
enum_func_status
st_xmysqlnd_stmt_op__execute::bind_one_param(const zval * param_zv)
{
	DBG_ENTER("st_xmysqlnd_stmt_op__execute::bind_one_stmt_param");
	const unsigned int param_no = params_allocated;
	DBG_RETURN(bind_one_param(param_no, param_zv));
}
/* }}} */


/* {{{ st_xmysqlnd_stmt_op__execute::bind_one_stmt_param */
enum_func_status
st_xmysqlnd_stmt_op__execute::bind_one_param(const unsigned int param_no, const zval * param_zv)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("st_xmysqlnd_stmt_op__execute::bind_one_stmt_param");
	DBG_INF_FMT("params=%p", params);
	if (!params || param_no >= params_allocated) {
		DBG_INF("Not enough space for params, realloc");
		params = (zval*) mnd_erealloc(params, (param_no + 1) * sizeof(zval));
		if (!params) {
			DBG_RETURN(FAIL);
		}
		/* Now we have a hole between the last allocated and the new param_no which is not zeroed. Zero it! */
		memset(&params[params_allocated], 0, (param_no - params_allocated + 1) * sizeof(zval));

		params_allocated = param_no + 1;
		ret = PASS;
	}
	zval_ptr_dtor(&params[param_no]);

	ZVAL_COPY_VALUE(&params[param_no], param_zv);
	Z_TRY_ADDREF(params[param_no]);
#ifdef PHP_DEBUG
	switch (ret) {
		case PASS:
			DBG_INF("PASS");
			break;
		case FAIL:
			DBG_INF("FAIL");
			break;
	}
#endif
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ st_xmysqlnd_stmt_op__execute::finalize_bind */
enum_func_status
st_xmysqlnd_stmt_op__execute::finalize_bind()
{
	enum_func_status ret = PASS;
	unsigned int i = 0;
	DBG_ENTER("st_xmysqlnd_stmt_op__execute::finalize_bind");
	for (; i < params_allocated; ++i) {
		Mysqlx::Datatypes::Any * arg = message.add_args();
		ret = zval2any(&(params[i]), *arg);
		if (FAIL == ret) {
			break;
		}

	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__create */
XMYSQLND_STMT_OP__EXECUTE *
xmysqlnd_stmt_execute__create(const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt)
{
	XMYSQLND_STMT_OP__EXECUTE * ret = NULL;
	DBG_ENTER("xmysqlnd_stmt_execute__create");
	DBG_INF_FMT("namespace_=%*s stmt=%*s", namespace_.l, namespace_.s, stmt.l, stmt.s);
	ret = new struct st_xmysqlnd_stmt_op__execute(namespace_, stmt, false);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__destroy */
void
xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__bind_one_param_add */
enum_func_status
xmysqlnd_stmt_execute__bind_one_param_add(XMYSQLND_STMT_OP__EXECUTE * obj, const zval * param_zv)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_one_param_add");
	const enum_func_status ret = obj->bind_one_param(param_zv);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__bind_one_param */
enum_func_status
xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE * obj, const unsigned int param_no, const zval * param_zv)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_one_param");
	const enum_func_status ret = obj->bind_one_param(param_no, param_zv);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__bind_value */
enum_func_status
xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE * obj, zval * value)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_value");
	Mysqlx::Datatypes::Any * arg = obj->message.add_args();
	const enum_func_status ret = zval2any(value, *arg);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__finalize_bind */
enum_func_status
xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__finalize_bind");
	const enum_func_status ret = obj->finalize_bind();
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__is_initialized */
zend_bool
xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_stmt_execute__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__get_protobuf_message */
struct st_xmysqlnd_pb_message_shell
xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_SQL_STMT_EXECUTE };
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
