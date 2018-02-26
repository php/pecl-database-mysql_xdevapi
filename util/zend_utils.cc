/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "zend_utils.h"

namespace mysqlx {

namespace util {

namespace zend {

namespace {

template<typename T>
void free_error_list(
	T& error_list,
	zend_bool persistent);

template<>
void free_error_list<zend_llist*>(
	zend_llist*& error_list,
	zend_bool persistent)
{
	if (error_list) {
		zend_llist_clean(error_list);
		mnd_pefree(error_list, persistent);
		error_list = nullptr;
	}
}

template<>
void free_error_list<zend_llist>(
	zend_llist& error_list,
	zend_bool persistent)
{
	zend_llist_clean(&error_list);
}

} // anonymous namespace

void free_error_info_list(
	MYSQLND_ERROR_INFO* error_info,
	zend_bool persistent)
{
	free_error_list(error_info->error_list, persistent);
}

} // namespace zend

} // namespace util

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
