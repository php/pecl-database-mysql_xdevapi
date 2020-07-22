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
#ifndef XMYSQLND_WARNING_LIST_H
#define XMYSQLND_WARNING_LIST_H

#include "xmysqlnd_driver.h"
#include "xmysqlnd_wireprotocol_types.h" /* enum xmysqlnd_stmt_warning_level */

namespace mysqlx {

namespace drv {

// marines: TODO problems with custom allocator vs emplace_back && in-place initialization
struct st_xmysqlnd_warning //: public util::custom_allocable
{
	util::string message;
	unsigned int code;
	xmysqlnd_stmt_warning_level level;
};

using XMYSQLND_WARNING = st_xmysqlnd_warning;

class xmysqlnd_warning_list : public util::custom_allocable
{
public:
	xmysqlnd_warning_list();
	void add_warning(
		const xmysqlnd_stmt_warning_level level,
		const unsigned int code,
		const util::string_view& message);
	std::size_t count() const;
	XMYSQLND_WARNING get_warning(std::size_t index) const;

private:
	util::vector<st_xmysqlnd_warning> warnings;
};

using XMYSQLND_WARNING_LIST = xmysqlnd_warning_list;

XMYSQLND_WARNING_LIST* xmysqlnd_warning_list_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
void xmysqlnd_warning_list_free(XMYSQLND_WARNING_LIST* const list);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_WARNING_LIST_H */
