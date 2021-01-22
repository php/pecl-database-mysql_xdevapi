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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_COLLECTION_INDEX_H
#define MYSQLX_COLLECTION_INDEX_H

namespace mysqlx {

namespace devapi {

util::zvalue create_collection_index(
	drv::xmysqlnd_collection* collection,
	const util::string_view& index_name,
	const util::string_view& index_desc_json);

util::zvalue drop_collection_index(mysqlx::drv::xmysqlnd_collection *collection,
	const util::string_view& index_name);

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_COLLECTION_INDEX_H */
