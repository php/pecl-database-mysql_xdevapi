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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {
#include <zend_exceptions.h>
}
#include "mysqlx_class_properties.h"
#include "object.h"

namespace mysqlx {

namespace util {

void safe_call_php_method(php_method_t handler, INTERNAL_FUNCTION_PARAMETERS)
{
	MYSQL_XDEVAPI_TRY {
		handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	} MYSQL_XDEVAPI_CATCH
}

void safe_call_php_function(php_function_t handler, INTERNAL_FUNCTION_PARAMETERS)
{
	MYSQL_XDEVAPI_TRY {
		handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	} MYSQL_XDEVAPI_CATCH
}

} // namespace util

} // namespace mysqlx
