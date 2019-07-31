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
#include "string_utils.h"
#include "exceptions.h"

namespace mysqlx {

namespace util {

/* {{{ to_string */
string to_string(const zval& zv)
{
	switch (Z_TYPE(zv)) {
		case IS_NULL:
			return "NULL";

		case IS_FALSE:
			return "FALSE";

		case IS_TRUE:
			return "TRUE";

		case IS_LONG:
			return to_string(Z_LVAL(zv));

		case IS_DOUBLE:
			return to_string(Z_DVAL(zv));

		case IS_STRING:
			return string(Z_STRVAL(zv), Z_STRLEN(zv));

		default:
			throw xdevapi_exception(xdevapi_exception::Code::unsupported_conversion_to_string);
	}
}
/* }}} */


/* {{{ string to_string */
string to_string(const MYSQLND_STRING& s)
{
	return string(s.s, s.l);
}
/* }}} */


/* {{{ string to_string */
string to_string(const MYSQLND_CSTRING& s)
{
	return string(s.s, s.l);
}
/* }}} */


/* {{{ to_strings */
strings to_strings(zval* zvals, int count)
{
	strings strings;
	strings.reserve(count);
	for (int i{0}; i < count; ++i) {
		strings.push_back(to_string(zvals[i]));
	}
	return strings;
}
/* }}} */


/* {{{ to_zend_string */
zend_string* to_zend_string(const char* str)
{
	return zend_string_init(str, strlen(str), 0);
}
/* }}} */


/* {{{ to_zend_string */
zend_string* to_zend_string(const string& str)
{
	return zend_string_init(str.c_str(), str.length(), 0);
}
/* }}} */


/* {{{ to_zend_string */
zend_string* to_zend_string(formatter& fmt)
{
	const string& str{ fmt.str() };
	return to_zend_string(str);
}
/* }}} */


/* {{{ escape_identifier */
string
escape_identifier( const string& identifier ) {
	std::stringstream result;
	result << '`';
	for( const auto& ch : identifier ) {
		if( ch == '`' ) {
			result << "``";
		} else {
			result << ch;
		}
	}
	result << '`';
	return result.str().c_str();
}
/* }}} */


/* {{{ to_int */
bool to_int(const string& str, int* value)
{
	try {
		*value = std::stoi(str.c_str());
		return true;
	} catch(std::exception& /*e*/) {
		return false;
	}
}
/* }}} */

/* {{{ to_int */
bool to_int(const std::string& str, int* value)
{
	try {
		*value = std::stoi(str);
		return true;
	} catch(std::exception& /*e*/) {
		return false;
	}
}
/* }}} */

} // namespace util

} // namespace mysqlx
