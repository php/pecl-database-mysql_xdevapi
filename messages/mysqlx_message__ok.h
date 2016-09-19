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
#ifndef MYSQLX_MESSAGE__OK_H
#define MYSQLX_MESSAGE__OK_H

/* This typically should be static, but we have coupling */
extern
#ifdef __cplusplus
"C"
#endif
zend_class_entry *mysqlx_message__ok_class_entry;

#ifdef  __cplusplus
#include "xmysqlnd/proto_gen/mysqlx.pb.h"
struct st_mysqlx_message__ok
{
	Mysqlx::Ok message;
	zend_bool persistent;
};

void mysqlx_new_message__ok(zval * return_value, const Mysqlx::Ok & message);
void dump_mysqlx_ok(const Mysqlx::Ok & ok);

#define MYSQLX_FETCH_MESSAGE__OK__FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_message__ok *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
}

#else
void mysqlx_register_message__ok_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_message__ok_class(SHUTDOWN_FUNC_ARGS);
#endif

#endif /* MYSQLX_MESSAGE__OK_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
