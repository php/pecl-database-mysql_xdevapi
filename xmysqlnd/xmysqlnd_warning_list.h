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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_WARNING_LIST_H
#define XMYSQLND_WARNING_LIST_H

#include "xmysqlnd_driver.h"
#include "xmysqlnd_wireprotocol.h" /* enum xmysqlnd_stmt_warning_level */

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_warning
{
	MYSQLND_STRING	message;
	unsigned int	code;
	enum xmysqlnd_stmt_warning_level level;
};


typedef struct st_xmysqlnd_cwarning
{
	MYSQLND_CSTRING	message;
	unsigned int	code;
	enum xmysqlnd_stmt_warning_level level;
} XMYSQLND_WARNING;

typedef struct st_xmysqlnd_warning_list XMYSQLND_WARNING_LIST;

typedef enum_func_status		(*func_xmysqlnd_warning_list__init)(XMYSQLND_WARNING_LIST * const warn_list, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef unsigned int			(*func_xmysqlnd_warning_list__count)(const XMYSQLND_WARNING_LIST * const warn_list);
typedef const XMYSQLND_WARNING	(*func_xmysqlnd_warning_list__get_warning)(const XMYSQLND_WARNING_LIST * const warn_list, unsigned int offset);
typedef void					(*func_xmysqlnd_warning_list__add_warning)(XMYSQLND_WARNING_LIST * const warn_list, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
typedef void					(*func_xmysqlnd_warning_list__free_contents)(XMYSQLND_WARNING_LIST * const warn_list);
typedef void					(*func_xmysqlnd_warning_list__dtor)(XMYSQLND_WARNING_LIST * const warn_list);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list)
{
	func_xmysqlnd_warning_list__init init;
	func_xmysqlnd_warning_list__add_warning add_warning;
	func_xmysqlnd_warning_list__count count;
	func_xmysqlnd_warning_list__get_warning get_warning;

	func_xmysqlnd_warning_list__free_contents free_contents;
	func_xmysqlnd_warning_list__dtor dtor;
};

struct st_xmysqlnd_warning_list
{
	unsigned int warning_count;
	struct st_xmysqlnd_warning * warnings;
	unsigned int warnings_allocated;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) * m;
	zend_bool persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_warning_list);
PHP_MYSQL_XDEVAPI_API XMYSQLND_WARNING_LIST * xmysqlnd_warning_list_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_warning_list_free(XMYSQLND_WARNING_LIST * const list);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_WARNING_LIST_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
