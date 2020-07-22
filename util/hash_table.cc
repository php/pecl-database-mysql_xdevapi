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
#include "hash_table.h"
#include "strings.h"
#include "value.h"

/*
	useful links:
	https://wiki.php.net/phpng-upgrading#hashtable_api
	http://blog.jpauli.tech/2016/04/08/hashtables.html#hashtable-api
	http://nikic.github.io/2014/12/22/PHPs-new-hashtable-implementation.html
*/

namespace mysqlx {

namespace util {

Hash_table::Hash_table(uint32_t hint_size) : owner(true)
{
	ALLOC_HASHTABLE(ht);
	zend_hash_init(ht, hint_size, nullptr, nullptr, 0);
}

Hash_table::Hash_table(zval* zv, bool is_owner)
	: owner(is_owner)
	, ht(Z_ARRVAL_P(zv))
{
}

Hash_table::~Hash_table()
{
	if (owner) {
		zend_hash_destroy(ht);
		FREE_HASHTABLE(ht);
	}
}

// -----------------------------------------------------------------------------

bool Hash_table::empty() const
{
	return size() == 0;
}

std::size_t Hash_table::size() const
{
	return zend_array_count(ht);
}

void Hash_table::clear()
{
	zend_hash_clean(ht);
}

HashTable* Hash_table::ptr()
{
	return ht;
}

zval* Hash_table::zv_ptr()
{
	//TODO
	//return ZVAL_ARR(ht);
	return nullptr;
}

// -----------------------------------------------------------------------------

zval* Hash_table::find(const long key)
{
	return zend_hash_index_find(ht, key);
}

zval* Hash_table::find(const string_view& key)
{
	return zend_hash_str_find(ht, key.data(), key.length());
}

// -----------------------------------------------------------------------------

void Hash_table::insert(const char* key, const string_view& value)
{
	insert(key, std::strlen(key), value);
}

void Hash_table::insert(const char* key, std::size_t key_len, const string_view& value)
{
	zvalue zv(value);
	if (zend_hash_str_update(ht, key, key_len, zv.ptr())) {
		zv.invalidate();
	}
}

void Hash_table::insert(const char* key, zval* value)
{
	zend_hash_str_update(ht, key, std::strlen(key), value);
}

} // namespace util

} // namespace mysqlx
