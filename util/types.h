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
#ifndef MYSQL_XDEVAPI_PHP_TYPES_H
#define MYSQL_XDEVAPI_PHP_TYPES_H

#include <deque>
#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "allocator.h"

namespace mysqlx {

namespace util {

template<typename T>
using vector = std::vector<T, allocator<T>>;

template<typename Key, typename T, typename Compare = std::less<Key>>
using map = std::map<Key, T, Compare, allocator<std::pair<const Key, T>>>;

template<typename Key, typename Compare = std::less<Key>>
using set = std::set<Key, Compare, allocator<Key>>;

template<typename Key, typename T, typename Hash = std::hash<Key>,typename KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, allocator<std::pair<const Key, T>>>;

template<typename Key, typename Hash = std::hash<Key>,typename KeyEqual = std::equal_to<Key>>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, allocator<Key>>;

template<typename T>
using deque = std::deque<T, std::allocator<T>>;

template<typename T, typename Container = deque<T>>
using stack = std::stack<T, Container>;

using byte = unsigned char;
using bytes = vector<byte>;

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_PHP_TYPES_H
