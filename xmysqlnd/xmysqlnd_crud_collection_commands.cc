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
#include "php.h"
#include "ext/json/php_json_parser.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
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

/* {{{ xmysqlnd_crud_collection__bind_value */
extern "C" enum_func_status
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


/* {{{ xmysqlnd_crud_collection_remove__add_sort */
extern "C" enum_func_status
xmysqlnd_crud_collection__add_sort(google::protobuf::RepeatedPtrField< Mysqlx::Crud::Order > * mutable_order,
								   const Mysqlx::Crud::DataModel data_model,
								   const MYSQLND_CSTRING & sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__add_sort");
	DBG_INF_FMT("sort=%*s", sort.l, sort.s);
	try {
		const std::string source(sort.s, sort.l);
		xmysqlnd::Orderby_parser parser(source, data_model == Mysqlx::Crud::DOCUMENT);
		parser.parse(*mutable_order);
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection__finalize_bind */
extern "C" enum_func_status
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


/****************************** COLLECTION.REMOVE() *******************************************************/
struct st_xmysqlnd_crud_collection_op__remove
{
	Mysqlx::Crud::Delete message;

	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_collection_op__remove(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name,
										   const bool document_mode = true)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(document_mode? Mysqlx::Crud::DOCUMENT : Mysqlx::Crud::TABLE);
	}

	virtual ~st_xmysqlnd_crud_collection_op__remove() {}
};


/* {{{ xmysqlnd_crud_collection_remove__create */
extern "C" XMYSQLND_CRUD_COLLECTION_OP__REMOVE *
xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE * ret = new struct st_xmysqlnd_crud_collection_op__remove(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__destroy */
extern "C" void
xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_criteria */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.set_allocated_criteria(criteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_limit */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_offset */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_offset(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__bind_value */
extern "C" enum_func_status
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
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__add_sort");
	const enum_func_status ret = xmysqlnd_crud_collection__add_sort(obj->message.mutable_order(), obj->message.data_model(), sort);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__is_initialized */
extern "C" zend_bool
xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_remove__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__finalize_bind */
extern "C" enum_func_status
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
extern "C" struct st_xmysqlnd_pb_message_shell 
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
										   const MYSQLND_CSTRING & object_name,
										   const bool document_mode = true)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(document_mode? Mysqlx::Crud::DOCUMENT : Mysqlx::Crud::TABLE);
	}

	virtual ~st_xmysqlnd_crud_collection_op__modify() {}
};


/* {{{ xmysqlnd_crud_collection_modify__create */
extern "C" XMYSQLND_CRUD_COLLECTION_OP__MODIFY *
xmysqlnd_crud_collection_modify__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY * ret = new struct st_xmysqlnd_crud_collection_op__modify(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__destroy */
extern "C" void
xmysqlnd_crud_collection_modify__destroy(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set_criteria */
extern "C" enum_func_status
xmysqlnd_crud_collection_modify__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.set_allocated_criteria(criteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set_limit */
extern "C" enum_func_status
xmysqlnd_crud_collection_modify__set_limit(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set_offset */
extern "C" enum_func_status
xmysqlnd_crud_collection_modify__set_offset(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__bind_value */
extern "C" enum_func_status
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
extern "C" enum_func_status
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
		xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
		docpath.reset(parser.document_field());
	} catch (xmysqlnd::Parser_error &e) {
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
				xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
				operation->set_allocated_value(parser.expr());
			} catch (xmysqlnd::Parser_error &e) {
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
extern "C" enum_func_status
xmysqlnd_crud_collection_modify__unset(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__unset");
	const enum_func_status ret = xmysqlnd_crud_collection_modify__add_operation(obj, op_type, path, NULL, FALSE, FALSE, FALSE);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__set */
extern "C" enum_func_status
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
extern "C" enum_func_status
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
extern "C" enum_func_status
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
extern "C" enum_func_status
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
extern "C" enum_func_status
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


/* {{{ xmysqlnd_crud_collection_modify__is_initialized */
extern "C" zend_bool
xmysqlnd_crud_collection_modify__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_modify__finalize_bind */
extern "C" enum_func_status
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
extern "C" struct st_xmysqlnd_pb_message_shell 
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
										 const MYSQLND_CSTRING & object_name,
										 const bool document_mode = true)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(document_mode? Mysqlx::Crud::DOCUMENT : Mysqlx::Crud::TABLE);
	}

	virtual ~st_xmysqlnd_crud_collection_op__find() {}
};


/* {{{ xmysqlnd_crud_collection_find__create */
extern "C" XMYSQLND_CRUD_COLLECTION_OP__FIND *
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
extern "C" void
xmysqlnd_crud_collection_find__destroy(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_criteria */
extern "C" enum_func_status
xmysqlnd_crud_collection_find__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
	try {
		const std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.set_allocated_criteria(criteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_limit */
extern "C" enum_func_status
xmysqlnd_crud_collection_find__set_limit(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_offset */
extern "C" enum_func_status
xmysqlnd_crud_collection_find__set_offset(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__bind_value */
extern "C" enum_func_status
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
extern "C" enum_func_status
xmysqlnd_crud_collection_find__add_sort(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING sort)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_crud_collection_find__add_sort");
	ret = xmysqlnd_crud_collection__add_sort(obj->message.mutable_order(), obj->message.data_model(), sort);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__add_grouping */
extern "C" enum_func_status
xmysqlnd_crud_collection_find__add_grouping(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING search_field)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__add_grouping");
	try {
		const static bool is_document = false; /*should be false, no comparison with data_model */
		const std::string source(search_field.s, search_field.l);
		xmysqlnd::Expression_parser parser(source, is_document, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.mutable_grouping()->AddAllocated(criteria);

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_fields */
extern "C" enum_func_status
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
			xmysqlnd::Projection_parser parser(source, is_document, allow_alias);
			parser.parse(*obj->message.mutable_projection());
		} catch (xmysqlnd::Parser_error &e) {
			DBG_ERR_FMT("%s", e.what());
			DBG_INF("Parser error");
			DBG_RETURN(FAIL);
		}

		DBG_INF("PASS");
		DBG_RETURN(PASS);
	} 

	try {
		xmysqlnd::Expression_parser parser(source);
		Mysqlx::Expr::Expr * criteria = parser.expr();

		// Parsing is done just to validate it is a valid JSON expression
		if (criteria->type() != Mysqlx::Expr::Expr_Type_OBJECT) {
			delete criteria;
			DBG_RETURN(FAIL);
		}
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}

	try {
		xmysqlnd::Expression_parser parser(source, is_document, false, &obj->placeholders);
		obj->message.mutable_projection()->Add()->set_allocated_source(parser.expr());

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__set_having */
extern "C" enum_func_status
xmysqlnd_crud_collection_find__set_having(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_having");
	try {
		const static zend_bool is_document = TRUE; /*should be TRUE, no comparison with data_model */
		const std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, is_document, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.set_allocated_grouping_criteria(criteria);

		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__is_initialized */
extern "C" zend_bool
xmysqlnd_crud_collection_find__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_find__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_find__finalize_bind */
extern "C" enum_func_status
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
extern "C" struct st_xmysqlnd_pb_message_shell 
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

	enum_func_status bind_one_param(const unsigned int param_no, const zval * param_zv);
	enum_func_status finalize_bind();

	virtual ~st_xmysqlnd_stmt_op__execute()
	{
		if (params) {
			mnd_efree(params);
		}
	}
};


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


/* {{{ st_xmysqlnd_stmt_op__execute::bind_one_stmt_param */
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
extern "C" XMYSQLND_STMT_OP__EXECUTE *
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
extern "C" void
xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__bind_one_param */
extern "C" enum_func_status
xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE * obj, const unsigned int param_no, const zval * param_zv)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_one_param");
	const enum_func_status ret = obj->bind_one_param(param_no, param_zv);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__bind_value */
extern "C" enum_func_status
xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE * obj, zval * value)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_value");
	Mysqlx::Datatypes::Any * arg = obj->message.add_args();
	const enum_func_status ret = zval2any(value, *arg);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__finalize_bind */
extern "C" enum_func_status
xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__finalize_bind");
	const enum_func_status ret = obj->finalize_bind();
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__is_initialized */
extern "C" zend_bool
xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_stmt_execute__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execute__get_protobuf_message */
extern "C" struct st_xmysqlnd_pb_message_shell
xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_SQL_STMT_EXECUTE };
	return ret;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */