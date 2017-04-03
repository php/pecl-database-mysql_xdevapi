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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_TABLE_CREATE_COLUMN_DEF_BASE_H
#define MYSQLX_TABLE_CREATE_COLUMN_DEF_BASE_H

#include "xmysqlnd/xmysqlnd_ddl_table_defs.h"
#include "phputils/allocator.h"

namespace mysqlx {

namespace devapi {

extern zend_class_entry* column_def_base_class_entry;

struct Column_def_data : public phputils::custom_allocable
{
	drv::Column_def column_def;
};

void mysqlx_register_column_def_base_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_column_def_base_class(SHUTDOWN_FUNC_ARGS);

drv::Column_def get_column_def_from_object(zval* column_def_zv);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_TABLE_CREATE_COLUMN_DEF_BASE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
