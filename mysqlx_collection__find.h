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
#ifndef MYSQLX_COLLECTION__FIND_H
#define MYSQLX_COLLECTION__FIND_H

namespace Mysqlx { namespace Crud { class Find; } }

namespace mysqlx {

namespace util {
	class zvalue;
	struct arg_zvals;
}

namespace drv {

struct xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__find;

} // namespace drv

namespace devapi {

class Collection_find : public util::custom_allocable
{
public:
	Collection_find() = default;
	Collection_find(const Collection_find&) = delete;
	Collection_find& operator=(const Collection_find&) = delete;
	~Collection_find();

	bool init(
		drv::xmysqlnd_collection* collection,
		const util::string_view& search_expression);

public:
	bool fields(util::zvalue& fields);

	enum class Operation {
		Sort,
		Group_by
	};

	bool add_operation(
		Operation op,
		const util::arg_zvals& sort_expressions);

	bool group_by(const util::arg_zvals& sort_expressions);

	bool having(const util::string_view& search_condition);

	bool sort(const util::arg_zvals& sort_expressions);

	bool limit(zend_long rows);

	bool offset(zend_long position);

	bool bind(const util::zvalue& bind_variables);

	bool lock_shared(int lock_waiting_option);
	bool lock_exclusive(int lock_waiting_option);

	util::zvalue execute();
	util::zvalue execute(zend_long flags);

	Mysqlx::Crud::Find* get_stmt();

private:
	drv::xmysqlnd_collection*                  collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__find* find_op{nullptr};
};

extern zend_class_entry* collection_find_class_entry;

util::zvalue create_collection_find(
	const util::string_view& search_expression,
	drv::xmysqlnd_collection* collection);
void mysqlx_register_collection__find_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_collection__find_class(SHUTDOWN_FUNC_ARGS);

Mysqlx::Crud::Find* get_stmt_from_collection_find(util::raw_zval* object_zv);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__FIND_H */
