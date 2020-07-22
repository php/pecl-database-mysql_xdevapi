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
#ifndef MYSQL_XDEVAPI_UTIL_HASH_TABLE_H
#define MYSQL_XDEVAPI_UTIL_HASH_TABLE_H

#include <cstddef>
#include "strings.h"

namespace mysqlx {

namespace util {

class Hash_table
{
	public:
		Hash_table(uint32_t hint_size = 0);
		Hash_table(zval* zv, bool owner);
		~Hash_table();

		// temporarily disabled
		Hash_table(const Hash_table&) = delete;
		Hash_table& operator=(const Hash_table&) = delete;

	public:
		bool empty() const;
		std::size_t size() const;
		void clear();

		HashTable* ptr();
		zval* zv_ptr();

	public:
		zval* find(const long key);
		zval* find(const string_view& key);

	public:
		void insert(const char* key, const string_view& value);
		void insert(const char* key, std::size_t key_len, const string_view& value);
		void insert(const char* key, zval* value);

		void erase(const char* key);
		void erase(const long key);

	private:
		bool owner{false};
		HashTable* ht{nullptr};

};

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_HASH_TABLE_H
