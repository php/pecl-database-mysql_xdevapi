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
#ifndef MYSQL_XDEVAPI_PHP_STRINGS_H
#define MYSQL_XDEVAPI_PHP_STRINGS_H

#include <string>
#include <sstream>
#include "allocator.h"

namespace mysqlx {

namespace phputils {

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string = std::basic_string<CharT, Traits, allocator<CharT>>;
using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_ostringstream = std::basic_ostringstream<CharT, Traits, allocator<CharT>>;
using ostringstream = basic_ostringstream<char>;

} // namespace phputils

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHP_STRINGS_H
