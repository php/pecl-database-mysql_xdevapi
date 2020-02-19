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
#ifndef MYSQL_XDEVAPI_PHP_UTIL_ALLOCATOR_H
#define MYSQL_XDEVAPI_PHP_UTIL_ALLOCATOR_H

#include <cstddef>
#include <memory>
#include <new>

namespace mysqlx {

namespace util {

struct alloc_tag_t {};
extern const alloc_tag_t alloc_tag;

struct permanent_tag_t {};
extern const permanent_tag_t permanent_tag;

namespace internal {

void* mem_alloc(std::size_t bytes_count);
void mem_free(void* ptr);

void* mem_permanent_alloc(std::size_t bytes_count);
void mem_permanent_free(void* ptr);

} // namespace internal

} // namespace util

} // namespace mysqlx

//------------------------------------------------------------------------------

inline void* operator new(std::size_t bytes_count, const mysqlx::util::alloc_tag_t&)
{
	return mysqlx::util::internal::mem_alloc(bytes_count);
}

inline void* operator new[](std::size_t bytes_count, const mysqlx::util::alloc_tag_t&)
{
	return mysqlx::util::internal::mem_alloc(bytes_count);
}

inline void operator delete(void* ptr, const mysqlx::util::alloc_tag_t&)
{
	mysqlx::util::internal::mem_free(ptr);
}

inline void operator delete[](void* ptr, const mysqlx::util::alloc_tag_t&)
{
	mysqlx::util::internal::mem_free(ptr);
}

//------------------------------------------------------------------------------

inline void* operator new(std::size_t bytes_count, const mysqlx::util::permanent_tag_t&)
{
	return mysqlx::util::internal::mem_permanent_alloc(bytes_count);
}

inline void* operator new[](std::size_t bytes_count, const mysqlx::util::permanent_tag_t&)
{
	return mysqlx::util::internal::mem_permanent_alloc(bytes_count);
}

inline void operator delete(void* ptr, const mysqlx::util::permanent_tag_t&)
{
	mysqlx::util::internal::mem_permanent_free(ptr);
}

inline void operator delete[](void* ptr, const mysqlx::util::permanent_tag_t&)
{
	mysqlx::util::internal::mem_permanent_free(ptr);
}

//------------------------------------------------------------------------------

namespace mysqlx {

namespace util {

template<typename T, typename allocation_tag = alloc_tag_t>
class allocator
{
	public:
		typedef T value_type;
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

		T* allocate(const size_t elem_count, const_pointer /*hint*/ = nullptr) const
		{
			if ((static_cast<size_t>(-1) / sizeof(T)) < elem_count)
			{
				throw std::bad_array_new_length();
			}

			const size_t bytes_count = elem_count * sizeof(T);
			void* ptr = ::operator new(bytes_count, allocation_tag{});
			return static_cast<T*>(ptr);
		}

		void deallocate(T* const ptr, size_t) const noexcept
		{
			::operator delete(ptr, allocation_tag{});
		}

		void destroy(T* const ptr) const noexcept
		{
			((T*)ptr)->~T();
		}
};

template<typename T>
using permanent_allocator = allocator<T, permanent_tag_t>;

//------------------------------------------------------------------------------

namespace internal {

template<typename allocation_tag>
class allocable
{
	public:
		static void* operator new(std::size_t bytes_count)
		{
			return ::operator new(bytes_count, allocation_tag());
		}

		static void* operator new[](std::size_t bytes_count)
		{
			return ::operator new(bytes_count, allocation_tag());
		}

		static void operator delete(void* ptr, size_t) noexcept
		{
			::operator delete(ptr, allocation_tag());
		}

		static void operator delete[](void* ptr, size_t) noexcept
		{
			::operator delete(ptr, allocation_tag());
		}

	protected:
		allocable() = default;
		~allocable() = default;

		allocable(const allocable& ) = default;
		allocable& operator=(const allocable& ) = default;

};

} // namespace internal

using custom_allocable = internal::allocable<alloc_tag_t>;
using permanent_allocable = internal::allocable<permanent_tag_t>;

//------------------------------------------------------------------------------

template<typename T>
struct deleter
{
	void operator()(T* t)
	{
		t->~T();
		::operator delete(t, util::alloc_tag);
	}
};

template<typename T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHP_ALLOCATOR_H
