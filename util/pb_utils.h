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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_UTIL_PB_UTILS_H
#define MYSQL_XDEVAPI_UTIL_PB_UTILS_H

#include "util/strings.h"
#include "xmysqlnd/proto_gen/mysqlx_datatypes.pb.h"
#include <optional>

namespace google { namespace protobuf { namespace io { class CodedInputStream; } } }

namespace Mysqlx { namespace Sql { class StmtExecute; } }

namespace Mysqlx { namespace Crud { class Find; } }

namespace mysqlx {

namespace util {

class zvalue;

namespace pb {

using Any = Mysqlx::Datatypes::Any;
using Array = Mysqlx::Datatypes::Array;
using Object = Mysqlx::Datatypes::Object;

// -----------------------------------------------------------------------------

bool read_variant_64(::google::protobuf::io::CodedInputStream& input_stream, uint64_t* value);

// -----------------------------------------------------------------------------

void to_any(std::nullptr_t /*nil*/, Any& any);

void to_any(bool value, Any& any);

void to_any(int value, Any& any);
void to_any(long value, Any& any);
void to_any(long long value, Any& any);

void to_any(unsigned int value, Any& any);
void to_any(unsigned long value, Any& any);
void to_any(unsigned long long value, Any& any);

void to_any(float value, Any& any);
void to_any(double value, Any& any);

void to_any(const char* str,const size_t length, Any& any);
void to_any(const std::string& value, Any& any);
void to_any(const util::string& value, Any& any);
void to_any(const util::string_view& value, Any& any);

void to_any(const util::zvalue& zv, Any& any);

void to_any(Object* value, Any& any);
void to_any(Array* value, Any& any);

// -----------------------------------------------------------------------------

template<typename T>
void add_field_to_object(const char* key, T value, Object* pb_obj)
{
	Mysqlx::Datatypes::Object_ObjectField* field = pb_obj->add_fld();
	field->set_key(key);
	Any* pb_value = field->mutable_value();
	to_any(value, *pb_value);
}

template<typename T>
void add_field_to_object(const util::string& key, T value, Object* pb_obj)
{
	add_field_to_object(key.c_str(), value, pb_obj);
}

// ------

template<typename T>
void add_field_to_object(const char* key, T value, std::unique_ptr<Object>& pb_obj)
{
	add_field_to_object(key, value, pb_obj.get());
}

template<typename T>
void add_field_to_object(const util::string& key, T value, std::unique_ptr<Object>& pb_obj)
{
	add_field_to_object(key, value, pb_obj.get());
}

// ------

template<typename K, typename T>
void add_optional_field_to_object(
	const K& key,
	const std::optional<T>& value,
	Object* pb_obj)
{
	if (value) {
		add_field_to_object(key, *value, pb_obj);
	}
}

template<typename K, typename T>
void add_optional_field_to_object(
	const K& key,
	const std::optional<T>& value,
	std::unique_ptr<Object>& pb_obj)
{
	add_optional_field_to_object(key, value, pb_obj.get());
}

// -----------------------------------------------------------------------------

template<typename T>
void add_value_to_array(T value, Array* pb_array)
{
	Any* array_element{pb_array->add_value()};
	to_any(value, *array_element);
}

template<typename T>
void add_value_to_array(T value, std::unique_ptr<Array>& pb_array)
{
	add_value_to_array(value, pb_array.get());
}

// -----------------------------------------------------------------------------

Object* add_object_arg(Mysqlx::Sql::StmtExecute& stmt_message);
Array* add_array_arg(Mysqlx::Sql::StmtExecute& stmt_message);

// -----------------------------------------------------------------------------

void verify_limit_offset(const Mysqlx::Crud::Find& message);

} // namespace pb

} // namespace util

} // namespace mysqlx

#endif /* MYSQL_XDEVAPI_UTIL_PB_UTILS_H */
