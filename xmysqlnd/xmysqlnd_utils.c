/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <dariusz.slusarczyk@oracle.com>                             |
  +----------------------------------------------------------------------+
*/
#include <php.h>
#undef ERROR
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_structs.h"
#include "xmysqlnd_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int equal_mysqlnd_cstr(const MYSQLND_CSTRING* lhs, const MYSQLND_CSTRING* rhs)
{
	int result = 0;
	if (lhs->l == rhs->l)
	{
		result = !memcmp(lhs->s, rhs->s, lhs->l);
	}
	return result;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
