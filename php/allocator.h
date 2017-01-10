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
#ifndef MYSQL_XDEVAPI_PHP_ALLOCATOR_H
#define MYSQL_XDEVAPI_PHP_ALLOCATOR_H

#include <memory>

namespace mysql
{

namespace php
{

struct alloc_tag_t {};
extern const alloc_tag_t alloc_tag;

namespace internal
{

void* mem_alloc(std::size_t bytes_count);
void mem_free(void* ptr);

} // namespace internal

} // namespace php

} // namespace mysql

//------------------------------------------------------------------------------

inline void* operator new(std::size_t bytes_count, const mysql::php::alloc_tag_t&)
{
	return mysql::php::internal::mem_alloc(bytes_count);
}

inline void* operator new[](std::size_t bytes_count, const mysql::php::alloc_tag_t&)
{
	return mysql::php::internal::mem_alloc(bytes_count);
}

inline void operator delete(void* ptr, const mysql::php::alloc_tag_t&)
{
	mysql::php::internal::mem_free(ptr);
}

inline void operator delete[](void* ptr, const mysql::php::alloc_tag_t&)
{
	mysql::php::internal::mem_free(ptr);
}

//------------------------------------------------------------------------------

namespace mysql
{

namespace php
{

template<typename T>
class allocator
{
	public:
		typedef T value_type;

		allocator() = default;

		template<typename U>
		allocator(const allocator<U>&) noexcept
		{
		}

		template<typename U>
		bool operator==(const allocator<U>&) const noexcept
		{
			return true;
		}

		template<typename U>
		bool operator!=(const allocator<U>&) const noexcept
		{
			return false;
		}

		T* allocate(const size_t elem_count) const
		{
			if ((static_cast<size_t>(-1) / sizeof(T)) < elem_count)
			{
				throw std::bad_array_new_length();
			}

			const size_t bytes_count = elem_count * sizeof(T);
			void* ptr = ::operator new(bytes_count, php::alloc_tag);
			return static_cast<T*>(ptr);
		}

		void deallocate(T* const ptr, size_t) const noexcept
		{
			::operator delete(ptr, php::alloc_tag);
		}


//------------------------------------------------------------------------------

	// TODO obsolete - remove all that stuff in case of newer gcc on pb
	public:
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef T& reference;
		typedef const T& const_reference;

		template<typename U>
		struct rebind 
		{ 
			using other = allocator<U>;
		};

};

//------------------------------------------------------------------------------

template<typename T>
struct deleter
{
	void operator()(T* t)
	{
		t->~T();
		::operator delete(t, php::alloc_tag);
	}
};

template<typename T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;

} // namespace php

} // namespace mysql

#endif // MYSQL_XDEVAPI_PHP_ALLOCATOR_H
