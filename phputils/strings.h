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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_PHPUTILS_STRINGS_H
#define MYSQL_XDEVAPI_PHPUTILS_STRINGS_H

extern "C" {
#include <zend_types.h>
struct st_mysqlnd_string;
struct st_mysqlnd_const_string;
}
#include <string>
#include <sstream>
#include <cstring>
#include "allocator.h"
#include "types.h"

namespace mysqlx {

namespace phputils {

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string = std::basic_string<CharT, Traits, allocator<CharT>>;
using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

using strings = vector<string>;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_ostringstream = std::basic_ostringstream<CharT, Traits, allocator<CharT>>;
using ostringstream = basic_ostringstream<char>;

std::ostream& operator<<(std::ostream& os, const string& str);

struct string_ptr
{
	string_ptr() = default;
	string_ptr(const char* s) : string_ptr(str, std::strlen(s)) {}
	string_ptr(const char* s, const size_t l) : str(s), len(l) {}
	string_ptr(zval* zv);

	string_ptr(const st_mysqlnd_string& s); // MYSQLND_STRING
	string_ptr(const st_mysqlnd_const_string& s); // MYSQLND_CSTRING

	bool empty() const
	{
		return (str == nullptr) && (len == 0);
	}

	string to_string() const
	{
		return string(str, len);
	}

	const char* c_str() const
	{
		return str;
	}

	size_t length() const
	{
		return len;
	}

	bool operator==(const string_ptr& rhs) const
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
		return !(rhs == str);
	}

	const char* str = nullptr;
	size_t len = 0;
};

} // namespace phputils

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
