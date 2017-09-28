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
#include <ext/standard/url.h>
}
#include "url_utils.h"
#include "string_utils.h"

namespace mysqlx {

namespace phputils {

/* {{{ Url::Url */
Url::Url(php_url* phpurl)
	: scheme(checked_to_string(phpurl->scheme))
	, user(checked_to_string(phpurl->user))
	, pass(checked_to_string(phpurl->pass))
	, host(checked_to_string(phpurl->host))
	, port(phpurl->port)
	, path(checked_to_string(phpurl->path))
	, query(checked_to_string(phpurl->query))
	, fragment(checked_to_string(phpurl->fragment))
{
}
/* }}} */


/* {{{ Url::empty */
bool Url::empty() const
{
	return scheme.empty()
		&& user.empty()
		&& pass.empty()
		&& host.empty()
		&& port == 0
		&& path.empty()
		&& query.empty()
		&& fragment.empty();
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
