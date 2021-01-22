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
#include "php_api.h"
#include "mysqlx_database_object.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

zend_class_entry * mysqlx_database_object_interface_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__get_session, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_database_object__exists_in_database, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


static const zend_function_entry mysqlx_database_object_methods[] = {
	PHP_ABSTRACT_ME(mysqlx_database_object, getSession, arginfo_mysqlx_database_object__get_session)

	PHP_ABSTRACT_ME(mysqlx_database_object, getName, arginfo_mysqlx_database_object__get_name)

	PHP_ABSTRACT_ME(mysqlx_database_object, existsInDatabase, arginfo_mysqlx_database_object__exists_in_database)

	{nullptr, nullptr, nullptr}
};

void
mysqlx_register_database_object_interface(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* /*mysqlx_std_object_handlers*/)
{
	MYSQL_XDEVAPI_REGISTER_INTERFACE(
		mysqlx_database_object_interface_entry,
		"DatabaseObject",
		mysqlx_database_object_methods);
}

void
mysqlx_unregister_database_object_interface(UNUSED_SHUTDOWN_FUNC_ARGS)
{
}

} // namespace devapi

} // namespace mysqlx
