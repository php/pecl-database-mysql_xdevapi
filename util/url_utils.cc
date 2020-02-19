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
#include "php_api.h"
extern "C" {
#include <ext/standard/url.h>
}
#include "url_utils.h"
#include "string_utils.h"

namespace mysqlx {

namespace util {

Url::Url(const php_url* phpurl)
	: scheme(to_string(phpurl->scheme))
	, user(to_string(phpurl->user))
	, pass(to_string(phpurl->pass))
	, host(to_string(phpurl->host))
	, port(phpurl->port)
	, query(to_string(phpurl->query))
	, fragment(to_string(phpurl->fragment))
{
	const string url_path = to_string( phpurl->path );
	if( !url_path.empty() ) {
		short prefix_offset = 0;
		if( url_path[0] == '/' ) {
			++prefix_offset;
		}
		path = url_path.substr( prefix_offset );
	}
}

bool Url::empty() const
{
	return scheme.empty()
		&& user.empty()
		&& pass.empty()
		&& host.empty()
		&& (port == 0)
		&& path.empty()
		&& query.empty()
		&& fragment.empty();
}

} // namespace util

} // namespace mysqlx
