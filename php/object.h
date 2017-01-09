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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_PHP_OBJECT_H
#define MYSQL_XDEVAPI_PHP_OBJECT_H

#include "allocator.h"

namespace mysql
{

namespace php
{

class custom_allocable
{
	public:
		static void* operator new(std::size_t bytes_count)
		{
			return ::operator new(bytes_count, php::alloc_tag);
		}

		static void* operator new[](std::size_t bytes_count)
		{
			return ::operator new(bytes_count, php::alloc_tag);
		}

		static void operator delete(void* ptr, size_t) noexcept
		{
			::operator delete(ptr, php::alloc_tag);
		}

		static void operator delete[](void* ptr, size_t) noexcept
		{
			::operator delete(ptr, php::alloc_tag);
		}

	protected:
		custom_allocable() = default;
		~custom_allocable() = default;

		custom_allocable(const custom_allocable& ) = default;
		custom_allocable& operator=(const custom_allocable& ) = default;

};

} // namespace php

} // namespace mysql

#endif // MYSQL_XDEVAPI_PHP_OBJECT_H