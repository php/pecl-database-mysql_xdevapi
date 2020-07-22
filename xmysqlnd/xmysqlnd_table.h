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
#ifndef XMYSQLND_TABLE_H
#define XMYSQLND_TABLE_H

#include "xmysqlnd_driver.h"

#include "xmysqlnd_crud_table_commands.h"
#include "util/allocator.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_schema;
struct st_xmysqlnd_session_on_error_bind;

struct xmysqlnd_table : util::custom_allocable
{
public:
	xmysqlnd_table() = default;
	xmysqlnd_table(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const cur_obj_factory,
				xmysqlnd_schema* const cur_schema,
				const util::string_view& cur_table_name,
				zend_bool is_persistent);
	enum_func_status		exists_in_database(struct st_xmysqlnd_session_on_error_bind on_error, zval* exists);
	enum_func_status		is_view(struct st_xmysqlnd_session_on_error_bind on_error, zval* exists);
	enum_func_status		count(struct st_xmysqlnd_session_on_error_bind on_error, zval* counter);
	xmysqlnd_stmt*       insert(XMYSQLND_CRUD_TABLE_OP__INSERT * op);
	xmysqlnd_stmt*       opdelete(XMYSQLND_CRUD_TABLE_OP__DELETE * op);
	xmysqlnd_stmt*		update(XMYSQLND_CRUD_TABLE_OP__UPDATE * op);
	xmysqlnd_stmt*		select(XMYSQLND_CRUD_TABLE_OP__SELECT * op);

	xmysqlnd_table *		get_reference();
	enum_func_status		free_reference(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
	void					free_contents();
	void					cleanup(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
	xmysqlnd_schema* get_schema() {
		return schema;
	}
	const util::string& get_name() const {
		return table_name;
	}
private:
	xmysqlnd_schema* schema;
	util::string table_name;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API xmysqlnd_table * xmysqlnd_table_create(xmysqlnd_schema* schema,
														const util::string_view& table_name,
														const zend_bool persistent,
														const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
														MYSQLND_STATS * const stats,
														MYSQLND_ERROR_INFO * const error_info);

PHP_MYSQL_XDEVAPI_API void xmysqlnd_table_free(xmysqlnd_table * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_TABLE_H */
