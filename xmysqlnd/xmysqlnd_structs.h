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
#ifndef XMYSQLND_STRUCTS_H
#define XMYSQLND_STRUCTS_H

namespace mysqlx {

namespace drv {

struct st_collection_op_var_binder_ctx
{
	util::string_view schema_name;
	util::string_view collection_name;
	util::string_view collection_options;
};

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STRUCTS_H */
