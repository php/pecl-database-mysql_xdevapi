/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_COLLECTION__REMOVE_H
#define MYSQLX_COLLECTION__REMOVE_H

namespace mysqlx {

namespace util {
	class zvalue;
	struct arg_zvals;
}

namespace drv {

struct xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__remove;

} // namespace drv

namespace devapi {

class Collection_remove : public util::custom_allocable
{
public:
	Collection_remove() = default;
	Collection_remove(const Collection_remove&) = delete;
	Collection_remove& operator=(const Collection_remove&) = delete;
	~Collection_remove();

	bool init(
		drv::xmysqlnd_collection* collection,
		const util::string_view& search_expression);

public:
	bool sort(const util::arg_zvals& sort_expressions);
	bool limit(zend_long rows);
	bool bind(const util::zvalue& bind_variables);
	util::zvalue execute();

private:
	drv::xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__remove* remove_op{nullptr};

};

util::zvalue create_collection_remove(
	const util::string_view& search_expression,
	drv::xmysqlnd_collection* collection);
void mysqlx_register_collection__remove_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_collection__remove_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__REMOVE_H */
