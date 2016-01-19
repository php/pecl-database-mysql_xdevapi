/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#ifndef MYSQLX_RESULTSET__COLUMN_METADATA_H
#define MYSQLX_RESULTSET__COLUMN_METADATA_H

#ifdef  __cplusplus
#include "proto_gen/mysqlx_resultset.pb.h"
extern zend_class_entry *mysqlx_column_metadata_class_entry;

struct st_mysqlx_column_metadata
{
	Mysqlx::Resultset::ColumnMetaData message;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__COLUMN_METADATA_FROM_ZVAL(_to, _from) \
{ \
	struct st_mysqlx_object * mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_column_metadata *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object or resource %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

void mysqlx_new_column_metadata(zval * return_value, const Mysqlx::Resultset::ColumnMetaData & message);
#else
void mysqlx_register_column_metadata_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_column_metadata_class(SHUTDOWN_FUNC_ARGS);
#endif

#endif /* MYSQLX_RESULTSET__COLUMN_METADATA_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
