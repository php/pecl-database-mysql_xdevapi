/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "arguments.h"

namespace mysqlx::util {

arg_zvals::iterator::iterator(const raw_zval* data)
	: it(data)
{
}

arg_zvals::iterator arg_zvals::iterator::operator++(int)
{
	iterator iter(it);
	++it;
	return iter;
}

arg_zvals::iterator& arg_zvals::iterator::operator++()
{
	++it;
	return *this;
}

arg_zvals::iterator::value_type arg_zvals::iterator::operator*() const
{
	return *it;
}

bool arg_zvals::iterator::operator==(const iterator& rhs) const
{
	return it == rhs.it;
}

bool arg_zvals::iterator::operator!=(const iterator& rhs) const
{
	return it != rhs.it;
}

} // namespace mysqlx::util
