/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_NODE_COLLECTION_INDEX_H
#define MYSQLX_NODE_COLLECTION_INDEX_H

namespace mysqlx {

namespace devapi {

void create_collection_index(
	drv::st_xmysqlnd_node_collection* collection,
	const phputils::string_view& index_name,
	const phputils::string_view& index_desc_json,
	zval* return_value);

void drop_collection_index(
	const drv::st_xmysqlnd_node_collection* collection,
	const phputils::string_view& index_name,
	zval* return_value);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_NODE_COLLECTION_INDEX_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
