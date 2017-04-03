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
#ifndef MYSQL_XDEVAPI_PHPUTILS_STRING_UTILS_H
#define MYSQL_XDEVAPI_PHPUTILS_STRING_UTILS_H

extern "C" {
#include <php.h>
#undef ERROR
#undef max
#undef inline
}
#include <boost/algorithm/string/compare.hpp>
#include "strings.h"

namespace mysqlx {

namespace phputils {

/* {{{ iless */
struct iless
{
	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return std::lexicographical_compare(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end(),
			boost::algorithm::is_iless()
		);
	}
};
/* }}} */

inline string to_string(const char* str) { return string(str); }
inline string to_string(const std::string& str) { return string(str.c_str(), str.length()); }

string to_string(const zval& zv);
inline string to_string(zval* zv) { return to_string(*zv); }

template<typename T>
inline string to_string(T val)
{
	static_assert(std::is_scalar<T>::value, "meant for scalar types only");
	const std::string& str_val = std::to_string(val);
	return string(str_val.c_str(), str_val.length());
}


strings to_strings(zval* zvals, int count);

/* {{{ to_strings */
template<typename Pred>
strings to_strings(zval* zvals, int count, Pred pred)
{
	strings strings;
	strings.reserve(count);
	for (int i = 0; i < count; ++i) {
		strings.push_back(pred(&zvals[i]));
	}
	return strings;
}
/* }}} */

} // namespace phputils

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHPUTILS_STRING_UTILS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
