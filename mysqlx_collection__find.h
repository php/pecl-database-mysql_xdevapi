/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#ifndef MYSQLX_NODE_COLLECTION__FIND_H
#define MYSQLX_NODE_COLLECTION__FIND_H

namespace Mysqlx { namespace Crud { class Find; } }

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__find;

} // namespace drv

namespace devapi {

/* {{{ Collection_find */
class Collection_find : public util::custom_allocable
{
public:
	bool init(
		zval* object_zv,
		drv::st_xmysqlnd_collection* collection,
		const util::string_view& search_expression);
	~Collection_find();

public:
	void fields(
		const zval* fields,
		zval* return_value);

	enum class Operation {
		Sort,
		Group_by
	};

	void add_operation(
		Operation op,
		zval* sort_expr,
		int num_of_expr,
		zval* return_value);

	void group_by(
		zval* sort_expr,
		int num_of_expr,
		zval* return_value);

	void having(
		const MYSQLND_CSTRING& search_condition,
		zval* return_value);

	void sort(
		zval* sort_expr,
		int num_of_expr,
		zval* return_value);

	void limit(
		zend_long rows,
		zval* return_value);

	void skip(
		zend_long position,
		zval* return_value);

	void bind(
		HashTable* bind_variables,
		zval* return_value);

	void lock_shared(zval* return_value, int lock_waiting_option);
	void lock_exclusive(zval* return_value, int lock_waiting_option);

	void execute(zval* return_value);
	void execute(
		zend_long flags,
		zval* return_value);

	Mysqlx::Crud::Find* get_stmt();

private:
	zval* object_zv{nullptr};
	drv::st_xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__find* find_op{nullptr};

};
/* }}} */


extern zend_class_entry* collection_find_class_entry;

void mysqlx_new_collection__find(
	zval * return_value,
	const util::string_view& search_expression,
	drv::st_xmysqlnd_collection* collection);
void mysqlx_register_collection__find_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_collection__find_class(SHUTDOWN_FUNC_ARGS);

Mysqlx::Crud::Find* get_stmt_from_collection_find(zval* object_zv);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_NODE_COLLECTION__FIND_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
