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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_warning_list.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace drv {

xmysqlnd_warning_list::xmysqlnd_warning_list()
{
	DBG_ENTER("xmysqlnd_warning_list::xmysqlnd_warning_list");
	DBG_VOID_RETURN;
}

void
xmysqlnd_warning_list::add_warning(
	const xmysqlnd_stmt_warning_level level,
	const unsigned int code,
	const util::string_view& message)
{
	DBG_ENTER("xmysqlnd_warning_list::add_warning");
	// marines: TODO problems with custom allocator vs emplace_back && in-place initialization
	// st_xmysqlnd_warning warning = {util::to_string(message), code, level};
	// warnings.emplace_back(util::to_string(message), code, level);
	warnings.push_back({util::string(message), code, level});
	DBG_VOID_RETURN;
}

std::size_t
xmysqlnd_warning_list::count() const
{
	DBG_ENTER("xmysqlnd_warning_list::count");
	DBG_RETURN(warnings.size());
}

XMYSQLND_WARNING
xmysqlnd_warning_list::get_warning(std::size_t index) const
{
	XMYSQLND_WARNING ret;
	DBG_ENTER("xmysqlnd_warning_list::get_warning");
	if (index < warnings.size()) {
		ret = warnings[index];
	}
	DBG_RETURN(ret);
}

XMYSQLND_WARNING_LIST*
xmysqlnd_warning_list_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_WARNING_LIST* result{nullptr};
	DBG_ENTER("xmysqlnd_warning_list_create");
	result = new xmysqlnd_warning_list();
	DBG_RETURN(result);
}

void
xmysqlnd_warning_list_free(XMYSQLND_WARNING_LIST * const warn_list)
{
	DBG_ENTER("xmysqlnd_warning_list_free");
	DBG_INF_FMT("warn_list=%p", warn_list);
	delete warn_list;
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
