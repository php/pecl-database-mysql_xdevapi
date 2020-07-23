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
#ifndef MYSQL_XDEVAPI_UTIL_STRINGS_H
#define MYSQL_XDEVAPI_UTIL_STRINGS_H

#include "php_api.h"
extern "C" {
struct st_mysqlnd_string;
struct st_mysqlnd_const_string;
}
#include <string>
#include <string_view>
#include <sstream>
#include <cstring>
#include <boost/format.hpp>
#include "allocator.h"
#include "types.h"

namespace mysqlx {

namespace util {

using string_view = std::string_view;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string = std::basic_string<CharT, Traits, allocator<CharT>>;
using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

using strings = vector<string>;
using stringset = set<string>;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_ostringstream = std::basic_ostringstream<CharT, Traits, allocator<CharT>>;
using ostringstream = basic_ostringstream<char>;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_istringstream = std::basic_istringstream<CharT, Traits, allocator<CharT>>;
using istringstream = basic_istringstream<char>;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_stringstream = std::basic_stringstream<CharT, Traits, allocator<CharT>>;
using stringstream = basic_stringstream<char>;

std::ostream& operator<<(std::ostream& os, const string& str);

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_formatter = boost::basic_format<CharT, Traits, allocator<CharT>>;
using formatter = basic_formatter<char>;
using wformatter = basic_formatter<wchar_t>;

/*
	it is meant to work with PHP methods as wrapper for string params coming
	from util::zend::parse_method_parameters
	in general it keeps pointer/len of string parameter, has some helper routines, and its contents
	INVALIDATES when called from MYSQL_XDEVAPI_PHP_METHOD ends

	common scenario:

	0)
	util::param_string index_name;
	[...]
	if (FAILURE == util::zend::parse_method_parameters(
		execute_data, getThis(), "Os+",
		&object_zv, table_create_class_entry,
		&index_name.str, &index_name.len))

	1) then optionally make some checks (whether is empty or make some comparison
	like == ), or immediately get proper util::string via to_string() member routine
*/
struct param_string
{
	param_string() = default;
	param_string(const char* cstr)
		: str(cstr)
		, len(std::strlen(cstr))
	{}

	bool empty() const
	{
		return (str == nullptr) || (*str == '\0');
	}

	string_view to_view() const
	{
		return string_view(str, len);
	}

	std::string_view to_std_view() const
	{
		return std::string_view(str, len);
	}

	string to_string() const
	{
		return string(str, len);
	}

	std::string to_std_string() const
	{
		return std::string(str, len);
	}

	const char* c_str() const
	{
		return str;
	}

	const char* data() const
	{
		return str;
	}

	size_t length() const
	{
		return len;
	}

	size_t size() const
	{
		return len;
	}

	const char* str{nullptr};
	std::size_t len{0};
};

// ------------------------------------------------------------------------------

using std_strings = std::vector<std::string>;
using std_stringset = std::set<std::string>;

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_STRINGS_H
