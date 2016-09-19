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
#ifndef PHP_MYSQLX_H
#define PHP_MYSQLX_H

#ifdef  __cplusplus
extern "C" {
#endif

#define MYSQLX_VERSION "v1.0.0"

#define phpext_mysqlx_ptr &mysqlx_module_entry
extern zend_module_entry mysqlx_module_entry;

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(mysqlx)
	zend_bool unused;
ZEND_END_MODULE_GLOBALS(mysqlx)


PHPAPI ZEND_EXTERN_MODULE_GLOBALS(mysqlx)
#define MYSQLX_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(mysqlx, v)

#if defined(ZTS) && defined(COMPILE_DL_MYSQLX)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#ifdef __cplusplus
}
#endif

#endif	/* PHP_MYSQLX_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
