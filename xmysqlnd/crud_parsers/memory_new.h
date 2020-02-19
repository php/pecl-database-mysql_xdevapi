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
  | Authors: Oracle Corp                                                 |
  +----------------------------------------------------------------------+
*/

#ifndef _EXPR_PARSER_MEMORY_H_
#define _EXPR_PARSER_MEMORY_H_

#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/function.hpp>
#include <boost/none.hpp>

namespace mysqlx {

namespace parser {

template <typename ArrayType>
void Memory_delete_array(ArrayType* array_ptr)
{
  delete[] array_ptr;
}

template <typename Type>
void Memory_delete(Type* ptr)
{
  delete ptr;
}


template <typename Type, typename DeleterType = boost::function<void (Type* value_ptr)> >
struct Custom_allocator
{
  typedef boost::interprocess::unique_ptr<Type, DeleterType > Unique_ptr;
};

template<typename Type>
struct Custom_allocator_with_check
{
  typedef void (*functor_type)(Type *ptr);

  Custom_allocator_with_check()
  {
    function = Memory_delete<Type>;
  }

  Custom_allocator_with_check(const boost::none_t &)
  {
    function = nullptr;
  }

  Custom_allocator_with_check(functor_type user_function)
  {
    function = user_function;
  }

  void operator() (Type *ptr)
  {
    if (function)
    {
      function(ptr);
    }
  }

  functor_type function;

  typedef boost::interprocess::unique_ptr<Type, Custom_allocator_with_check<Type> > Unique_ptr;
};

template<typename Type>
struct Memory_new
{
  struct Unary_delete
  {
    void operator() (Type *ptr)
    {
      delete ptr;
    }
  };

  typedef boost::interprocess::unique_ptr<Type, Unary_delete > Unique_ptr;
};

} // namespace parser

} // namespace mysqlx

#endif // _EXPR_PARSER_MEMORY_H_
