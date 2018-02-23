/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#ifndef MYSQL_XDEVAPI_PHPUTILS_STRINGS_H
#define MYSQL_XDEVAPI_PHPUTILS_STRINGS_H

#include "php_api.h"
extern "C" {
struct st_mysqlnd_string;
struct st_mysqlnd_const_string;
}
#include <string>
#include <sstream>
#include <cstring>
#include <boost/format.hpp>
#include "allocator.h"
#include "types.h"

namespace mysqlx {

namespace util {

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

std::ostream& operator<<(std::ostream& os, const string& str);

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_formatter = boost::basic_format<CharT, Traits, allocator<CharT>>;
using formatter = basic_formatter<char>;
using wformatter = basic_formatter<wchar_t>;

/*
	it is meant to work with PHP methods as wrapper for string params coming
	from zend_parse_method_parameters
	in general it keeps pointer/len of string parameter, has some helper routines, and its contents
	INVALIDATES when called from MYSQL_XDEVAPI_PHP_METHOD ends

	common scenario:

	0)
	util::string_view index_name;
	[...]
	if (FAILURE == zend_parse_method_parameters(
		ZEND_NUM_ARGS(), getThis(), "Os+",
		&object_zv, table_create_class_entry,
		&index_name.str, &index_name.len))

	1) then optionally make some checks (whether is empty or make some 	comparison
	like == ), or immediately get proper util::string via to_string() member routine
*/
/* {{{ string_view */
struct string_view
{
	string_view() = default;
	string_view(const char* s) : string_view(s, s ? std::strlen(s) : 0) {}
	string_view(const char* s, const size_t l) : str(s), len(l) {}
	string_view(const string& s) : string_view(s.c_str(), s.length()) {}
	string_view(zval* zv);

	string_view(const st_mysqlnd_string& s); // MYSQLND_STRING
	string_view(const st_mysqlnd_const_string& s); // MYSQLND_CSTRING

	bool empty() const
	{
		return (str == nullptr) || (*str == '\0');
	}

	string to_string() const
	{
		return string(str, len);
	}

	std::string to_std_string() const
	{
		return std::string(str, len);
	}

	st_mysqlnd_const_string to_nd_cstr() const;

	void to_zval(zval* dest) const;

	const char* c_str() const
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

	bool operator==(const string_view& rhs) const
	{
		return std::strcmp(str, rhs.str) == 0;
	}

	bool operator==(const char* rhs) const
	{
		return std::strcmp(str, rhs) == 0;
	}

	bool operator==(const string& rhs) const
	{
		return rhs == str;
	}

	bool operator==(const std::string& rhs) const
	{
		return rhs == str;
	}

	template<typename T>
	bool operator!=(const T& rhs) const
	{
		return !operator==(rhs);
	}

	char operator[](std::size_t index) const
	{
		assert(index < len);
		return *(str + index);
	}

	const char* begin() const
	{
		return str;
	}

	const char* end() const
	{
		return str + len;
	}

	const char* str{nullptr};
	size_t len{0};
};
/* }}} */

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHP_STRINGS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
