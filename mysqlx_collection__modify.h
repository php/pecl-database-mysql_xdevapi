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
#ifndef MYSQLX_COLLECTION__MODIFY_H
#define MYSQLX_COLLECTION__MODIFY_H

namespace mysqlx {

namespace util {
	class zvalue;
	struct arg_zvals;
}

namespace drv {

struct xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__modify;

} // namespace drv

namespace devapi {

class Collection_modify : public util::custom_allocable
{
public:
	Collection_modify() = default;
	Collection_modify(const Collection_modify&) = delete;
	Collection_modify& operator=(const Collection_modify&) = delete;
	~Collection_modify();

	bool init(
		drv::xmysqlnd_collection* collection,
		const util::string_view& search_expression);
public:
	bool sort(const util::arg_zvals& sort_expressions);
	bool limit(zend_long rows);
	bool skip(zend_long position);
	bool bind(const util::zvalue& bind_variables);

	bool set(
		const util::string_view& path,
		const util::zvalue& value);
	bool unset(const util::arg_zvals& variables);
	bool replace(
		const util::string_view& path,
		const util::zvalue& value);
	bool patch(const util::string_view& document_contents);

	bool array_insert(
		const util::string_view& path,
		const util::zvalue& value);
	bool array_append(
		const util::string_view& path,
		const util::zvalue& value);

	util::zvalue execute();

private:
	drv::Modify_value prepare_value(
		const util::string_view& path,
		util::zvalue value,
		bool validate_array = false);

private:
	drv::xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__modify* modify_op{nullptr};

};

util::zvalue create_collection_modify(
	const util::string_view& search_expression,
	drv::xmysqlnd_collection* collection);
void mysqlx_register_collection__modify_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_collection__modify_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__MODIFY_H */
