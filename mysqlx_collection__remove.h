/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#ifndef MYSQLX_COLLECTION__REMOVE_H
#define MYSQLX_COLLECTION__REMOVE_H

namespace mysqlx {

namespace drv {

struct xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__remove;

} // namespace drv

namespace devapi {

/* {{{ Collection_remove */
class Collection_remove : public util::custom_allocable
{
public:
	Collection_remove() = default;
	Collection_remove(const Collection_remove& rhs) = delete;
	Collection_remove& operator=(const Collection_remove& rhs) = delete;
	~Collection_remove();

	bool init(
		zval* object_zv,
		drv::xmysqlnd_collection* collection,
		const util::string_view& search_expression);

public:
	void sort(
		zval* sort_expr,
		int num_of_expr,
		zval* return_value);
	void limit(
		zend_long rows,
		zval* return_value);
	void bind(
		HashTable* bind_variables,
		zval* return_value);
	void execute(zval* return_value);

private:
	zval* object_zv{nullptr};
	drv::xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__remove* remove_op{nullptr};

};
/* }}} */


void mysqlx_new_collection__remove(
	zval* return_value,
	const util::string_view& search_expression,
	drv::xmysqlnd_collection* collection);
void mysqlx_register_collection__remove_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_collection__remove_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION__REMOVE_H */
