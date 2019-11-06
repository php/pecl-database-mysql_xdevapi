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
#include "mysqlnd_api.h"
#include "allocator.h"

namespace mysqlx {

namespace util {

const alloc_tag_t alloc_tag{};
const permanent_tag_t permanent_tag{};

namespace internal
{

void* mem_alloc(std::size_t bytes_count)
{
	void* ptr = mnd_ecalloc(1, bytes_count);
	if (ptr) {
		return ptr;
	} else {
		throw std::bad_alloc();
	}
}

void mem_free(void* ptr)
{
	mnd_efree(ptr);
}

//------------------------------------------------------------------------------

void* mem_permanent_alloc(std::size_t bytes_count)
{
	void* ptr = mnd_pecalloc(1, bytes_count, true);
	if (ptr) {
		return ptr;
	} else {
		throw std::bad_alloc();
	}
}

void mem_permanent_free(void* ptr)
{
	mnd_pefree(ptr, true);
}

} // namespace internal

} // namespace util

} // namespace mysqlx
