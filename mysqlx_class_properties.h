/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_CLASS_PROPERTIES_H
#define MYSQLX_CLASS_PROPERTIES_H

#include "mysqlx_object.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef zval * (*func_mysqlx_property_get)(const struct st_mysqlx_object *obj, zval *rv);
typedef int    (*func_mysqlx_property_set)(struct st_mysqlx_object *obj, zval *newval);

struct st_mysqlx_property_entry
{
	MYSQLND_CSTRING property_name;
	func_mysqlx_property_get get_value;
	func_mysqlx_property_set set_value;
};

struct st_mysqlx_property
{
	zend_string *name;
	func_mysqlx_property_get get_value;
	func_mysqlx_property_set set_value;
};


void mysqlx_add_properties(HashTable * ht, const struct st_mysqlx_property_entry * entries);

zval * mysqlx_property_get_value(zval * object, zval * member, int type, void ** cache_slot, zval * rv);
void mysqlx_property_set_value(zval * object, zval * member, zval * value, void ** cache_slot);
int mysqlx_object_has_property(zval * object, zval *member, int has_set_exists, void ** cache_slot);

void mysqlx_free_property_cb(zval *el);

#ifdef  __cplusplus
} /* extern "C" */
#endif

#endif /* MYSQLX_CLASS_PROPERTIES_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
