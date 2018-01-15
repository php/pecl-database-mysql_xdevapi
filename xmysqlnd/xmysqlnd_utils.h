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
#ifndef XMYSQLND_UTILS_H
#define XMYSQLND_UTILS_H

#include "util/strings.h"

namespace mysqlx {

namespace drv {

template<typename String>
bool is_empty_str(const String& mystr)
{
	return (mystr.s == nullptr) || (mystr.l == 0);
}

MYSQLND_CSTRING make_mysqlnd_cstr(const char * str);
MYSQLND_STRING make_mysqlnd_str(const char * str);
bool equal_mysqlnd_cstr(const MYSQLND_CSTRING& lhs, const MYSQLND_CSTRING& rhs);

void xmysqlnd_utils_decode_doc_row(zval* src, zval* dest);
void xmysqlnd_utils_decode_doc_rows(zval* src, zval* dest);

//https://en.wikipedia.org/wiki/Percent-encoding
util::string decode_pct_path(const util::string& encoded_path);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_UTILS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
