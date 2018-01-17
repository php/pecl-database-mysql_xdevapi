/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "allocator.h"

namespace mysqlx {

namespace util {

const alloc_tag_t alloc_tag{};
const permanent_tag_t permanent_tag{};

namespace internal
{

/* {{{ mysqlx::util::internal::mem_alloc */
void* mem_alloc(std::size_t bytes_count)
{
	void* ptr = mnd_ecalloc(1, bytes_count);
	if (ptr) {
		return ptr;
	} else {
		throw std::bad_alloc();
	}
}
/* }}} */

/* {{{ mysqlx::util::internal::mem_free */
void mem_free(void* ptr)
{
	mnd_efree(ptr);
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ mysqlx::util::internal::mem_permanent_alloc */
void* mem_permanent_alloc(std::size_t bytes_count)
{
	void* ptr = mnd_pecalloc(1, bytes_count, false);
	if (ptr) {
		return ptr;
	} else {
		throw std::bad_alloc();
	}
}
/* }}} */

/* {{{ mysqlx::util::internal::mem_permanent_free */
void mem_permanent_free(void* ptr)
{
	mnd_pefree(ptr, false);
}
/* }}} */

} // namespace internal

} // namespace util

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
