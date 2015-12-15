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
#ifndef MYSQLX_MESSAGE__DATA_FETCH_DONE_H
#define MYSQLX_MESSAGE__DATA_FETCH_DONE_H

#ifdef  __cplusplus
#include "proto_gen/mysqlx_resultset.pb.h"
void mysqlx_new_data_fetch_done(zval * return_value, const Mysqlx::Resultset::FetchDone & message);
#else
void mysqlx_register_message__data_fetch_done_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_message__data_fetch_done_class(SHUTDOWN_FUNC_ARGS);
#endif

#endif /* MYSQLX_MESSAGE__DATA_FETCH_DONE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
