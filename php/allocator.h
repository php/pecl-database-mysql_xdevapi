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
  | Authors: Darek Slusarczyk <marines@php.net>							 |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQL_XDEVAPI_PHP_ALLOCATOR_H
#define MYSQL_XDEVAPI_PHP_ALLOCATOR_H

#include <memory>  

namespace mysql
{

namespace php
{

struct zend_emalloc_tag {};
extern const zend_emalloc_tag zend_emalloc;

//struct zend_malloc_tag {};
//extern const zend_malloc_tag zend_malloc;


template<typename alloc_tag>
void* zend_alloc_impl(std::size_t bytes_count);

template<typename alloc_tag>
void zend_free_impl(void* ptr);

template<typename alloc_tag>
void* zend_alloc(std::size_t bytes_count)
{
	void* ptr = zend_alloc_impl<alloc_tag>(bytes_count);
	if (ptr) {
		return ptr; 
	} else {
		throw std::bad_alloc();
	}
}

template<typename alloc_tag>
void zend_free(void* ptr)
{
	zend_free_impl<alloc_tag>(ptr);
}

//------------------------------------------------------------------------------

template <class T, class alloc_tag = zend_emalloc_tag>
struct ZendAllocator
{  
	typedef T value_type;  

	ZendAllocator() noexcept 
	{
	}
  
	template<typename U> 
	ZendAllocator(const ZendAllocator<U>&) noexcept 
	{
	}  

	template<typename U> 
	bool operator==(const ZendAllocator<U>&) const noexcept  
	{  
		return true;  
	}  

	template<typename U> 
	bool operator!=(const ZendAllocator<U>&) const noexcept  
	{  
		return false;  
	} 

	T* allocate(const size_t elem_count) const
	{  
		return static_cast<T*>(zend_alloc<alloc_tag>(elem_count * sizeof(T)));
	}

	void deallocate(T* const ptr, size_t) const noexcept
	{  
		zend_free<alloc_tag>(ptr);
	}  
};

//------------------------------------------------------------------------------

template<typename tag>
struct zend_alloc_tag_wrapper
{
	static const typename zend_emalloc_tag tag;
};

//template<typename T, class alloc_tag = zend_emalloc_tag>
template<typename T>
struct ZendDeleter
{
	void operator()(T* t) 
	{ 
		//delete (zend_alloc_tag_wrapper<alloc_tag>::tag, t);
		delete (T::tag, t);
	}
};

template<typename T>
using unique_ptr = std::unique_ptr<T, ZendDeleter<T>>;

} // namespace php

} // namespace mysql

//------------------------------------------------------------------------------

inline void* operator new(std::size_t bytes_count, const mysql::php::zend_emalloc_tag&) 
{
	return mysql::php::zend_alloc<mysql::php::zend_emalloc_tag>(bytes_count);
}

inline void* operator new[](std::size_t bytes_count, const mysql::php::zend_emalloc_tag&) 
{
	return mysql::php::zend_alloc<mysql::php::zend_emalloc_tag>(bytes_count);
}

inline void operator delete(void* ptr, const mysql::php::zend_emalloc_tag&)
{
	mysql::php::zend_free<mysql::php::zend_emalloc_tag>(ptr);
}

inline void operator delete[](void* ptr, const mysql::php::zend_emalloc_tag&)
{
	mysql::php::zend_free<mysql::php::zend_emalloc_tag>(ptr);
}

#endif
