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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "strings.h"

namespace mysqlx {

namespace phputils {

/* {{{ operator<< */
std::ostream& operator<<(std::ostream& os, const string& str)
{
	return os << str.c_str();
}
/* }}} */


/* {{{ string_input_param::string_input_param */
string_input_param::string_input_param(zval* zv)
	: string_input_param(Z_STRVAL_P(zv), Z_STRLEN_P(zv))
{
	assert(Z_TYPE_P(zv) == IS_STRING);
}
/* }}} */


/* {{{ string_input_param::string_input_param */
string_input_param::string_input_param(const MYSQLND_STRING& s)
	: string_input_param(s.s, s.l)
{
}
/* }}} */


/* {{{ string_input_param::string_input_param */
string_input_param::string_input_param(const MYSQLND_CSTRING& s)
	: string_input_param(s.s, s.l)
{
}
/* }}} */

} // namespace phputils

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
