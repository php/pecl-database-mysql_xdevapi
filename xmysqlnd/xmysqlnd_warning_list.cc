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
#include "php_api.h"
extern "C" {
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
}
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_warning_list.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_warning_list::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_warning_list, init)(XMYSQLND_WARNING_LIST * const warn_list,
											 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
											 MYSQLND_STATS * const stats,
											 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_warning_list::init");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_warning_list::add_warning */
static void
XMYSQLND_METHOD(xmysqlnd_warning_list, add_warning)(XMYSQLND_WARNING_LIST * const warn_list,
													const enum xmysqlnd_stmt_warning_level level,
													const unsigned int code,
													const MYSQLND_CSTRING message)
{
	DBG_ENTER("xmysqlnd_warning_list::add_warning");
	if (!warn_list->warnings || warn_list->warnings_allocated == warn_list->warning_count) {
		warn_list->warnings_allocated = ((warn_list->warnings_allocated + 1) * 5)/ 3;
		warn_list->warnings = static_cast<st_xmysqlnd_warning*>(mnd_perealloc(warn_list->warnings,
											warn_list->warnings_allocated * sizeof(struct st_xmysqlnd_warning),
											warn_list->persistent));
	}

	{
		const struct st_xmysqlnd_warning warn = { mnd_dup_cstring(message, warn_list->persistent), code, level };
		warn_list->warnings[warn_list->warning_count++] = warn;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_warning_list::count */
static unsigned int
XMYSQLND_METHOD(xmysqlnd_warning_list, count)(const XMYSQLND_WARNING_LIST * const warn_list)
{
	DBG_ENTER("xmysqlnd_warning_list::count");
	DBG_RETURN(warn_list->warning_count);
}
/* }}} */


/* {{{ xmysqlnd_warning_list::get_warning */
static const XMYSQLND_WARNING
XMYSQLND_METHOD(xmysqlnd_warning_list, get_warning)(const XMYSQLND_WARNING_LIST * const warn_list, unsigned int offset)
{
	XMYSQLND_WARNING ret = { {nullptr, 0}, 0, XSTMT_WARN_NONE };
	DBG_ENTER("xmysqlnd_warning_list::get_warning");
	if (offset < warn_list->warning_count) {
		ret.message = mnd_str2c(warn_list->warnings[offset].message);
		ret.code = warn_list->warnings[offset].code;
		ret.level = warn_list->warnings[offset].level;
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_warning_list::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_warning_list, free_contents)(XMYSQLND_WARNING_LIST * const warn_list)
{
	const zend_bool pers = warn_list->persistent;

	DBG_ENTER("xmysqlnd_warning_list::free_contents");
	if (warn_list->warnings) {
		if (warn_list->warning_count) {
			unsigned int i = 0;
			DBG_INF_FMT("Freeing %u warning(s)", warn_list->warning_count);
			for (;i < warn_list->warning_count; ++i) {
				mnd_pefree(warn_list->warnings[i].message.s, pers);
			}
		}
		mnd_pefree(warn_list->warnings, pers);
		warn_list->warnings = nullptr;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_warning_list::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_warning_list, dtor)(XMYSQLND_WARNING_LIST * const warn_list)
{
	DBG_ENTER("xmysqlnd_warning_list::dtor");
	if (warn_list) {
		warn_list->m->free_contents(warn_list);
		mnd_pefree(warn_list, warn_list->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_warning_list)
	XMYSQLND_METHOD(xmysqlnd_warning_list, init),
	XMYSQLND_METHOD(xmysqlnd_warning_list, add_warning),
	XMYSQLND_METHOD(xmysqlnd_warning_list, count),
	XMYSQLND_METHOD(xmysqlnd_warning_list, get_warning),
	XMYSQLND_METHOD(xmysqlnd_warning_list, free_contents),
	XMYSQLND_METHOD(xmysqlnd_warning_list, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_warning_list);

/* {{{ xmysqlnd_warning_list_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_WARNING_LIST *
xmysqlnd_warning_list_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_WARNING_LIST* result{nullptr};
	DBG_ENTER("xmysqlnd_warning_list_create");
	result = object_factory->get_warning_list(object_factory, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_warning_list_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_warning_list_free(XMYSQLND_WARNING_LIST * const warn_list)
{
	DBG_ENTER("xmysqlnd_warning_list_free");
	DBG_INF_FMT("warn_list=%p", warn_list);
	if (warn_list) {
		warn_list->m->dtor(warn_list);
	}
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
