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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_COLLECTION__ADD_H
#define MYSQLX_COLLECTION__ADD_H

#include "util/value.h"

namespace mysqlx {

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
		zval* docs,
		int num_of_docs);
	bool add_docs(
		drv::xmysqlnd_collection* collection,
		const util::string_view& single_doc_id,
		zval* doc);

public:
	void execute(zval* resultset);

private:
	drv::xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__add* add_op{nullptr};
	std::vector<util::zvalue> docs;
};

void mysqlx_new_collection__add(
	zval* return_value,
	drv::xmysqlnd_collection* schema,
	zval* docs,
	int num_of_docs);
void mysqlx_register_collection__add_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_collection__add_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__ADD_H */
