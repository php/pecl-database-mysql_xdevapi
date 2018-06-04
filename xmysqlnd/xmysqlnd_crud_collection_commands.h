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
#ifndef XMYSQLND_CRUD_COLLECTION_COMMANDS_H
#define XMYSQLND_CRUD_COLLECTION_COMMANDS_H

#include <string>
#include "xmysqlnd_crud_commands.h"

namespace Mysqlx { namespace Sql { class StmtExecute; } }

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_crud_collection_op__add XMYSQLND_CRUD_COLLECTION_OP__ADD;
XMYSQLND_CRUD_COLLECTION_OP__ADD * xmysqlnd_crud_collection_add__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection);
void xmysqlnd_crud_collection_add__destroy(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
enum_func_status xmysqlnd_crud_collection_add__set_upsert(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
enum_func_status xmysqlnd_crud_collection_add__add_doc(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj, zval * doc);
enum_func_status xmysqlnd_crud_collection_add__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_add__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);

typedef struct st_xmysqlnd_crud_collection_op__remove XMYSQLND_CRUD_COLLECTION_OP__REMOVE;

XMYSQLND_CRUD_COLLECTION_OP__REMOVE * xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection);
void xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
enum_func_status xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const std::string& criteria);
enum_func_status xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_remove__set_skip(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_remove__bind_value(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING sort);
enum_func_status xmysqlnd_crud_collection_remove__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_remove__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
zend_bool xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);


typedef struct st_xmysqlnd_crud_collection_op__modify XMYSQLND_CRUD_COLLECTION_OP__MODIFY;
XMYSQLND_CRUD_COLLECTION_OP__MODIFY * xmysqlnd_crud_collection_modify__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection);
void xmysqlnd_crud_collection_modify__destroy(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
enum_func_status xmysqlnd_crud_collection_modify__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const std::string& criteria);
enum_func_status xmysqlnd_crud_collection_modify__set_limit(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_modify__set_skip(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_modify__bind_value(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_collection_modify__add_sort(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING sort);

enum_func_status xmysqlnd_crud_collection_modify__unset(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj, const MYSQLND_CSTRING path);
enum_func_status xmysqlnd_crud_collection_modify__set(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
													  const MYSQLND_CSTRING path,
													  const zval * const value,
													  const zend_bool is_expression,
													  const zend_bool is_document);
enum_func_status xmysqlnd_crud_collection_modify__replace(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
														  const MYSQLND_CSTRING path,
														  const zval * const value,
														  const zend_bool is_expression,
														  const zend_bool is_document);
enum_func_status xmysqlnd_crud_collection_modify__merge(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
														const MYSQLND_CSTRING path,
														const zval * const value);
enum_func_status xmysqlnd_crud_collection_modify__patch(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
                                                        const MYSQLND_CSTRING path,
                                                        const zval * const patch);
enum_func_status xmysqlnd_crud_collection_modify__array_insert(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
															   const MYSQLND_CSTRING path,
															   const zval * const value);
enum_func_status xmysqlnd_crud_collection_modify__array_append(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
															   const MYSQLND_CSTRING path,
															   const zval * const value);
enum_func_status xmysqlnd_crud_collection_modify__array_delete(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj,
															   const MYSQLND_CSTRING path);
enum_func_status xmysqlnd_crud_collection_modify__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_modify__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
zend_bool xmysqlnd_crud_collection_modify__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);


typedef struct st_xmysqlnd_crud_collection_op__find XMYSQLND_CRUD_COLLECTION_OP__FIND;
XMYSQLND_CRUD_COLLECTION_OP__FIND * xmysqlnd_crud_collection_find__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection);
void xmysqlnd_crud_collection_find__destroy(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
enum_func_status xmysqlnd_crud_collection_find__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_collection_find__set_limit(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_find__set_offset(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_find__bind_value(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_collection_find__add_sort(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING sort);
enum_func_status xmysqlnd_crud_collection_find__add_grouping(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING search_field);
enum_func_status xmysqlnd_crud_collection_find__set_having(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_collection_find__set_fields(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj,
														   const MYSQLND_CSTRING field,
														   const zend_bool is_expression,
														   const zend_bool allow_alias);
enum_func_status xmysqlnd_crud_collection_find__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_find__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
zend_bool xmysqlnd_crud_collection_find__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
enum_func_status xmysqlnd_crud_collection_find__enable_lock_shared(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj);
enum_func_status xmysqlnd_crud_collection_find__enable_lock_exclusive(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj);
enum_func_status xmysqlnd_crud_collection_find_set_lock_waiting_option(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj, int lock_waiting_option);


typedef struct st_xmysqlnd_stmt_op__execute XMYSQLND_STMT_OP__EXECUTE;
XMYSQLND_STMT_OP__EXECUTE* xmysqlnd_stmt_execute__create(const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt);
void xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE* obj);
Mysqlx::Sql::StmtExecute& xmysqlnd_stmt_execute__get_pb_msg(XMYSQLND_STMT_OP__EXECUTE* obj);
zend_bool xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE* obj);
enum_func_status xmysqlnd_stmt_execute__bind_one_param_add(XMYSQLND_STMT_OP__EXECUTE* obj, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE* obj, const unsigned int param_no, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE* obj, zval * value);
enum_func_status xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE* obj);

struct st_xmysqlnd_pb_message_shell xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE* obj);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_CRUD_COLLECTION_COMMANDS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
