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
#ifndef MYSQL_XDEVAPI_UTIL_VALUE_H
#define MYSQL_XDEVAPI_UTIL_VALUE_H

#include "strings.h"
#include "string_utils.h"
#include <optional>

namespace mysqlx::util {

class zvalue
{
	public:
		enum class Type {
			Undefined = IS_UNDEF,
			Null = IS_NULL,
			False = IS_FALSE,
			True = IS_TRUE,
			Long = IS_LONG,
			Double = IS_DOUBLE,
			String = IS_STRING,
			Array = IS_ARRAY,
			Object = IS_OBJECT,
			Resource = IS_RESOURCE,
			Reference = IS_REFERENCE,
			Constant_ast = IS_CONSTANT_AST,
			Internal_type_bool = _IS_BOOL,
			Callable = IS_CALLABLE,
			Void = IS_VOID
		};

	public:
		zvalue();
		zvalue(Type type);
		zvalue(const zvalue& rhs);
		zvalue(zvalue&& rhs);
		zvalue(const zval& zv);
		zvalue(zval&& zv);
		zvalue(const zval* zv);

		zvalue(std::nullptr_t value);
		zvalue(bool value);
		zvalue(int32_t value);
		zvalue(int64_t value);
		zvalue(uint32_t value);
		zvalue(uint64_t value);
		zvalue(double value);

		zvalue(char value);
		zvalue(const string& value);
		zvalue(const string_view& value);
		zvalue(const std::string& value);
		zvalue(const char* value);
		zvalue(const char* value, std::size_t length);

		zvalue(std::initializer_list<std::pair<const char*, zvalue>> values);

		template<typename T>
		zvalue(std::initializer_list<T> values);

		template<typename Iterator>
		zvalue(Iterator begin, Iterator end);

		template<typename T>
		zvalue(const vector<T>& values);

		template<typename T>
		zvalue(const set<T>& values);

		template<typename Key, typename Value>
		zvalue(const map<Key, Value>& values);

		static zvalue create_array(std::size_t size = 0);
		static zvalue create_object();

		~zvalue();

	public:
		// assignment
		zvalue& operator=(const zvalue& rhs);
		zvalue& operator=(zvalue&& rhs);
		zvalue& operator=(const zval& rhs);
		zvalue& operator=(zval&& rhs);
		zvalue& operator=(const zval* rhs);

		zvalue& operator=(std::nullptr_t value);
		zvalue& operator=(bool value);
		zvalue& operator=(int32_t value);
		zvalue& operator=(int64_t value);
		zvalue& operator=(uint32_t value);
		zvalue& operator=(uint64_t value);
		zvalue& operator=(double value);

		zvalue& operator=(char value);
		zvalue& operator=(const string& value);
		zvalue& operator=(const string_view& value);
		zvalue& operator=(const std::string& value);
		zvalue& operator=(const char* value);

		void assign(const char* value, std::size_t length);

		zvalue& operator=(std::initializer_list<std::pair<const char*, zvalue>> values);

		template<typename T>
		zvalue& operator=(std::initializer_list<T> values);

		template<typename T>
		zvalue& operator=(const vector<T>& values);

		template<typename T>
		zvalue& operator=(const set<T>& values);

		template<typename Key, typename Value>
		zvalue& operator=(const map<Key, Value>& values);

	public:
		Type type() const;

		bool is_undef() const;
		bool is_null() const;
		bool is_false() const;
		bool is_true() const;
		bool is_bool() const;
		bool is_long() const;
		bool is_double() const;
		bool is_string() const;
		bool is_array() const;
		bool is_object() const;
		bool is_reference() const;

		// returns true if type is neither undefined nor null (i.e. type != Undefined && type != Null)
		bool has_value() const;

	public:
		// call below methods only if U are fully sure they have proper type (or it may crash)
		// else use to_optional_value or to_obligatory_value
		bool to_bool() const;

		int to_int() const;
		long to_long() const;
		zend_long to_zlong() const;
		int32_t to_int32() const;
		int64_t to_int64() const;

		unsigned int to_uint() const;
		unsigned long to_ulong() const;
		zend_ulong to_zulong() const;
		std::size_t to_size_t() const;
		uint32_t to_uint32() const;
		uint64_t to_uint64() const;

		double to_double() const;

		string to_string() const;
		string_view to_string_view() const;
		std::string to_std_string() const;
		const char* c_str() const;

		template<typename T>
		T to_value() const;

		// if value is undefined or has incompatible type, then return std::nullopt
		template<typename T>
		std::optional<T> to_optional_value() const;

		// if value is undefined or has incompatible type, then throws exception
		template<typename T>
		T to_obligatory_value() const;

	public:
		// applicable only for string and array
		std::size_t size() const;
		std::size_t length() const;

		bool empty() const;

		// clears current value, but keeps current type - applicable only for string and array
		void clear();

		// inits array with storage reserved for specified number of items
		void reserve(std::size_t size = 0);

		// frees current value, and sets type to undefined
		void reset();

		// returns duplicate - if it is reference type, it will be
		// new fully separated item, not just next reference incremented
		zvalue clone() const;

		static zvalue clone_from(zval* src);

		// take over ownership of passed zval
		void acquire(zval* zv);
		void acquire(zval& zv);

		// copies value to zv
		void copy_to(zval* zv);
		void copy_to(zval& zv);

		static void copy_to(zval* src, zval* dst);

		// moves value to zv, and sets type to undefined
		void move_to(zval* zv);
		void move_to(zval& zv);

		static void move_to(zval* src, zval* dst);

		// increment / decrement reference counter if type is refcounted, else does nothing
		void inc_ref() const;
		void dec_ref() const;

		// returns holded value, and sets type to undefined
		zval release();

		// set to undefined without releasing currently kept value
		void invalidate();

		void swap(zvalue& rhs) noexcept;

	public:
		// object
		std::size_t properties_count() const;

		bool has_property(const string& name) const;
		bool has_property(const string_view& name) const;
		bool has_property(const std::string& name) const;
		bool has_property(const char* name) const;
		bool has_property(const char* name, std::size_t name_length) const;

		// in case property doesn't exists it returns 'undefined' zvalue, i.e. is_undef() == true
		zvalue get_property(const string& name) const;
		zvalue get_property(const string_view& name) const;
		zvalue get_property(const std::string& name) const;
		zvalue get_property(const char* name) const;
		zvalue get_property(const char* name, std::size_t name_length) const;

		// in case property doesn't exists it throws an exception
		zvalue require_property(const string& name) const;
		zvalue require_property(const string_view& name) const;
		zvalue require_property(const std::string& name) const;
		zvalue require_property(const char* name) const;
		zvalue require_property(const char* name, std::size_t name_length) const;

		void set_property(const string& name, const zvalue& value);
		void set_property(const string_view& name, const zvalue& value);
		void set_property(const std::string& name, const zvalue& value);
		void set_property(const char* name, const zvalue& value);
		void set_property(const char* name, std::size_t name_length, const zvalue& value);

		void unset_property(const string& name);
		void unset_property(const string_view& name);
		void unset_property(const std::string& name);
		void unset_property(const char* name);
		void unset_property(const char* name, std::size_t name_length);

	public:
		// array / hash_table
		bool contains(std::size_t index) const;
		bool contains(long index) const;

		bool contains(const string& key) const;
		bool contains(const string_view& key) const;
		bool contains(const std::string& key) const;
		bool contains(const char* key) const;
		bool contains(const char* key, std::size_t key_length) const;

		// it returns value, not reference!
		// returns empty result (is_undef() == true) in case given index/key not found
		zvalue find(std::size_t index) const;
		zvalue find(long index) const;

		zvalue find(const string& key) const;
		zvalue find(const string_view& key) const;
		zvalue find(const std::string& key) const;
		zvalue find(const char* key) const;
		zvalue find(const char* key, std::size_t key_length) const;

		// it returns value, not reference!
		// returns empty result (is_undef() == true) in case given index/key not found
		zvalue operator[](std::size_t index) const;
		zvalue operator[](long index) const;

		zvalue operator[](const string& key) const;
		zvalue operator[](const string_view& key) const;
		zvalue operator[](const std::string& key) const;
		zvalue operator[](const char* key) const;

		// it returns value, not reference!
		// throws an exception in case given index/key not found
		zvalue at(std::size_t index) const;
		zvalue at(long index) const;

		zvalue at(const string& key) const;
		zvalue at(const string_view& key) const;
		zvalue at(const std::string& key) const;
		zvalue at(const char* key) const;
		zvalue at(const char* key, std::size_t key_length) const;

		// inserts new item, or overwrites existing one if given index/key already exists
		void insert(std::size_t index, const zvalue& value);
		void insert(long index, const zvalue& value);

		void insert(std::size_t index, zvalue&& value);
		void insert(long index, zvalue&& value);

		void insert(const string& key, const zvalue& value);
		void insert(const string_view& key, const zvalue& value);
		void insert(const std::string& key, const zvalue& value);
		void insert(const char* key, const zvalue& value);
		void insert(const char* key, std::size_t key_length, const zvalue& value);

		void insert(const string& key, zvalue&& value);
		void insert(const string_view& key, zvalue&& value);
		void insert(const std::string& key, zvalue&& value);
		void insert(const char* key, zvalue&& value);
		void insert(const char* key, std::size_t key_length, zvalue&& value);

		template<typename Key, typename Value>
		void insert(const std::pair<Key, Value>& key_value);

		void insert(std::initializer_list<std::pair<const char*, zvalue>> values);

		template<typename Key, typename Value>
		void append(std::initializer_list<std::pair<Key, Value>> values);

		// adds new item at the next free index
		void push_back(const zvalue& value);
		void push_back(zvalue&& value);

		template<typename T>
		void push_back(std::initializer_list<T> values);

		// returns true if item was erased, or false if given index/key didn't exist
		bool erase(std::size_t index);
		bool erase(long index);

		bool erase(const string& key);
		bool erase(const string_view& key);
		bool erase(const std::string& key);
		bool erase(const char* key);
		bool erase(const char* key, std::size_t key_length);

	public:
		// to iterate through array items as pair<key, value>
		class iterator
		{
			public:
				using value_type = std::pair<zvalue, zvalue>;
				using difference_type = std::ptrdiff_t;
				using pointer = value_type*;
				using reference = value_type&;
				using iterator_category = std::forward_iterator_tag;

			public:
				explicit iterator(HashTable* ht, HashPosition size, HashPosition pos);

				iterator operator++(int);
				iterator& operator++();

				value_type operator*() const;

				bool operator==(const iterator& rhs) const;
				bool operator!=(const iterator& rhs) const;

			private:
				HashTable* ht;
				HashPosition size;
				mutable HashPosition pos;
		};

		iterator begin() const;
		iterator end() const;

	public:
		// to iterate through array items as keys (values are not returned)
		class key_iterator
		{
			public:
				using key_type = zvalue;
				using difference_type = std::ptrdiff_t;
				using pointer = key_type*;
				using reference = key_type&;
				using iterator_category = std::forward_iterator_tag;

			public:
				explicit key_iterator(HashTable* ht, HashPosition size, HashPosition pos);

				key_iterator operator++(int);
				key_iterator& operator++();

				key_type operator*() const;

				bool operator==(const key_iterator& rhs) const;
				bool operator!=(const key_iterator& rhs) const;

			private:
				HashTable* ht;
				HashPosition size;
				mutable HashPosition pos;
		};

		key_iterator kbegin() const;
		key_iterator kend() const;

		struct keys_range
		{
			keys_range(const zvalue& ref);
			key_iterator begin() const;
			key_iterator end() const;
			const zvalue& ref;
		};

		keys_range keys() const;

	public:
		// to iterate through array items as zvalues (keys are not returned)
		class value_iterator
		{
			public:
				using value_type = zvalue;
				using difference_type = std::ptrdiff_t;
				using pointer = value_type*;
				using reference = value_type&;
				using iterator_category = std::forward_iterator_tag;

			public:
				explicit value_iterator(HashTable* ht, HashPosition size, HashPosition pos);

				value_iterator operator++(int);
				value_iterator& operator++();

				value_type operator*() const;

				bool operator==(const value_iterator& rhs) const;
				bool operator!=(const value_iterator& rhs) const;

			private:
				HashTable* ht;
				HashPosition size;
				mutable HashPosition pos;
		};

		value_iterator vbegin() const;
		value_iterator vend() const;

		struct values_range
		{
			values_range(const zvalue& ref);
			value_iterator begin() const;
			value_iterator end() const;
			const zvalue& ref;
		};

		values_range values() const;

	public:
		const zval& c_ref() const;
		zval& ref() const;

		const zval* c_ptr() const;
		zval* ptr() const;

	public:
		// diagnostics
		util::string serialize() const;

	private:
		zval zv;
};

} // namespace mysqlx::util

#include "value.inl"

#endif // MYSQL_XDEVAPI_UTIL_VALUE_H
