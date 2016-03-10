/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#ifndef XMYSQLND_CRUD_EXPRESSION_PARSERS_H
#define XMYSQLND_CRUD_EXPRESSION_PARSERS_H

#ifdef __cplusplus
extern "C"
{
#endif
struct xmysqlnd_crud_collection__remove;

struct xmysqlnd_crud_collection__remove * xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection);
void xmysqlnd_crud_collection_remove__destroy(struct xmysqlnd_crud_collection__remove * obj);
enum_func_status xmysqlnd_crud_collection_remove__set_criteria(struct xmysqlnd_crud_collection__remove * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_collection_remove__set_limit(struct xmysqlnd_crud_collection__remove * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_remove__set_offset(struct xmysqlnd_crud_collection__remove * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_remove__bind_value(struct xmysqlnd_crud_collection__remove * obj, const MYSQLND_CSTRING name, zval * value);

#ifdef __cplusplus
}
#endif

#endif /* XMYSQLND_CRUD_EXPRESSION_PARSERS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
