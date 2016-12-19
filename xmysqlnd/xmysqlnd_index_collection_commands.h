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
  | Authors: Darek Slusarczyk <marines@php.net>							 |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_INDEX_COLLECTION_COMMANDS_H
#define XMYSQLND_INDEX_COLLECTION_COMMANDS_H

#include "xmysqlnd_crud_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

struct st_xmysqlnd_node_session;
struct st_xmysqlnd_node_session_on_error_bind;

typedef struct st_xmysqlnd_collection_op__create_index XMYSQLND_COLLECTION_OP__CREATE_INDEX;
XMYSQLND_COLLECTION_OP__CREATE_INDEX* xmysqlnd_collection_create_index__create(const MYSQLND_CSTRING schema_name, const MYSQLND_CSTRING collection_name);
void xmysqlnd_collection_create_index__destroy(XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj);
enum_func_status xmysqlnd_collection_create_index__set_index_name(XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj, const MYSQLND_CSTRING index_name);
enum_func_status xmysqlnd_collection_create_index__set_unique(XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj, const zend_bool is_unique);
enum_func_status xmysqlnd_collection_create_index__add_field(
	XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj,
	MYSQLND_CSTRING doc_path,
	MYSQLND_CSTRING column_type,
	zend_bool is_required);

zend_bool xmysqlnd_collection_create_index__is_initialized(XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj);
enum_func_status xmysqlnd_collection_create_index__execute(struct st_xmysqlnd_node_session * const session, XMYSQLND_COLLECTION_OP__CREATE_INDEX * obj, struct st_xmysqlnd_node_session_on_error_bind on_error);


typedef struct st_xmysqlnd_collection_op__drop_index XMYSQLND_COLLECTION_OP__DROP_INDEX;
XMYSQLND_COLLECTION_OP__DROP_INDEX * xmysqlnd_collection_drop_index__create(const MYSQLND_CSTRING schema_name, const MYSQLND_CSTRING collection_name);
void xmysqlnd_collection_drop_index__destroy(XMYSQLND_COLLECTION_OP__DROP_INDEX * obj);
enum_func_status xmysqlnd_collection_drop_index__set_index_name(XMYSQLND_COLLECTION_OP__DROP_INDEX * obj, const MYSQLND_CSTRING index_name);
zend_bool xmysqlnd_collection_drop_index__is_initialized(XMYSQLND_COLLECTION_OP__DROP_INDEX * obj);
enum_func_status xmysqlnd_collection_drop_index__execute(struct st_xmysqlnd_node_session * const session, XMYSQLND_COLLECTION_OP__DROP_INDEX * obj, struct st_xmysqlnd_node_session_on_error_bind on_error);

#ifdef __cplusplus
}
#endif

#endif /* XMYSQLND_INDEX_COLLECTION_COMMANDS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
