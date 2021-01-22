/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
#ifndef MYSQLX_OBJECT_H
#define MYSQLX_OBJECT_H

#include "php_api.h"

namespace mysqlx {

namespace devapi {

/*
  We callocate a structure, which includes a zend_object (encloses it),
  then we return a pointer to this ZO from inside our structure. When we get a
  zend_object from Zend, we calculate back the beginning address of our
  structure 'st_mysqlx_object' by using XtOffsetOf.
*/
struct st_mysqlx_object
{
	void		*ptr;
	HashTable	*properties;
	zend_object	zo;
};

typedef struct
{
	void *ptr;		/* resource: (driver, xsession, session)   */
} MYSQLX_RESOURCE;

/**
  Does pointer arithmetic to find the address of the st_mysqlx_object from a
  zend_object which is inside the structure.
*/
st_mysqlx_object* mysqlx_fetch_object_from_zo(zend_object *obj);
#define Z_MYSQLX_P(zv) mysqlx::devapi::mysqlx_fetch_object_from_zo(Z_OBJ_P((zv)))
st_mysqlx_object* to_mysqlx_object(zend_object* object);
st_mysqlx_object* to_mysqlx_object(util::raw_zval* object);

void mysqlx_object_free_storage(zend_object * object);
HashTable * mysqlx_object_get_debug_info(
#if PHP_VERSION_ID >= 80000 // PHP 8.0 or newer
	zend_object* object,
#else
	util::raw_zval* object,
#endif
	int* is_temp);

} // namespace devapi

} // namespace mysqlx

#endif	/* MYSQLX_OBJECT_H */
