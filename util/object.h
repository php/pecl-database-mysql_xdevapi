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
#ifndef MYSQL_XDEVAPI_PHP_OBJECT_H
#define MYSQL_XDEVAPI_PHP_OBJECT_H

#include "php_api.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_object.h"
#include "exceptions.h"

namespace mysqlx {

const int dont_allow_null = 0;
const int no_pass_by_ref = 0;

//TODO: these should be moved to util
namespace devapi {
struct st_mysqlx_property_entry;
struct st_mysqlx_object;
}

//------------------------------------------------------------------------------

namespace util {

using object_allocator_func_t = zend_object*(zend_class_entry*);

template<typename ... Interfaces>
zend_class_entry* register_interface(
	zend_class_entry* tmp_ce,
	const Interfaces& ... interfaces)
{
	using namespace devapi;

	zend_class_entry* interface_entry = zend_register_internal_interface(tmp_ce);

	auto interfaces_count{ sizeof...(Interfaces) };
	if (interfaces_count) {
		zend_class_implements(
			interface_entry,
			sizeof...(Interfaces),
			interfaces ...);
	}

	return interface_entry;
}

template<typename ... Interfaces>
zend_class_entry* register_class(
	zend_class_entry* tmp_ce,
	zend_object_handlers* std_handlers,
	zend_object_handlers* handlers,
	object_allocator_func_t object_allocator_func,
	zend_object_free_obj_t free_storage_func,
	HashTable* properties,
	const devapi::st_mysqlx_property_entry* property_entries,
	const Interfaces& ... interfaces)
{
	using namespace devapi;

	*handlers = *std_handlers;
	handlers->free_obj = free_storage_func;

	tmp_ce->create_object = object_allocator_func;
	zend_class_entry* class_entry = zend_register_internal_class(tmp_ce);
	auto interfaces_count{ sizeof...(Interfaces) };
	if (interfaces_count) {
		zend_class_implements(
			class_entry,
			sizeof...(Interfaces),
			interfaces ...);
	}

	if (properties) {
		zend_hash_init(properties, 0, nullptr, mysqlx_free_property_cb, 1);

		if (property_entries) {
			/* Add name + getter + setter to the hash table with the properties for the class */
			mysqlx_add_properties(properties, property_entries);
		}
	}

	return class_entry;
}

template<typename ... Interfaces>
zend_class_entry* register_derived_class(
	zend_class_entry* tmp_ce,
	zend_class_entry* base_class_entry,
	zend_object_handlers* std_handlers,
	zend_object_handlers* handlers,
	HashTable* properties,
	const devapi::st_mysqlx_property_entry* property_entries)
{
	using namespace devapi;

	*handlers = *std_handlers;

	zend_class_entry* class_entry = zend_register_internal_class_ex(tmp_ce, base_class_entry);

	if (properties) {
		zend_hash_init(properties, 0, nullptr, mysqlx_free_property_cb, 1);

		if (property_entries) {
			/* Add name + getter + setter to the hash table with the properties for the class */
			mysqlx_add_properties(properties, property_entries);
		}
	}

	return class_entry;
}

template<typename Data_object, typename Allocation_tag = util::alloc_tag_t>
devapi::st_mysqlx_object* alloc_object(
	zend_class_entry* class_type,
	zend_object_handlers* handlers,
	HashTable* properties)
{
	using namespace devapi;

	const std::size_t bytes_count = sizeof(st_mysqlx_object) + zend_object_properties_size(class_type);
	st_mysqlx_object* mysqlx_object = static_cast<st_mysqlx_object*>(::operator new(bytes_count, Allocation_tag()));

	static_assert(
		std::is_base_of<util::internal::allocable<Allocation_tag>, Data_object>::value,
		"proper kind of custom allocation should be applied");
	mysqlx_object->ptr = new Data_object;

	zend_object_std_init(&mysqlx_object->zo, class_type);
	object_properties_init(&mysqlx_object->zo, class_type);

	mysqlx_object->zo.handlers = handlers;
	mysqlx_object->properties = properties;

	return mysqlx_object;
}

template<typename Data_object>
devapi::st_mysqlx_object* alloc_permanent_object(
	zend_class_entry* class_type,
	zend_object_handlers* handlers,
	HashTable* properties)
{
	return alloc_object<Data_object, util::permanent_tag_t>(
		class_type,
		handlers,
		properties);
}

template<typename Data_object>
const Data_object& fetch_data_object(const devapi::st_mysqlx_object* mysqlx_object)
{
	using namespace devapi;

	const Data_object* data_object{ static_cast<const Data_object*>(mysqlx_object->ptr) };
	if (!data_object) {
		throw util::doc_ref_exception(util::doc_ref_exception::Severity::warning, mysqlx_object->zo.ce);
	}
	return *data_object;
}

template<typename Data_object>
Data_object& fetch_data_object(devapi::st_mysqlx_object* mysqlx_object)
{
	using namespace devapi;

	Data_object* data_object{ static_cast<Data_object*>(mysqlx_object->ptr) };
	if (!data_object) {
		throw util::doc_ref_exception(util::doc_ref_exception::Severity::warning, mysqlx_object->zo.ce);
	}
	return *data_object;
}

template<typename Data_object>
Data_object& fetch_data_object(zval* from)
{
	using namespace devapi;

	st_mysqlx_object* mysqlx_object{ Z_MYSQLX_P(from) };
	return fetch_data_object<Data_object>(mysqlx_object);
}

template<typename Data_object>
Data_object& fetch_data_object(zend_object* from)
{
	using namespace devapi;

	st_mysqlx_object* mysqlx_object{ mysqlx_fetch_object_from_zo(from) };
	return fetch_data_object<Data_object>(mysqlx_object);
}

template<typename Data_object>
Data_object& init_object(zend_class_entry* ce, zval* mysqlx_object)
{
	if ((SUCCESS == object_init_ex(mysqlx_object, ce)) && (IS_OBJECT == Z_TYPE_P(mysqlx_object))) {
		auto& data_object = util::fetch_data_object<Data_object>(mysqlx_object);
		return data_object;
	} else {
		throw util::doc_ref_exception(util::doc_ref_exception::Severity::warning, ce);
	}
}

template<typename Data_object>
void free_object(zend_object* object)
{
	using namespace devapi;

	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	Data_object* data_object = static_cast<Data_object*>(mysqlx_object->ptr);
	static_assert(std::is_base_of<custom_allocable, Data_object>::value || std::is_base_of<permanent_allocable, Data_object>::value,
		"custom allocation should be applied");
	delete data_object;
	mysqlx_object_free_storage(object);
}

template<typename Result, typename Result_iterator>
zend_object_iterator* create_result_iterator(
	zend_class_entry* /*ce*/,
	zend_object_iterator_funcs* result_iterator_funcs,
	zval* object,
	int by_ref)
{
	using namespace devapi;

	st_mysqlx_object* mysqlx_object = Z_MYSQLX_P(object);
	Result* mysqlx_result = mysqlx_object->ptr ? static_cast<Result*>(mysqlx_object->ptr) : nullptr;

	if (by_ref) {
		zend_error(E_ERROR, "An iterator cannot be used with foreach by reference");
		return nullptr;
	}

	Result_iterator* iterator = new Result_iterator();
	if (iterator) {
		zend_iterator_init(&iterator->intern);

		ZVAL_COPY(&iterator->intern.data, object);

		iterator->intern.funcs = result_iterator_funcs;
		iterator->row_num = 0;
		iterator->started = false;
		iterator->usable = true;
		iterator->result = mysqlx_result->result->m.get_reference(mysqlx_result->result);
	}

	return &iterator->intern;
}

//------------------------------------------------------------------------------

using php_method_t = void(INTERNAL_FUNCTION_PARAMETERS);
using php_function_t = void(INTERNAL_FUNCTION_PARAMETERS);

void safe_call_php_method(php_method_t handler, INTERNAL_FUNCTION_PARAMETERS);
void safe_call_php_function(php_function_t handler, INTERNAL_FUNCTION_PARAMETERS);

} // namespace util

} // namespace mysqlx

#define MYSQL_XDEVAPI_REGISTER_INTERFACE( \
	class_entry, \
	class_name, \
	methods, \
	...) \
{ \
	zend_class_entry tmp_ce; \
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", class_name, methods); \
	class_entry = util::register_interface( \
		&tmp_ce, \
		##__VA_ARGS__); \
}

#define MYSQL_XDEVAPI_REGISTER_CLASS( \
	class_entry, \
	class_name, \
	std_handlers, \
	handlers, \
	object_allocator_func, \
	free_storage_func, \
	methods, \
	properties, \
	property_entries, \
	...) \
{ \
	zend_class_entry tmp_ce; \
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", class_name, methods); \
	class_entry = util::register_class( \
		&tmp_ce, \
		std_handlers, \
		&handlers, \
		object_allocator_func, \
		free_storage_func, \
		&properties, \
		property_entries, \
		##__VA_ARGS__); \
}

#define MYSQL_XDEVAPI_REGISTER_DERIVED_CLASS( \
	class_entry, \
	base_class_entry, \
	class_name, \
	std_handlers, \
	handlers, \
	methods, \
	properties, \
	property_entries, \
	...) \
{ \
	zend_class_entry tmp_ce; \
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", class_name, methods); \
	class_entry = util::register_derived_class( \
		&tmp_ce, \
		base_class_entry, \
		std_handlers, \
		&handlers, \
		&properties, \
		property_entries, \
		##__VA_ARGS__); \
}

#define	MYSQL_XDEVAPI_PHP_METHOD(class_name, name) \
static void class_name##_##name##_body(INTERNAL_FUNCTION_PARAMETERS); \
static PHP_METHOD(class_name, name) \
{ \
	util::safe_call_php_method(class_name##_##name##_body, INTERNAL_FUNCTION_PARAM_PASSTHRU); \
} \
static void class_name##_##name##_body(INTERNAL_FUNCTION_PARAMETERS)


#define	MYSQL_XDEVAPI_PHP_FUNCTION(name) \
static void function_##name##_body(INTERNAL_FUNCTION_PARAMETERS); \
PHP_FUNCTION(name) \
{ \
	util::safe_call_php_function(function_##name##_body, INTERNAL_FUNCTION_PARAM_PASSTHRU); \
} \
static void function_##name##_body(INTERNAL_FUNCTION_PARAMETERS)

#endif // MYSQL_XDEVAPI_PHP_OBJECT_H
