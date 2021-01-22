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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_COLUMN_RESULT_H
#define MYSQLX_COLUMN_RESULT_H

namespace mysqlx {

namespace drv {
struct st_xmysqlnd_result_field_meta;
}

namespace devapi {

struct st_mysqlx_column_result : public util::custom_allocable
{
	const drv::st_xmysqlnd_result_field_meta* meta;
};

util::zvalue create_column_result(const drv::st_xmysqlnd_result_field_meta* meta);
void mysqlx_register_column_result_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_column_result_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLUMN_RESULT_H */
