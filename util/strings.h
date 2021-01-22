/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                |
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


using std_strings = std::vector<std::string>;
using std_stringset = std::set<std::string>;

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_STRINGS_H
