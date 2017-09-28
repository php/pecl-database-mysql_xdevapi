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
#ifndef MYSQL_XDEVAPI_PHPUTILS_URL_UTILS_H
#define MYSQL_XDEVAPI_PHPUTILS_URL_UTILS_H

#include "strings.h"

extern "C" {
struct php_url;
}

namespace mysqlx {

namespace phputils {

/* {{{ Url */
struct Url
{
	Url() = default;
	Url(php_url* phpurl);

	bool empty() const;

	string scheme;
	string user;
	string pass;
	string host;
	unsigned short port = 0;
	string path;
	string query;
	string fragment;
};
/* }}} */

} // namespace phputils

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHPUTILS_URL_UTILS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
