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
#ifndef MYSQL_XDEVAPI_UTIL_STRING_UTILS_H
#define MYSQL_XDEVAPI_UTIL_STRING_UTILS_H

#include "php_api.h"
#include "mysqlnd_api.h"
#include "strings.h"
#include <boost/algorithm/string/compare.hpp>
#include <cctype>

namespace mysqlx {

namespace util {

struct iless
{
	bool operator()(const char* lhs, const char* rhs) const
	{
		return std::lexicographical_compare(
			lhs, lhs + std::strlen(lhs),
			rhs, rhs + std::strlen(rhs),
			boost::algorithm::is_iless()
		);
	}

	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return std::lexicographical_compare(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end(),
			boost::algorithm::is_iless()
		);
	}

	bool operator()(const util::string& lhs, const util::string& rhs) const
	{
		return std::lexicographical_compare(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end(),
			boost::algorithm::is_iless()
		);
	}
};

//------------------------------------------------------------------------------

inline string to_string(const zend_string* zstr)
{
	return zstr ? string(ZSTR_VAL(zstr), ZSTR_LEN(zstr)) : string();
}

string to_string(const zval& zv);
inline string to_string(zval* zv)
{
	return to_string(*zv);
}

inline string to_string(const string& str)
{
	return str;
}

inline string to_string(const std::string& str)
{
	return string(str.c_str(), str.length());
}

template<
	typename T,
	typename = typename std::enable_if< std::is_arithmetic<T>::value >::type>
string to_string(T val)
{
	const std::string& str_val = std::to_string(val);
	return string(str_val.c_str(), str_val.length());
}

template<
	typename T,
	typename = typename std::enable_if< std::is_constructible<string, T*>::value >::type>
string to_string(T* val)
{
	return val ? string(val) : string();
}

//------------------------------------------------------------------------------

inline std::string to_std_string(const string& str)
{
	return std::string(str.data(), str.length());
}

inline std::string to_std_string(const std::string& str)
{
	return str;
}

//------------------------------------------------------------------------------

strings to_strings(zval* zvals, int count);

template<typename Pred>
strings to_strings(zval* zvals, int count, Pred pred)
{
	strings strings;
	strings.reserve(count);
	for (int i{0}; i < count; ++i) {
		strings.push_back(pred(&zvals[i]));
	}
	return strings;
}

zend_string* to_zend_string(const char* str);
zend_string* to_zend_string(const string& str);
zend_string* to_zend_string(formatter& fmt);

//------------------------------------------------------------------------------

/*
 * Escape an identifier name adding '`' everywhere needed
 */
string escape_identifier( const string& identifier );

template<size_t Length>
st_mysqlnd_string literal_to_mysqlnd_str(const char (&literal)[Length])
{
	return {const_cast<char*>(literal), Length};
}

inline st_mysqlnd_const_string to_mysqlnd_cstr(const string& str)
{
	return st_mysqlnd_const_string{ str.c_str(), str.length() };
}

inline st_mysqlnd_const_string to_mysqlnd_cstr(const string_view& str)
{
	return st_mysqlnd_const_string{ str.data(), str.length() };
}

inline bool is_empty(const st_mysqlnd_string& mysqlnd_str)
{
	return (mysqlnd_str.s == nullptr) || (mysqlnd_str.l == 0);
}

inline bool is_empty(const st_mysqlnd_const_string& mysqlnd_str)
{
	return (mysqlnd_str.s == nullptr) || (mysqlnd_str.l == 0);
}

//------------------------------------------------------------------------------

bool to_int(const string& str, int* value);
bool to_int(const std::string& str, int* value);

bool is_alnum_identifier(const std::string& ident);

//------------------------------------------------------------------------------

template<typename String>
bool contains_blank(const String& str)
{
	return std::find_if(
		str.begin(),
		str.end(),
		[](unsigned char chr){ return std::isblank(chr); }) != str.end();
}

template<typename String>
String quotation(const String& str)
{
	return "'" + str + "'";
}

template<typename String>
String quotation_if_blank(const String& str)
{
	if (str.empty() || contains_blank(str)) {
		return quotation(str);
	}
	return str;
}

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_STRING_UTILS_H
