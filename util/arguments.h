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
#ifndef MYSQL_XDEVAPI_UTIL_ARGUMENTS_H
#define MYSQL_XDEVAPI_UTIL_ARGUMENTS_H

#include "strings.h"
#include "value.h"

namespace mysqlx::util {

/*
	it is meant to work with PHP methods as wrapper for string params coming
	from util::get_method_arguments
	in general it keeps pointer/len of string parameter, has some helper routines, and its contents
	INVALIDATES when called from MYSQL_XDEVAPI_PHP_METHOD ends

	common scenario:

	0)
	util::param_string index_name;
	[...]
	if (FAILURE == util::get_method_arguments(
		execute_data, getThis(), "Os+",
		&object_zv, table_create_class_entry,
		&index_name.str, &index_name.len))

	1) then optionally make some checks (whether is empty or make some comparison
	like == ), or immediately get proper util::string via to_string() member routine
*/
struct param_string
{
	param_string() = default;
	param_string(const char* cstr);

	bool empty() const;

	string_view to_view() const;
	std::string_view to_std_view() const;

	string to_string() const;
	std::string to_std_string() const;

	const char* c_str() const;
	const char* data() const;

	size_t length() const;
	size_t size() const;

	const char* str{nullptr};
	std::size_t len{0};
};

// -----------------------------------------------------------------------------

struct arg_zvals
{
	arg_zvals() = default;
	arg_zvals(raw_zval* data, int size);

	bool empty() const;
	std::size_t size() const;

	class iterator
	{
		public:
			using value_type = zvalue;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = std::forward_iterator_tag;

		public:
			explicit iterator(const raw_zval* data);

			iterator operator++(int);
			iterator& operator++();

			value_type operator*() const;

			bool operator==(const iterator& rhs) const;
			bool operator!=(const iterator& rhs) const;

		private:
			const raw_zval* it;
	};

	iterator begin() const;
	iterator end() const;

	raw_zval* data = nullptr;
	int counter = 0;
};

} // namespace mysqlx::util

#include "arguments.inl"

#endif // MYSQL_XDEVAPI_UTIL_ARGUMENTS_H
