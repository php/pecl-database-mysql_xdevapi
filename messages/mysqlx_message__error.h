/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
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
#ifndef MYSQLX_MESSAGE__ERROR_H
#define MYSQLX_MESSAGE__ERROR_H

#include "xmysqlnd/proto_gen/mysqlx.pb.h"

namespace mysqlx {

namespace devapi {

namespace msg {

/* This typically should be static, but we have coupling */
extern zend_class_entry *mysqlx_message__error_class_entry;

struct st_mysqlx_message__error
{
	Mysqlx::Error message;
	zend_bool persistent;
};

void mysqlx_new_message__error(zval * return_value, const Mysqlx::Error & message);
void mysqlx_new_message__error(zval * return_value, const char * msg, const char * sql_state, const unsigned int code);

void dump_mysqlx_error(const Mysqlx::Error & error);

#define MYSQLX_FETCH_MESSAGE__ERROR__FROM_ZVAL(_to, _from) \
{ \
	st_mysqlx_object* mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_message__error*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
}

void mysqlx_register_message__error_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_message__error_class(SHUTDOWN_FUNC_ARGS);

} // namespace msg

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_MESSAGE__ERROR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
