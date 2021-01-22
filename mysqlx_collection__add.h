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
#ifndef MYSQLX_COLLECTION__ADD_H
#define MYSQLX_COLLECTION__ADD_H

#include "util/value.h"

namespace mysqlx {

namespace util {
	struct arg_zvals;
}

namespace drv {

struct xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__add;

} // namespace drv

namespace devapi {

class Collection_add : public util::custom_allocable
{
public:
	Collection_add() = default;
	Collection_add(const Collection_add&) = delete;
	Collection_add& operator=(const Collection_add&) = delete;
	~Collection_add();

	bool add_docs(
		drv::xmysqlnd_collection* collection,
		const util::arg_zvals& documents);
	bool add_docs(
		drv::xmysqlnd_collection* collection,
		const util::string_view& single_doc_id,
		const util::zvalue& doc);

public:
	util::zvalue execute();

private:
	drv::xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__add* add_op{nullptr};
	util::zvalues docs;
};

util::zvalue create_collection_add(
	drv::xmysqlnd_collection* schema,
	const util::arg_zvals& docs);
void mysqlx_register_collection__add_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_collection__add_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__ADD_H */
