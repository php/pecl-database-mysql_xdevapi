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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_CLASS_PROPERTIES_H
#define MYSQLX_CLASS_PROPERTIES_H

#include "mysqlnd_api.h"
#include "mysqlx_object.h"
#include <string_view>

namespace mysqlx {

namespace devapi {

typedef zval * (*func_mysqlx_property_get)(const st_mysqlx_object* obj, zval *rv);
typedef int    (*func_mysqlx_property_set)(st_mysqlx_object* obj, zval *newval);

struct st_mysqlx_property_entry
{
	std::string_view property_name;
	func_mysqlx_property_get get_value;
	func_mysqlx_property_set set_value;
};

struct st_mysqlx_property
{
	zend_string *name;
	func_mysqlx_property_get get_value;
	func_mysqlx_property_set set_value;
};

#if PHP_VERSION_ID >= 70400 // PHP 7.4 or newer
using property_set_value_return_type = zval*;
#else // PHP older than 7.4
using property_set_value_return_type = void;
#endif

void mysqlx_add_properties(HashTable * ht, const st_mysqlx_property_entry* entries);

zval * mysqlx_property_get_value(zval * object, zval * member, int type, void ** cache_slot, zval * rv);
property_set_value_return_type mysqlx_property_set_value(zval * object, zval * member, zval * value, void ** cache_slot);
int mysqlx_object_has_property(zval * object, zval *member, int has_set_exists, void ** cache_slot);

void mysqlx_free_property_cb(zval *el);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_CLASS_PROPERTIES_H */
