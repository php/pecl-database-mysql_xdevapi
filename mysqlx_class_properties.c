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
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"


/* {{{ mysqlx_property_get_forbidden */
static zval *
mysqlx_property_get_forbidden(const struct st_mysqlx_object * not_used1, zval * not_used2)
{
	php_error_docref(NULL, E_ERROR, "Write-only property");
	return NULL;
}
/* }}} */


/* {{{ mysqlx_property_set_forbidden */
static int
mysqlx_property_set_forbidden(struct st_mysqlx_object * not_used1, zval * not_used2)
{
	php_error_docref(NULL, E_ERROR, "Read-only property");
	return FAILURE;
}
/* }}} */


/* {{{ mysqlx_add_property */
static void
mysqlx_add_property(HashTable * properties, const MYSQLND_CSTRING property_name, const func_mysqlx_property_get get, const func_mysqlx_property_set set)
{
	struct st_mysqlx_property property;

	property.name		= zend_string_init(property_name.s, property_name.l, 1);
	property.get_value	= get? get : mysqlx_property_get_forbidden;
	property.set_value	= set? set : mysqlx_property_set_forbidden;

	zend_hash_add_mem(properties, property.name, &property, sizeof(struct st_mysqlx_property));

	zend_string_release(property.name);
}
/* }}} */


/* {{{ mysqlx_add_properties */
void
mysqlx_add_properties(HashTable * ht, const struct st_mysqlx_property_entry * entries)
{
	unsigned int i;
	for (i = 0; entries[i].property_name.s != NULL; ++i) {
		mysqlx_add_property(ht, entries[i].property_name, entries[i].get_value, entries[i].set_value);
	}
}
/* }}} */


/* {{{ mysqlx_property_get_value */
zval *
mysqlx_property_get_value(zval * object, zval * member, int type, void ** cache_slot, zval * rv)
{
	zval tmp_member;
	zval *retval;
	const struct st_mysqlx_object * mysqlx_obj;
	const struct st_mysqlx_property * property = NULL;

	mysqlx_obj = Z_MYSQLX_P(object);

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	if (mysqlx_obj->properties != NULL) {
		property = zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member));
	}

	if (property) {
		retval = property->get_value(mysqlx_obj, rv);
		if (retval == NULL) {
			retval = &EG(uninitialized_zval);
		}
	} else {
		zend_object_handlers *standard_handlers = zend_get_std_object_handlers();
		retval = standard_handlers->read_property(object, member, type, cache_slot, rv);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return retval;
}
/* }}} */


/* {{{ mysqlx_property_set_value */
void
mysqlx_property_set_value(zval * object, zval * member, zval * value, void **cache_slot)
{
	zval tmp_member;
	struct st_mysqlx_object * mysqlx_obj;
	const struct st_mysqlx_property * property = NULL;

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	mysqlx_obj = Z_MYSQLX_P(object);

	if (mysqlx_obj->properties != NULL) {
		property = zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member));
	}

	if (property) {
		property->set_value(mysqlx_obj, value);
	} else {
		const zend_object_handlers * standard_handlers = zend_get_std_object_handlers();
		standard_handlers->write_property(object, member, value, cache_slot);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}
/* }}} */


/* {{{ mysqlx_object_has_property */
int
mysqlx_object_has_property(zval * object, zval * member, int has_set_exists, void **cache_slot)
{
	const struct st_mysqlx_object * mysqlx_obj = Z_MYSQLX_P(object);
	const struct st_mysqlx_property * property;
	int ret = 0;

	if ((property = zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member))) != NULL) {
		switch (has_set_exists) {
			case 0:{
				zval rv;
				zval *value = mysqlx_property_get_value(object, member, BP_VAR_IS, cache_slot, &rv);
				if (value != &EG(uninitialized_zval)) {
					ret = Z_TYPE_P(value) != IS_NULL? 1 : 0;
					zval_ptr_dtor(value);
				}
				break;
			}
			case 1: {
				zval rv;
				zval *value = mysqlx_property_get_value(object, member, BP_VAR_IS, cache_slot, &rv);
				if (value != &EG(uninitialized_zval)) {
					convert_to_boolean(value);
					ret = Z_TYPE_P(value) == IS_TRUE ? 1 : 0;
				}
				break;
			}
			case 2:
				ret = 1;
				break;
			default:
				php_error_docref(NULL, E_WARNING, "Invalid value for has_set_exists");
		}
	} else {
		const zend_object_handlers * standard_handlers = zend_get_std_object_handlers();
		ret = standard_handlers->has_property(object, member, has_set_exists, cache_slot);
	}

	return ret;
}
/* }}} */


/* {{{ mysqlx_free_property_cb */
void
mysqlx_free_property_cb(zval * el)
{
	pefree(Z_PTR_P(el), 1);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

