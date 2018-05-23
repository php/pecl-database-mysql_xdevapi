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
#ifndef XMYSQLND_CRUD_TABLE_COMMANDS_H
#define XMYSQLND_CRUD_TABLE_COMMANDS_H

#include "xmysqlnd_crud_commands.h"

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_crud_table_op__insert XMYSQLND_CRUD_TABLE_OP__INSERT;

XMYSQLND_CRUD_TABLE_OP__INSERT * xmysqlnd_crud_table_insert__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING table, zval * columns, const int num_of_columns);
void xmysqlnd_crud_table_insert__destroy(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
enum_func_status xmysqlnd_crud_table_insert__add_row(XMYSQLND_CRUD_TABLE_OP__INSERT * obj, zval * values_zv);
enum_func_status xmysqlnd_crud_table_insert__finalize_bind(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_insert__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
zend_bool xmysqlnd_crud_table_insert__is_initialized(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);


typedef struct st_xmysqlnd_crud_table_op__delete XMYSQLND_CRUD_TABLE_OP__DELETE;

XMYSQLND_CRUD_TABLE_OP__DELETE * xmysqlnd_crud_table_delete__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING table);
void xmysqlnd_crud_table_delete__destroy(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
enum_func_status xmysqlnd_crud_table_delete__set_criteria(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_table_delete__set_limit(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_delete__set_offset(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t offset);
enum_func_status xmysqlnd_crud_table_delete__bind_value(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_table_delete__add_orderby(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const MYSQLND_CSTRING orderby);
enum_func_status xmysqlnd_crud_table_delete__finalize_bind(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_delete__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
zend_bool xmysqlnd_crud_table_delete__is_initialized(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);


typedef struct st_xmysqlnd_crud_table_op__update XMYSQLND_CRUD_TABLE_OP__UPDATE;
XMYSQLND_CRUD_TABLE_OP__UPDATE * xmysqlnd_crud_table_update__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING table);
void xmysqlnd_crud_table_update__destroy(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
enum_func_status xmysqlnd_crud_table_update__set_criteria(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_table_update__set_limit(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_update__set_offset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t offset);
enum_func_status xmysqlnd_crud_table_update__bind_value(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_table_update__add_orderby(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING orderby);

//enum_func_status xmysqlnd_crud_table_update__unset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const MYSQLND_CSTRING path);
enum_func_status xmysqlnd_crud_table_update__set(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
													  const MYSQLND_CSTRING path,
													  const zval * const value,
													  const zend_bool is_expression,
													  const zend_bool is_document);
enum_func_status xmysqlnd_crud_table_update__finalize_bind(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_update__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
zend_bool xmysqlnd_crud_table_update__is_initialized(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);


typedef struct st_xmysqlnd_crud_table_op__select XMYSQLND_CRUD_TABLE_OP__SELECT;
XMYSQLND_CRUD_TABLE_OP__SELECT * xmysqlnd_crud_table_select__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING table, zval * columns,const int num_of_columns);
void xmysqlnd_crud_table_select__destroy(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
enum_func_status xmysqlnd_crud_table_select__set_criteria(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_table_select__set_limit(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_select__set_offset(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t offset);
enum_func_status xmysqlnd_crud_table_select__bind_value(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING name, zval * value);
enum_func_status xmysqlnd_crud_table_select__add_orderby(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING orderby);
enum_func_status xmysqlnd_crud_table_select__add_grouping(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING search_field);
enum_func_status xmysqlnd_crud_table_select__set_having(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const MYSQLND_CSTRING criteria);
enum_func_status xmysqlnd_crud_table_select__set_column(XMYSQLND_CRUD_TABLE_OP__SELECT * obj,
														   const MYSQLND_CSTRING column,
														   const zend_bool is_expression,
														   const zend_bool allow_alias);
enum_func_status xmysqlnd_crud_table_select__finalize_bind(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_select__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
zend_bool xmysqlnd_crud_table_select__is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
enum_func_status xmysqlnd_crud_table_select__enable_lock_exclusive(XMYSQLND_CRUD_TABLE_OP__SELECT* obj);
enum_func_status xmysqlnd_crud_table_select__enable_lock_shared(XMYSQLND_CRUD_TABLE_OP__SELECT* obj);
enum_func_status xmysqlnd_crud_table_select_set_lock_waiting_option(XMYSQLND_CRUD_TABLE_OP__SELECT* obj, int lock_waiting_option);


typedef struct st_xmysqlnd_stmt_op__execute XMYSQLND_STMT_OP__EXECUTE;
XMYSQLND_STMT_OP__EXECUTE * xmysqlnd_stmt_execute__create(const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt);
void xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE * obj);
zend_bool xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE * obj);
enum_func_status xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE * obj, const unsigned int param_no, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE * obj, zval * value);
enum_func_status xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE * obj);

struct st_xmysqlnd_pb_message_shell xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE * obj);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_CRUD_TABLE_COMMANDS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
