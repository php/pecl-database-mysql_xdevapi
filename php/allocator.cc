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
extern "C"
{
#include <php.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "allocator.h"

namespace mysql
{

namespace php
{

const zend_emalloc_tag zend_emalloc;
//const zend_malloc_tag zend_malloc;

template<>
void* zend_alloc_impl<zend_emalloc_tag>(std::size_t bytes_count)
{
	//return malloc(bytes_count);
	return mnd_emalloc(bytes_count);
}

template<>
void zend_free_impl<zend_emalloc_tag>(void* ptr)
{
	//free(ptr);
	mnd_efree(ptr);
}

//template<>
//void* zend_alloc_impl<zend_malloc_tag>(std::size_t bytes_count)
//{
//	return mnd_malloc(bytes_count);
//}
//
//template<>
//void zend_free_impl<zend_malloc_tag>(void* ptr)
//{
//	mnd_free(ptr);
//}

} // namespace php

} // namespace mysql
