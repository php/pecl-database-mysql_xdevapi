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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
#include <zend_smart_str.h>
}
#include "json_utils.h"
#include "exceptions.h"
#include "hash_table.h"
#include "value.h"

#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif

namespace mysqlx {

namespace util {

namespace json {

void to_zv_string(zval* src, zval* dest)
{
	smart_str buf = { 0 };
	JSON_G(error_code) = PHP_JSON_ERROR_NONE;
	JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;
	const int encode_flag = (Z_TYPE_P(src) == IS_OBJECT) ? PHP_JSON_FORCE_OBJECT : 0;
	php_json_encode(&buf, src, encode_flag);

	if (JSON_G(error_code) != PHP_JSON_ERROR_NONE) {
		smart_str_free(&buf);
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}

	//TODO marines: there is fockup with lack of terminating zero, which makes troubles in
	// xmysqlnd_json_string_find_id, i.e. php_json_yyparse returns result != 0
	if (buf.s->len < buf.a)
	{
		buf.s->val[buf.s->len] = '\0';
	}
	ZVAL_UNDEF(dest);
	ZVAL_STRINGL(dest, buf.s->val, buf.s->len);
	smart_str_free(&buf);
}

util::zvalue to_zv_string(const util::zvalue& src)
{
	util::zvalue dest;
	to_zv_string(src.ptr(), dest.ptr());
	return dest;
}

util::zvalue to_zv_object(const char* src, const std::size_t src_len)
{
	util::zvalue dest;
	if (php_json_decode(
		dest.ptr(),
		const_cast<char*>(src),
		static_cast<int>(src_len),
		false,
		PHP_JSON_PARSER_DEFAULT_DEPTH) != SUCCESS)
	{
		throw xdevapi_exception(xdevapi_exception::Code::json_parse_error);
	}
	return dest;
}

namespace {

bool is_first_char(const util::zvalue& value, const char chr)
{
	assert(value.is_string());
	if (value.empty()) return false;
	const char* value_str = value.c_str();
	return *value_str == chr;
}

} // anonymous namespace

bool can_be_document(const util::zvalue& value)
{
	return is_first_char(value, '{');
}

bool can_be_array(const util::zvalue& value)
{
	return is_first_char(value, '[');
}

bool can_be_binding(const util::zvalue& value)
{
	return is_first_char(value, ':');
}

namespace {

class Ensure_doc_id
{
	public:
		Ensure_doc_id(
			zval* raw_doc,
			const string_view& doc_id);

	public:
		util::zvalue run();

	private:
		void process_string();
		void process_array();
		void process_object();

		void decode_json(zval* doc_as_str);
		void store_id();

	private:
		zval* raw_doc{nullptr};
		const string_view& doc_id;
		util::zvalue doc_with_id;

};

//------------------------------------------------------------------------------

Ensure_doc_id::Ensure_doc_id(zval* src, const string_view& id)
	: raw_doc(src)
	, doc_id(id)
{
}

util::zvalue Ensure_doc_id::run()
{
	switch(Z_TYPE_P(raw_doc)) {
		case IS_STRING:
			process_string();
			break;

		case IS_ARRAY:
			process_array();
			break;

		case IS_OBJECT:
			process_object();
			break;

		default:
			throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
	return doc_with_id;
}

void Ensure_doc_id::process_string()
{
	decode_json(raw_doc);
	store_id();
}

void Ensure_doc_id::process_array()
{
	HashTable* ht = Z_ARRVAL_P(raw_doc);
	if (zend_array_count(ht) <= 0) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}

	doc_with_id = zvalue::clone_from(raw_doc);
	store_id();
}

void Ensure_doc_id::process_object()
{
	util::zvalue doc_as_str;
	to_zv_string(raw_doc, doc_as_str.ptr());
	decode_json(doc_as_str.ptr());
	store_id();
}

void Ensure_doc_id::decode_json(zval* doc_as_str)
{
	assert(Z_TYPE_P(doc_as_str) == IS_STRING);
	const char* doc_str = Z_STRVAL_P(doc_as_str);
	std::size_t doc_len = static_cast<std::size_t>(Z_STRLEN_P(doc_as_str));
	doc_with_id = to_zval(doc_str, doc_len);
	if (!doc_with_id.is_array() && !doc_with_id.is_object()) {
		throw xdevapi_exception(xdevapi_exception::Code::json_fail);
	}
}

void Ensure_doc_id::store_id()
{
	const char* Id_column_name = "_id";
	switch (doc_with_id.type()) {
	case util::zvalue::Type::Array:
		doc_with_id.insert(Id_column_name, doc_id);
		break;

	case util::zvalue::Type::Object:
		doc_with_id.set_property(Id_column_name, doc_id);
		break;

	default:
		assert(!"unexpected doc type!");
	}
}

} // anonymous namespace

util::zvalue ensure_doc_id(
	zval* raw_doc,
	const string_view& id)
{
	Ensure_doc_id edi(raw_doc, id);
	return edi.run();
}

//------------------------------------------------------------------------------

namespace {

class Json_to_zval
{
public:
	util::zvalue run(
		const char* doc,
		std::size_t doc_len);

private:
	util::zvalue to_number(const rapidjson::Value& value);
	util::zvalue to_string(const rapidjson::Value& value);
	util::zvalue to_array(const rapidjson::Value& value);
	util::zvalue to_object(const rapidjson::Value& value);
	util::zvalue to_value(const rapidjson::Value& value);
};

util::zvalue Json_to_zval::run(
	const char* doc,
	std::size_t doc_len)
{
	rapidjson::Document document;
	document.Parse(doc, doc_len);

	if (document.HasParseError()) {
		util::ostringstream err;
		err << "(character " << document.GetErrorOffset() << "): "
			<< GetParseError_En(document.GetParseError());
		throw xdevapi_exception(xdevapi_exception::Code::json_parse_error, err.str());
	}
	return to_value(document);
}

util::zvalue Json_to_zval::to_number(const rapidjson::Value& value)
{
	if (value.IsInt()) {
		return value.GetInt();
	} else if (value.IsInt64()) {
		return value.GetInt64();
	} else if (value.IsUint()) {
		return value.GetUint();
	} else if (value.IsUint64()) {
		return value.GetUint64();
	} else if (value.IsDouble()) {
		return value.GetDouble();
	} else {
		assert(!"unknown numeric type");
		return zvalue();
	}
}

util::zvalue Json_to_zval::to_string(const rapidjson::Value& value)
{
	return value.GetString();
}

util::zvalue Json_to_zval::to_array(const rapidjson::Value& value)
{
	util::zvalue arr(zvalue::create_array(value.Size()));
	for (const auto& elem : value.GetArray()) {
		arr.push_back(to_value(elem));
	}
	return arr;
}

util::zvalue Json_to_zval::to_object(const rapidjson::Value& value)
{
	util::zvalue obj(zvalue::create_object());
	for (const auto& member : value.GetObject()) {
		obj.set_property(
			member.name.GetString(),
			to_value(member.value));
	}
	return obj;
}

util::zvalue Json_to_zval::to_value(const rapidjson::Value& value)
{
	switch (value.GetType()) {
		case rapidjson::kNullType:
			return nullptr;

		case rapidjson::kFalseType:
			return false;

		case rapidjson::kTrueType:
			return true;

		case rapidjson::kNumberType:
			return to_number(value);

		case rapidjson::kStringType:
			return to_string(value);

		case rapidjson::kArrayType:
			return to_array(value);

		case rapidjson::kObjectType:
			return to_object(value);

		default:
			assert(!"unknown type!");
			return zvalue();
	}
}

} // anonymouse namespace

util::zvalue to_zval(const char* doc, std::size_t doc_len)
{
	Json_to_zval json_to_zval;
	return json_to_zval.run(doc, doc_len);
}

//------------------------------------------------------------------------------

namespace {

class Json_to_any
{
public:
	void run(
		const char* doc,
		std::size_t doc_len,
		Mysqlx::Datatypes::Any& result);

private:
	void to_null(Mysqlx::Datatypes::Any& any);
	void to_boolean(bool value, Mysqlx::Datatypes::Any& any);
	void to_number(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any);
	void to_string(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any);
	void to_array(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any);
	void to_object(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any);
	void to_value(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any);
};

void Json_to_any::run(
	const char* doc,
	std::size_t doc_len,
	Mysqlx::Datatypes::Any& result)
{
	rapidjson::Document document;
	document.Parse(doc, doc_len);

	if (document.HasParseError()) {
		util::ostringstream err;
		err << "(character " << document.GetErrorOffset() << "): "
			<< GetParseError_En(document.GetParseError());
		throw xdevapi_exception(xdevapi_exception::Code::json_parse_error, err.str());
	}
	to_value(document, result);
}

void Json_to_any::to_null(Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_NULL);
}

void Json_to_any::to_boolean(bool value, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_BOOL);
	any.mutable_scalar()->set_v_bool(value);
}

void Json_to_any::to_number(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_SCALAR);

	if (value.IsInt()) {
		any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_SINT);
		any.mutable_scalar()->set_v_signed_int(value.GetInt());
	} else if (value.IsInt64()) {
		any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_SINT);
		any.mutable_scalar()->set_v_signed_int(value.GetInt64());
	} else if (value.IsUint()) {
		any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_UINT);
		any.mutable_scalar()->set_v_unsigned_int(value.GetUint());
	} else if (value.IsUint64()) {
		any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_UINT);
		any.mutable_scalar()->set_v_unsigned_int(value.GetUint64());
	} else if (value.IsDouble()) {
		any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_DOUBLE);
		any.mutable_scalar()->set_v_double(value.GetDouble());
	} else {
		assert(!"unknown numeric type");
	}
}

void Json_to_any::to_string(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_SCALAR);
	any.mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar_Type_V_STRING);
	any.mutable_scalar()->mutable_v_string()->set_value(value.GetString());
}

void Json_to_any::to_array(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_ARRAY);
	for (const auto& elem : value.GetArray()) {
		Mysqlx::Datatypes::Any* any_elem = any.mutable_array()->add_value();
		to_value(elem, *any_elem);
	}
}

void Json_to_any::to_object(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any)
{
	any.set_type(Mysqlx::Datatypes::Any_Type_OBJECT);

	Mysqlx::Datatypes::Object* obj{ any.mutable_obj() };
	for (const auto& member : value.GetObject()) {
		Mysqlx::Datatypes::Object_ObjectField* field{ obj->add_fld() };
		field->set_key(member.name.GetString());
		Mysqlx::Datatypes::Any* field_value{ field->mutable_value() };
		to_value(member.value, *field_value);
	}
}

void Json_to_any::to_value(const rapidjson::Value& value, Mysqlx::Datatypes::Any& any)
{
	switch (value.GetType()) {
		case rapidjson::kNullType:
			to_null(any);
			break;

		case rapidjson::kFalseType:
			to_boolean(false, any);
			break;

		case rapidjson::kTrueType:
			to_boolean(true, any);
			break;

		case rapidjson::kNumberType:
			to_number(value, any);
			break;

		case rapidjson::kStringType:
			to_string(value, any);
			break;

		case rapidjson::kArrayType:
			to_array(value, any);
			break;

		case rapidjson::kObjectType:
			to_object(value, any);
			break;

		default:
			assert(!"unknown type!");
	}
}

} // anonymouse namespace

void to_any(const char* doc, std::size_t doc_len, Mysqlx::Datatypes::Any& any)
{
	Json_to_any json_to_any;
	json_to_any.run(doc, doc_len, any);
}

} // namespace json

} // namespace util

} // namespace mysqlx
