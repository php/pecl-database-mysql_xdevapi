/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_crud_table_op__insert XMYSQLND_CRUD_TABLE_OP__INSERT;

XMYSQLND_CRUD_TABLE_OP__INSERT * xmysqlnd_crud_table_insert__create(const util::string& schema, const util::string& table, zval * columns, const int num_of_columns);
void xmysqlnd_crud_table_insert__destroy(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
enum_func_status xmysqlnd_crud_table_insert__add_row(XMYSQLND_CRUD_TABLE_OP__INSERT * obj, zval * values_zv);
enum_func_status xmysqlnd_crud_table_insert__finalize_bind(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_insert__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);
zend_bool xmysqlnd_crud_table_insert__is_initialized(XMYSQLND_CRUD_TABLE_OP__INSERT * obj);


typedef struct st_xmysqlnd_crud_table_op__delete XMYSQLND_CRUD_TABLE_OP__DELETE;

XMYSQLND_CRUD_TABLE_OP__DELETE * xmysqlnd_crud_table_delete__create(
	const util::string& schema,
	const util::string& table);
void xmysqlnd_crud_table_delete__destroy(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
enum_func_status xmysqlnd_crud_table_delete__set_criteria(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_table_delete__set_limit(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_delete__bind_value(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& name, zval * value);
enum_func_status xmysqlnd_crud_table_delete__add_orderby(XMYSQLND_CRUD_TABLE_OP__DELETE * obj, const util::string_view& orderby);
enum_func_status xmysqlnd_crud_table_delete__finalize_bind(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_delete__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);
zend_bool xmysqlnd_crud_table_delete__is_initialized(XMYSQLND_CRUD_TABLE_OP__DELETE * obj);


typedef struct st_xmysqlnd_crud_table_op__update XMYSQLND_CRUD_TABLE_OP__UPDATE;
XMYSQLND_CRUD_TABLE_OP__UPDATE * xmysqlnd_crud_table_update__create(const util::string& schema, const util::string& table);
void xmysqlnd_crud_table_update__destroy(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
enum_func_status xmysqlnd_crud_table_update__set_criteria(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_table_update__set_limit(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_update__set_offset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const size_t offset);
enum_func_status xmysqlnd_crud_table_update__bind_value(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& name, zval * value);
enum_func_status xmysqlnd_crud_table_update__add_orderby(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& orderby);

//enum_func_status xmysqlnd_crud_table_update__unset(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj, const util::string_view& path);
enum_func_status xmysqlnd_crud_table_update__set(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj,
													  const util::string_view& path,
													  const zval * const value,
													  const zend_bool is_expression,
													  const zend_bool is_document);
enum_func_status xmysqlnd_crud_table_update__finalize_bind(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_update__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);
zend_bool xmysqlnd_crud_table_update__is_initialized(XMYSQLND_CRUD_TABLE_OP__UPDATE * obj);


typedef struct st_xmysqlnd_crud_table_op__select XMYSQLND_CRUD_TABLE_OP__SELECT;
XMYSQLND_CRUD_TABLE_OP__SELECT * xmysqlnd_crud_table_select__create(const util::string& schema, const util::string& table, zval * columns,const int num_of_columns);
void xmysqlnd_crud_table_select__destroy(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
enum_func_status xmysqlnd_crud_table_select__set_criteria(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_table_select__set_limit(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t limit);
enum_func_status xmysqlnd_crud_table_select__set_offset(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const size_t offset);
enum_func_status xmysqlnd_crud_table_select__bind_value(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& name, zval * value);
enum_func_status xmysqlnd_crud_table_select__add_orderby(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& orderby);
enum_func_status xmysqlnd_crud_table_select__add_grouping(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& search_field);
enum_func_status xmysqlnd_crud_table_select__set_having(XMYSQLND_CRUD_TABLE_OP__SELECT * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_table_select__set_column(XMYSQLND_CRUD_TABLE_OP__SELECT * obj,
														   const util::string_view& column,
														   const zend_bool is_expression,
														   const zend_bool allow_alias);
enum_func_status xmysqlnd_crud_table_select__finalize_bind(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_table_select__get_protobuf_message(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
zend_bool xmysqlnd_crud_table_select__is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT * obj);
void xmysqlnd_crud_table_select_verify_is_initialized(XMYSQLND_CRUD_TABLE_OP__SELECT* obj);
enum_func_status xmysqlnd_crud_table_select__enable_lock_exclusive(XMYSQLND_CRUD_TABLE_OP__SELECT* obj);
enum_func_status xmysqlnd_crud_table_select__enable_lock_shared(XMYSQLND_CRUD_TABLE_OP__SELECT* obj);
enum_func_status xmysqlnd_crud_table_select_set_lock_waiting_option(XMYSQLND_CRUD_TABLE_OP__SELECT* obj, int lock_waiting_option);


typedef struct st_xmysqlnd_stmt_op__execute XMYSQLND_STMT_OP__EXECUTE;
XMYSQLND_STMT_OP__EXECUTE * xmysqlnd_stmt_execute__create(const std::string_view& namespace_, const util::string_view& stmt);
void xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE * obj);
zend_bool xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE * obj);
enum_func_status xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE * obj, const unsigned int param_no, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE * obj, zval * value);
enum_func_status xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE * obj);

struct st_xmysqlnd_pb_message_shell xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE * obj);

struct st_xmysqlnd_crud_table_op__insert
{
	Mysqlx::Crud::Insert message;

	std::vector<std::string> column_names;
	std::vector<zval > rows_zv;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_table_op__insert(
		const util::string& schema,
		const util::string_view& object_name,
		zval * columns_zv,
		const int num_of_columns) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.c_str(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::TABLE);

		add_columns(columns_zv,num_of_columns);
	}

	~st_xmysqlnd_crud_table_op__insert() {}

	void add_columns(zval * columns_zv, const int num_of_columns);
	void add_column(zval * column_zv);

	void add_row(zval* row_zv);

	void bind_columns();
	void bind_column(const std::string& column_name);

	void bind_rows();
	void bind_row(zval* values_zv, ::Mysqlx::Crud::Insert_TypedRow* row);
	void bind_row_field(zval* value_zv, ::Mysqlx::Crud::Insert_TypedRow* row);

};

struct st_xmysqlnd_crud_table_op__select
{
	Mysqlx::Crud::Find message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_table_op__select(
		const util::string& schema,
		const util::string_view& object_name,
		zval * columns,
		const int num_of_columns) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.c_str(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::TABLE);

		add_columns(columns,num_of_columns);
	}

	~st_xmysqlnd_crud_table_op__select() {}

	void add_columns(const zval * columns, const int num_of_columns);
};

struct st_xmysqlnd_crud_table_op__update
{
	Mysqlx::Crud::Update message;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_table_op__update(const util::string& schema,
										   const util::string_view& object_name) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.c_str(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::TABLE);
	}

	~st_xmysqlnd_crud_table_op__update() {}
};

struct st_xmysqlnd_crud_table_op__delete
{
	Mysqlx::Crud::Delete message;

	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_table_op__delete(const util::string& schema,
								const util::string_view& object_name) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.c_str(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::TABLE);
	}

	~st_xmysqlnd_crud_table_op__delete() {}
};

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_CRUD_TABLE_COMMANDS_H */
