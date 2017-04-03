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
#ifndef MYSQLX_TABLE_CREATE_FOREIGN_KEY_DEF_H
#define MYSQLX_TABLE_CREATE_FOREIGN_KEY_DEF_H

namespace mysqlx {

namespace drv {
struct Foreign_key_def;
}

namespace devapi {

extern zend_class_entry* foreign_key_def_class_entry;

void mysqlx_register_foreign_key_def_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_foreign_key_def_class(SHUTDOWN_FUNC_ARGS);

drv::Foreign_key_def get_foreign_key_def_from_object(zval* fk_def_zv);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_TABLE_CREATE_FOREIGN_KEY_DEF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
