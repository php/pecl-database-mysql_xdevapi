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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"
#include "mysqlx_enum_n_def.h"
#include "util/exceptions.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "util/pb_utils.h"

namespace mysqlx {

namespace drv {

Bindings::Bindings()
{
}

Bindings::~Bindings()
{
	for (auto& bound_value : bound_variables) {
		auto& value = bound_value.second;
		delete value;
	}
}

bool Bindings::empty() const
{
	return bound_variables.empty();
}

std::size_t Bindings::size() const
{
	return bound_variables.size();
}

void Bindings::add_placeholder(const util::string& placeholder)
{
	auto it{ find_variable(placeholder) };
	if (it == bound_variables.end()) {
		bound_variables.push_back({ placeholder, nullptr });
	}
}

void Bindings::add_placeholders(const util::std_strings& placeholders)
{
	for (const auto& placeholder : placeholders) {
		add_placeholder(util::to_string(placeholder));
	}
}

util::std_strings Bindings::get_placeholders() const
{
	util::std_strings placeholders;
	for (const auto& var_name_value : bound_variables) {
		placeholders.push_back(util::to_std_string(var_name_value.first));
	}
	return placeholders;
}

bool Bindings::bind(const util::string& var_name, zval* var_value)
{
	DBG_ENTER("Bindings::bind");
	DBG_INF_FMT("name=%*s", var_name.length(), var_name.c_str());

	auto it{ find_variable(var_name) };
	if (it == bound_variables.end()) {
		DBG_ERR("No such variable in the expression");
		DBG_RETURN(false);
	}

	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(var_value, any)) {
		DBG_ERR("Error converting the zval to scalar");
		DBG_RETURN(false);
	}
	any2log(any);

	auto& bound_value = it->second;
	if (bound_value) {
		delete bound_value;
	}
	bound_value = any.release_scalar();

	scalar2log(*bound_value);

	DBG_RETURN(true);
}

bool Bindings::finalize(google::protobuf::RepeatedPtrField< ::Mysqlx::Datatypes::Scalar >* mutable_args)
{
	DBG_ENTER("Bindings::finalize");
    //assert(mutable_args->empty());
	for (const auto& var_name_value : bound_variables) {
		auto var_value{ var_name_value.second };
		if (var_value == nullptr) {
			util::ostringstream os;
			os << "No such variable in the expression: '" << var_name_value.first << "'.";
			throw util::xdevapi_exception(
				util::xdevapi_exception::Code::bind_fail,
				os.str());
		}
		Mysqlx::Datatypes::Scalar* cloned_var_value{ new Mysqlx::Datatypes::Scalar(*var_value) };
		mutable_args->AddAllocated(cloned_var_value);
	}
	DBG_RETURN(true);
}

Bindings::Bound_values Bindings::get_bound_values() const
{
	DBG_ENTER("Bindings::get_bound_values");
	Bound_values bound_values;
	for (const auto& var_name_value : bound_variables) {
		auto var_value{ var_name_value.second };
		// finalize should always be called before get_bound_values
		assert(var_value != nullptr);
		bound_values.push_back(var_value);
	}
	DBG_RETURN(bound_values);
}

Bindings::Bound_variables_it Bindings::find_variable(const util::string& var_name)
{
	return std::find_if(
		bound_variables.begin(),
		bound_variables.end(),
		[var_name](const auto& var_name_value) { return var_name_value.first == var_name; }
	);
}

namespace {

Mysqlx::Expr::Expr* parse_expression(
	const std::string& expression_str,
	Bindings& bindings,
	bool is_document_model = true)
{
	util::std_strings placeholders(bindings.get_placeholders());
	Mysqlx::Expr::Expr* expression
		= mysqlx::devapi::parser::parse(expression_str, is_document_model, placeholders);

	bindings.add_placeholders(placeholders);
	return expression;
}

} // anonymous namespace

template< typename MSG >
enum_func_status
xmysqlnd_crud_collection__add_sort(MSG& message,
								   const util::string_view& sort)
{
	DBG_ENTER("xmysqlnd_crud_collection__add_sort");
	DBG_INF_FMT("sort=%*s", sort.length(), sort.data());
	try {
		if( false == mysqlx::devapi::parser::orderby(std::string{ sort }, true, &message) ) {
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

/****************************** COLLECTION.ADD() *******************************************************/

XMYSQLND_CRUD_COLLECTION_OP__ADD *
xmysqlnd_crud_collection_add__create(
	const util::string_view& schema,
	const util::string_view& object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(),
				schema.data(), object_name.length(), object_name.data());
	XMYSQLND_CRUD_COLLECTION_OP__ADD * ret = new st_xmysqlnd_crud_collection_op__add(
				schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_collection_add__destroy(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_collection_add__set_upsert(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__set_upsert");
	obj->message.set_upsert(true);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_add__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__finalize_bind");
	const enum_func_status ret = PASS;
	obj->bind_docs();
	DBG_RETURN(ret);
}

struct st_xmysqlnd_pb_message_shell
		xmysqlnd_crud_collection_add__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_INSERT };
	return ret;
}

enum_func_status
xmysqlnd_crud_collection_add__add_doc(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj,
									  zval * values_zv)
{
	DBG_ENTER("xmysqlnd_crud_collection_add__add_doc");
	enum_func_status ret{PASS};
	obj->add_document(values_zv);
	DBG_RETURN(ret);
}

void st_xmysqlnd_crud_collection_op__add::add_document(zval* doc)
{
	zval new_doc;
	ZVAL_DUP(&new_doc, doc);
	docs_zv.push_back(new_doc);
}

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

/****************************** COLLECTION.REMOVE() *******************************************************/

XMYSQLND_CRUD_COLLECTION_OP__REMOVE *
xmysqlnd_crud_collection_remove__create(const util::string_view& schema, const util::string_view& object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.data(), object_name.length(), object_name.data());
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE * ret = new st_xmysqlnd_crud_collection_op__remove(schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const std::string& criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
	try {
		Mysqlx::Expr::Expr* expr_criteria = parse_expression(criteria, obj->bindings);
		obj->message.set_allocated_criteria(expr_criteria);
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
xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_remove__set_skip(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_skip");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_remove__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE* obj,
	const util::string& name,
	zval* value)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__bind_value");
	DBG_RETURN(obj->bindings.bind(name, value) ? PASS : FAIL);
}

enum_func_status
xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const util::string_view& sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__add_sort");
	const enum_func_status ret = xmysqlnd_crud_collection__add_sort(obj->message, sort);
	DBG_RETURN(ret);
}

zend_bool
xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_remove__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_collection_remove__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__finalize_bind");
	DBG_RETURN(obj->bindings.finalize(obj->message.mutable_args()) ? PASS : FAIL);
}

struct st_xmysqlnd_pb_message_shell
		xmysqlnd_crud_collection_remove__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_DELETE };
	return ret;
}

/****************************** COLLECTION.MODIFY() *******************************************************/

XMYSQLND_CRUD_COLLECTION_OP__MODIFY *
xmysqlnd_crud_collection_modify__create(const util::string_view& schema, const util::string_view& object_name)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.data(), object_name.length(), object_name.data());
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY * ret = new st_xmysqlnd_crud_collection_op__modify(schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_collection_modify__destroy(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

bool
xmysqlnd_crud_collection_modify__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const std::string& criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_criteria");
	try {
		Mysqlx::Expr::Expr* expr_criteria = parse_expression(criteria, obj->bindings);
		obj->message.set_allocated_criteria(expr_criteria);
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_INF("Parser error");
		DBG_RETURN(false);
	}
	DBG_INF("PASS");
	DBG_RETURN(true);
}

bool
xmysqlnd_crud_collection_modify__set_limit(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(true);
}

bool
xmysqlnd_crud_collection_modify__set_skip(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__set_skip");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(true);
}

bool
xmysqlnd_crud_collection_modify__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const util::string& name,
	zval* value)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__bind_value");
	DBG_RETURN(obj->bindings.bind(name, value));
}

bool
xmysqlnd_crud_collection_modify__add_sort(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const util::string_view& sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__add_sort");
	const enum_func_status ret = xmysqlnd_crud_collection__add_sort(obj->message, sort);
	DBG_RETURN(ret == PASS);
}

static bool
xmysqlnd_crud_collection_modify__add_operation(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type,
	const Modify_value& modify_value)
{
	const util::string_view& path{ modify_value.path };
	const util::zvalue& value{ modify_value.value };
	const bool is_expression{ modify_value.is_expression };
	const bool is_document{ modify_value.is_document };
	const bool validate_array{ modify_value.validate_array };

	DBG_ENTER("xmysqlnd_crud_collection_modify__add_operation");
	DBG_INF_FMT("operation=%s", Mysqlx::Crud::UpdateOperation::UpdateType_Name(op_type).c_str());
	DBG_INF_FMT("path=%*s  value=%p  is_expr=%u  is_document=%u  validate_array=%u",
		path.length(),
		path.data(),
		value.ptr(),
		static_cast<unsigned>(is_expression),
		static_cast<unsigned>(is_document),
		static_cast<unsigned>(validate_array));

	if (!value.is_undef()) {
		DBG_INF_FMT("value_type=%u", value.type());
		switch (value.type()) {
		case util::zvalue::Type::Array:
		case util::zvalue::Type::Object:
		case util::zvalue::Type::Resource:
			DBG_ERR("Wrong value type");
			DBG_RETURN(false);
		default:
			break;
		}
	}

	Mysqlx::Crud::UpdateOperation * operation = obj->message.mutable_operation()->Add();
	operation->set_operation(op_type);

	std::unique_ptr<Mysqlx::Expr::Expr> docpath;

	try {
		const std::string Default_path("$");
		const std::string& source(path.empty() ? Default_path : std::string{ path });
		docpath.reset(mysqlx::devapi::parser::parse(source, true));
	} catch (cdk::Error &e) {
		php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_ERR("Parser error for document field");
		DBG_RETURN(false);
	}

	Mysqlx::Expr::ColumnIdentifier identifier(docpath->identifier());

	// Validates the source is an array item
	const size_t size = identifier.document_path().size();
	if (size) {
		DBG_INF_FMT("doc_path_size=%u", static_cast<unsigned int>(size));
		if (validate_array) {
			const Mysqlx::Expr::DocumentPathItem_Type doc_path_type
				= identifier.document_path().Get(static_cast<int>(size) - 1).type();
			DBG_INF_FMT("type=%s", Mysqlx::Expr::DocumentPathItem::Type_Name(doc_path_type).c_str());
			if (doc_path_type != Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX) {
				DBG_ERR("An array document path must be specified");
				DBG_RETURN(false);
			}
		}
	} else if (op_type != Mysqlx::Crud::UpdateOperation::ITEM_MERGE) {
		DBG_ERR("Invalid document path");
		DBG_RETURN(false);
	}

	operation->mutable_source()->CopyFrom(identifier);

	if (!value.is_undef()) {
		if (value.is_string() && (is_expression || is_document)) {
			try {
				const std::string& source(value.to_std_string());
				Mysqlx::Expr::Expr* expr_criteria = parse_expression(source, obj->bindings);
				operation->set_allocated_value( expr_criteria );
			} catch (cdk::Error& e) {
				php_error_docref(nullptr, E_WARNING, "Error while parsing, details: %s", e.what());
				DBG_ERR_FMT("%s", e.what());
				DBG_ERR("Parser error for document field");
				DBG_RETURN(false);
			}
		} else {
			Mysqlx::Datatypes::Any any;
			if (FAIL == zval2any(value.ptr(), any)) {
				DBG_ERR("Error converting the zval to scalar");
				DBG_RETURN(false);
			}
			any2log(any);

			operation->mutable_value()->set_type(Mysqlx::Expr::Expr::LITERAL);
			operation->mutable_value()->set_allocated_literal(any.release_scalar());
		}
	}

	DBG_RETURN(true);
}

bool xmysqlnd_crud_collection_modify__unset(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const util::string_view& path)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REMOVE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__unset");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(
		obj, op_type, Modify_value{ path, util::zvalue(), false, false, false }));
}

bool xmysqlnd_crud_collection_modify__set(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_SET;
	DBG_ENTER("xmysqlnd_crud_collection_modify__set");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool xmysqlnd_crud_collection_modify__replace(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_REPLACE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__replace");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool xmysqlnd_crud_collection_modify__merge(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ITEM_MERGE;
	DBG_ENTER("xmysqlnd_crud_collection_modify__merge");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool xmysqlnd_crud_collection_modify__patch(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::MERGE_PATCH;
	DBG_ENTER("xmysqlnd_crud_collection_modify__patch");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool xmysqlnd_crud_collection_modify__array_insert(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_INSERT;
	DBG_ENTER("xmysqlnd_crud_collection_modify__array_insert");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool xmysqlnd_crud_collection_modify__array_append(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value)
{
	const Mysqlx::Crud::UpdateOperation_UpdateType op_type = Mysqlx::Crud::UpdateOperation::ARRAY_APPEND;
	DBG_ENTER("xmysqlnd_crud_collection_modify__array_append");
	DBG_RETURN(xmysqlnd_crud_collection_modify__add_operation(obj, op_type, modify_value));
}

bool
xmysqlnd_crud_collection_modify__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	bool ret = obj && obj->message.IsInitialized();
	DBG_ENTER("xmysqlnd_crud_collection_modify__is_initialized");
	DBG_INF_FMT("is_initialized=%u", static_cast<unsigned>(ret));
	DBG_RETURN(ret);
}

bool
xmysqlnd_crud_collection_modify__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_modify__finalize_bind");
	DBG_RETURN(obj->bindings.finalize(obj->message.mutable_args()));
}

st_xmysqlnd_pb_message_shell
	xmysqlnd_crud_collection_modify__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj)
{
	st_xmysqlnd_pb_message_shell ret{ (void *) &obj->message, COM_CRUD_UPDATE };
	return ret;
}

/****************************** COLLECTION.FIND() *******************************************************/

XMYSQLND_CRUD_COLLECTION_OP__FIND *
xmysqlnd_crud_collection_find__create(const util::string_view& schema, const util::string_view& object_name)
{
	XMYSQLND_CRUD_COLLECTION_OP__FIND* ret{nullptr};
	DBG_ENTER("xmysqlnd_crud_collection_find__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.length(), schema.data(), object_name.length(), object_name.data());
	ret = new st_xmysqlnd_crud_collection_op__find(schema, object_name);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_collection_find__destroy(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

enum_func_status
xmysqlnd_crud_collection_find__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_criteria");
	try {
		Mysqlx::Expr::Expr* expr_criteria = parse_expression(std::string{ criteria }, obj->bindings);
		obj->message.set_allocated_criteria(expr_criteria);
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
xmysqlnd_crud_collection_find__set_limit(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t limit)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_limit");
	obj->message.mutable_limit()->set_row_count(limit);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_find__set_offset(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_find__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__FIND* obj,
	const util::string& name,
	zval* value)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__bind_value");
	DBG_RETURN(obj->bindings.bind(name, value) ? PASS : FAIL);
}

enum_func_status
xmysqlnd_crud_collection_find__add_sort(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& sort)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_crud_collection_find__add_sort");
	ret = xmysqlnd_crud_collection__add_sort(obj->message, sort);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_collection_find__add_grouping(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& search_field)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__add_grouping");
	try {
		Mysqlx::Expr::Expr* expr_criteria = parse_expression(std::string{ search_field }, obj->bindings, false);
		obj->message.mutable_grouping()->AddAllocated(expr_criteria);
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
xmysqlnd_crud_collection_find__set_fields(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj,
										  const util::string_view& field,
										  const zend_bool /*is_expression*/,
										  const zend_bool allow_alias)
{
	const bool is_document = (obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
	const std::string source(field);
	DBG_ENTER("xmysqlnd_crud_collection_find__set_fields");
	if (allow_alias) {
		try {
			mysqlx::devapi::parser::projection( source,
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
		Mysqlx::Expr::Expr* criteria = parse_expression(source, obj->bindings);

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
		Mysqlx::Expr::Expr* criteria = parse_expression(source, obj->bindings);
		obj->message.mutable_projection()->Add()->set_allocated_source( criteria );
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
xmysqlnd_crud_collection_find__set_having(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__set_having");
	try {
		Mysqlx::Expr::Expr* expr_criteria = parse_expression(std::string{ criteria }, obj->bindings);
		obj->message.set_allocated_grouping_criteria(expr_criteria);
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
xmysqlnd_crud_collection_find__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_find__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

void
xmysqlnd_crud_collection_find_verify_is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj)
{
	if (xmysqlnd_crud_collection_find__is_initialized(obj)) return;

	util::pb::verify_limit_offset(obj->message);

	throw util::xdevapi_exception(util::xdevapi_exception::Code::find_fail);
}

enum_func_status
xmysqlnd_crud_collection_find__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__finalize_bind");
	const enum_func_status ret
		= obj->bindings.finalize(obj->message.mutable_args()) ? PASS : FAIL;
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_crud_collection_find__enable_lock_shared(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__enable_lock_shared");
	obj->message.set_locking(::Mysqlx::Crud::Find_RowLock_SHARED_LOCK);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_find__enable_lock_exclusive(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_find__enable_lock_exclusive");
	obj->message.set_locking(::Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);
	DBG_RETURN(PASS);
}

enum_func_status
xmysqlnd_crud_collection_find_set_lock_waiting_option(
	XMYSQLND_CRUD_COLLECTION_OP__FIND* obj,
	int lock_waiting_option)
{
	DBG_ENTER("xmysqlnd_crud_collection_find_set_lock_waiting_option");
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
		xmysqlnd_crud_collection_find__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_CRUD_FIND };
	return ret;
}

/****************************** SQL EXECUTE *******************************************************/

enum_func_status
st_xmysqlnd_stmt_op__execute::bind_one_param(const zval * param_zv)
{
	DBG_ENTER("st_xmysqlnd_stmt_op__execute::bind_one_stmt_param");
	const unsigned int param_no = params_allocated;
	DBG_RETURN(bind_one_param(param_no, param_zv));
}

enum_func_status
st_xmysqlnd_stmt_op__execute::bind_one_param(const unsigned int param_no, const zval * param_zv)
{
	enum_func_status ret{FAIL};
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

enum_func_status
st_xmysqlnd_stmt_op__execute::finalize_bind()
{
	enum_func_status ret{PASS};
	DBG_ENTER("st_xmysqlnd_stmt_op__execute::finalize_bind");
	for (unsigned int i{0}; i < params_allocated; ++i) {
		Mysqlx::Datatypes::Any * arg = message.add_args();
		ret = zval2any(&(params[i]), *arg);
		if (FAIL == ret) {
			break;
		}

	}
	DBG_RETURN(ret);
}

XMYSQLND_STMT_OP__EXECUTE *
xmysqlnd_stmt_execute__create(const std::string_view& namespace_, const util::string_view& stmt)
{
	XMYSQLND_STMT_OP__EXECUTE* ret{nullptr};
	DBG_ENTER("xmysqlnd_stmt_execute__create");
	DBG_INF_FMT("namespace_=%*s stmt=%*s", namespace_.length(), namespace_.data(), stmt.length(), stmt.data());
	ret = new st_xmysqlnd_stmt_op__execute(namespace_, stmt, false);
	DBG_RETURN(ret);
}

void
xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__destroy");
	delete obj;
	DBG_VOID_RETURN;
}

Mysqlx::Sql::StmtExecute& xmysqlnd_stmt_execute__get_pb_msg(XMYSQLND_STMT_OP__EXECUTE* obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__get_pb_msg");
	DBG_RETURN(obj->message);
}

enum_func_status
xmysqlnd_stmt_execute__bind_one_param_add(XMYSQLND_STMT_OP__EXECUTE * obj, const zval * param_zv)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_one_param_add");
	const enum_func_status ret = obj->bind_one_param(param_zv);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE * obj, const unsigned int param_no, const zval * param_zv)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_one_param");
	const enum_func_status ret = obj->bind_one_param(param_no, param_zv);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE * obj, zval * value)
{
	DBG_ENTER("xmysqlnd_stmt_execute__bind_value");
	Mysqlx::Datatypes::Any * arg = obj->message.add_args();
	const enum_func_status ret = zval2any(value, *arg);
	DBG_RETURN(ret);
}

enum_func_status
xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	DBG_ENTER("xmysqlnd_stmt_execute__finalize_bind");
	const enum_func_status ret = obj->finalize_bind();
	DBG_RETURN(ret);
}

zend_bool
xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_stmt_execute__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}

struct st_xmysqlnd_pb_message_shell
xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message, COM_SQL_STMT_EXECUTE };
	return ret;
}

} // namespace drv

} // namespace mysqlx
