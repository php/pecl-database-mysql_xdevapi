/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "strings.h"

namespace mysqlx {

namespace util {

std::ostream& operator<<(std::ostream& os, const string& str)
{
	return os << str.c_str();
}
/* }}} */


string_view::string_view(zval* zv)
	: string_view(Z_STRVAL_P(zv), Z_STRLEN_P(zv))
{
	assert(Z_TYPE_P(zv) == IS_STRING);
}
/* }}} */


string_view::string_view(const MYSQLND_STRING& s)
	: string_view(s.s, s.l)
{
}
/* }}} */


string_view::string_view(const MYSQLND_CSTRING& s)
	: string_view(s.s, s.l)
{
}
/* }}} */


MYSQLND_CSTRING string_view::to_nd_cstr() const
{
	return MYSQLND_CSTRING{ str, len };
}
/* }}} */


void string_view::to_zval(zval* dest) const
{
	ZVAL_STRINGL(dest, str, len);
}
/* }}} */

} // namespace util

} // namespace mysqlx
