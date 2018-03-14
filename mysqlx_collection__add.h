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
#ifndef MYSQLX_NODE_COLLECTION__ADD_H
#define MYSQLX_NODE_COLLECTION__ADD_H

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_collection;
struct st_xmysqlnd_crud_collection_op__add;

} // namespace drv

namespace devapi {

/* {{{ Collection_add */
class Collection_add : public util::custom_allocable
{
public:
	bool init(
		zval* object_zv,
		drv::st_xmysqlnd_collection* collection,
		zval* docs,
		int num_of_docs);
	bool init(
		zval* object_zv,
		drv::st_xmysqlnd_collection* collection,
		const util::string_view& single_doc_id,
		zval* doc);
	~Collection_add();

public:
	void execute(zval* return_value);

private:
	zval* object_zv{nullptr};
	drv::st_xmysqlnd_collection* collection{nullptr};
	drv::st_xmysqlnd_crud_collection_op__add* add_op{nullptr};
	zval* docs{nullptr};
	int num_of_docs{0};
};
/* }}} */


void mysqlx_new_collection__add(
	zval* return_value,
	drv::st_xmysqlnd_collection* schema,
	zval* docs,
	int num_of_docs);
void mysqlx_register_collection__add_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_node_collection__add_class(SHUTDOWN_FUNC_ARGS);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_NODE_COLLECTION__ADD_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
