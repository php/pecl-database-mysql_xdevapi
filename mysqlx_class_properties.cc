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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

static zval *
mysqlx_property_get_forbidden(const st_mysqlx_object* /*not_used1*/, zval* /*not_used2*/)
{
	php_error_docref(nullptr, E_ERROR, "Write-only property");
	return nullptr;
}

static int
mysqlx_property_set_forbidden(st_mysqlx_object* /*not_used1*/, zval* /*not_used2*/)
{
	php_error_docref(nullptr, E_ERROR, "Read-only property");
	return FAILURE;
}

static void
mysqlx_add_property(HashTable * properties, const std::string_view& property_name, const func_mysqlx_property_get get, const func_mysqlx_property_set set)
{
	DBG_ENTER("mysqlx_add_property");
	DBG_INF_FMT("property=%s  getter=%p setter=%p", property_name.data(), get, set);

	st_mysqlx_property property;
	property.name		= zend_string_init(property_name.data(), property_name.length(), 1);
	property.get_value	= get? get : mysqlx_property_get_forbidden;
	property.set_value	= set? set : mysqlx_property_set_forbidden;

	zend_hash_add_mem(properties, property.name, &property, sizeof(struct st_mysqlx_property));

	zend_string_release(property.name);
	DBG_VOID_RETURN;
}

void
mysqlx_add_properties(HashTable * ht, const st_mysqlx_property_entry* entries)
{
	for (unsigned int i{0}; !entries[i].property_name.empty(); ++i) {
		mysqlx_add_property(ht, entries[i].property_name, entries[i].get_value, entries[i].set_value);
	}
}

zval *
mysqlx_property_get_value(zval * object, zval * member, int type, void ** cache_slot, zval * rv)
{
	zval tmp_member;
	zval* retval{nullptr};
	const st_mysqlx_object* mysqlx_obj{nullptr};
	const st_mysqlx_property* property{nullptr};
	DBG_ENTER("mysqlx_property_get_value");

	mysqlx_obj = Z_MYSQLX_P(object);

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	DBG_INF_FMT("property=%s", Z_STRVAL_P(member));

	if (mysqlx_obj->properties != nullptr) {
		property = static_cast<const st_mysqlx_property*>(zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member)));
	}

	if (property) {
		DBG_INF("internal property");
		retval = property->get_value(mysqlx_obj, rv);
		if (retval == nullptr) {
			DBG_INF("uninitialized_zval");
			retval = &EG(uninitialized_zval);
		}
	} else {
		const zend_object_handlers * standard_handlers = zend_get_std_object_handlers();
		DBG_INF("hash property");
		retval = standard_handlers->read_property(object, member, type, cache_slot, rv);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	DBG_RETURN(retval);
}

property_set_value_return_type
mysqlx_property_set_value(zval * object, zval * member, zval * value, void **cache_slot)
{
	zval tmp_member;
	st_mysqlx_object* mysqlx_obj{nullptr};
	const st_mysqlx_property* property{nullptr};
	DBG_ENTER("mysqlx_property_set_value");

	if (Z_TYPE_P(member) != IS_STRING) {
		ZVAL_COPY(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	DBG_INF_FMT("property=%s", Z_STRVAL_P(member));

	mysqlx_obj = Z_MYSQLX_P(object);

	if (mysqlx_obj->properties != nullptr) {
		property = static_cast<const st_mysqlx_property*>(zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member)));
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

	#if PHP_VERSION_ID >= 70400 // PHP 7.4 or newer
	DBG_RETURN(value);
	#else // PHP older than 7.4
	DBG_VOID_RETURN;
	#endif
}

int
mysqlx_object_has_property(zval * object, zval * member, int has_set_exists, void **cache_slot)
{
	const st_mysqlx_object* mysqlx_obj = Z_MYSQLX_P(object);
	const st_mysqlx_property* property{nullptr};
	int ret{0};
	DBG_ENTER("mysqlx_object_has_property");

	if ((property = static_cast<const st_mysqlx_property*>(zend_hash_find_ptr(mysqlx_obj->properties, Z_STR_P(member)))) != nullptr) {
		switch (has_set_exists) {
			case 0:{
				zval rv, *value;
				ZVAL_UNDEF(&rv);
				value = mysqlx_property_get_value(object, member, BP_VAR_IS, cache_slot, &rv);
				if (value != &EG(uninitialized_zval)) {
					ret = Z_TYPE_P(value) != IS_NULL? 1 : 0;
					zval_ptr_dtor(value);
				}
				DBG_INF("type 0");
				break;
			}
			case 1: {
				zval rv, *value;
				ZVAL_UNDEF(&rv);
				value = mysqlx_property_get_value(object, member, BP_VAR_IS, cache_slot, &rv);
				if (value != &EG(uninitialized_zval)) {
					convert_to_boolean(value);
					ret = Z_TYPE_P(value) == IS_TRUE ? 1 : 0;
				}
				DBG_INF("type 1");
				break;
			}
			case 2:
				ret = 1;
				break;
			default:
				php_error_docref(nullptr, E_WARNING, "Invalid value for has_set_exists");
		}
	} else {
		const zend_object_handlers * standard_handlers = zend_get_std_object_handlers();
		DBG_INF("hash property");
		ret = standard_handlers->has_property(object, member, has_set_exists, cache_slot);
	}

	DBG_RETURN(ret);
}

void
mysqlx_free_property_cb(zval * el)
{
	pefree(Z_PTR_P(el), 1);
}

} // namespace devapi

} // namespace mysqlx
