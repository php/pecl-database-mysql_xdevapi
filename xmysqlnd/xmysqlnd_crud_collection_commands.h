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
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_CRUD_COLLECTION_COMMANDS_H
#define XMYSQLND_CRUD_COLLECTION_COMMANDS_H

#include <string>
#include <vector>
#include "proto_gen/mysqlx_sql.pb.h"
#include "xmysqlnd_crud_commands.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"
#include "util/types.h"
#include "util/value.h"

namespace Mysqlx { namespace Sql { class StmtExecute; } }

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_crud_collection_op__add XMYSQLND_CRUD_COLLECTION_OP__ADD;
XMYSQLND_CRUD_COLLECTION_OP__ADD  * xmysqlnd_crud_collection_add__create(const util::string_view& schema, const util::string_view& collection);
void                                xmysqlnd_crud_collection_add__destroy(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
enum_func_status                    xmysqlnd_crud_collection_add__set_upsert(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
enum_func_status                    xmysqlnd_crud_collection_add__add_doc(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj, zval * doc);
enum_func_status                    xmysqlnd_crud_collection_add__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);
struct st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_add__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__ADD * obj);

typedef struct st_xmysqlnd_crud_collection_op__remove XMYSQLND_CRUD_COLLECTION_OP__REMOVE;

XMYSQLND_CRUD_COLLECTION_OP__REMOVE * xmysqlnd_crud_collection_remove__create(const util::string_view& schema, const util::string_view& collection);
void xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
enum_func_status xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const std::string& criteria);
enum_func_status xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_remove__set_skip(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_remove__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE* obj,
	const util::string& name,
	zval* value);
enum_func_status xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const util::string_view& sort);
enum_func_status xmysqlnd_crud_collection_remove__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_remove__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);
zend_bool xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj);


struct Modify_value
{
	const util::string_view path;
	util::zvalue value;
	bool is_expression;
	bool is_document;
	bool validate_array;
};

typedef struct st_xmysqlnd_crud_collection_op__modify XMYSQLND_CRUD_COLLECTION_OP__MODIFY;
XMYSQLND_CRUD_COLLECTION_OP__MODIFY * xmysqlnd_crud_collection_modify__create(const util::string_view& schema, const util::string_view& collection);
void xmysqlnd_crud_collection_modify__destroy(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
bool xmysqlnd_crud_collection_modify__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const std::string& criteria);
bool xmysqlnd_crud_collection_modify__set_limit(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const size_t limit);
bool xmysqlnd_crud_collection_modify__set_skip(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const size_t offset);
bool xmysqlnd_crud_collection_modify__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const util::string& name,
	zval* value);
bool xmysqlnd_crud_collection_modify__add_sort(XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj, const util::string_view& sort);

bool xmysqlnd_crud_collection_modify__unset(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const util::string_view& path);
bool xmysqlnd_crud_collection_modify__set(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__replace(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__merge(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__patch(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__array_insert(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__array_append(
	XMYSQLND_CRUD_COLLECTION_OP__MODIFY* obj,
	const Modify_value& modify_value);
bool xmysqlnd_crud_collection_modify__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_modify__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);
bool xmysqlnd_crud_collection_modify__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__MODIFY * obj);


typedef struct st_xmysqlnd_crud_collection_op__find XMYSQLND_CRUD_COLLECTION_OP__FIND;
XMYSQLND_CRUD_COLLECTION_OP__FIND * xmysqlnd_crud_collection_find__create(const util::string_view& schema, const util::string_view& collection);
void xmysqlnd_crud_collection_find__destroy(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
enum_func_status xmysqlnd_crud_collection_find__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_collection_find__set_limit(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t limit);
enum_func_status xmysqlnd_crud_collection_find__set_offset(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const size_t offset);
enum_func_status xmysqlnd_crud_collection_find__bind_value(
	XMYSQLND_CRUD_COLLECTION_OP__FIND* obj,
	const util::string& name,
	zval* value);
enum_func_status xmysqlnd_crud_collection_find__add_sort(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& sort);
enum_func_status xmysqlnd_crud_collection_find__add_grouping(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& search_field);
enum_func_status xmysqlnd_crud_collection_find__set_having(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj, const util::string_view& criteria);
enum_func_status xmysqlnd_crud_collection_find__set_fields(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj,
														   const util::string_view& field,
														   const zend_bool is_expression,
														   const zend_bool allow_alias);
enum_func_status xmysqlnd_crud_collection_find__finalize_bind(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
st_xmysqlnd_pb_message_shell xmysqlnd_crud_collection_find__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
zend_bool xmysqlnd_crud_collection_find__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND * obj);
void xmysqlnd_crud_collection_find_verify_is_initialized(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj);
enum_func_status xmysqlnd_crud_collection_find__enable_lock_shared(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj);
enum_func_status xmysqlnd_crud_collection_find__enable_lock_exclusive(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj);
enum_func_status xmysqlnd_crud_collection_find_set_lock_waiting_option(XMYSQLND_CRUD_COLLECTION_OP__FIND* obj, int lock_waiting_option);


typedef struct st_xmysqlnd_stmt_op__execute XMYSQLND_STMT_OP__EXECUTE;
XMYSQLND_STMT_OP__EXECUTE* xmysqlnd_stmt_execute__create(const std::string_view& namespace_, const util::string_view& stmt);
void xmysqlnd_stmt_execute__destroy(XMYSQLND_STMT_OP__EXECUTE* obj);
Mysqlx::Sql::StmtExecute& xmysqlnd_stmt_execute__get_pb_msg(XMYSQLND_STMT_OP__EXECUTE* obj);
zend_bool xmysqlnd_stmt_execute__is_initialized(XMYSQLND_STMT_OP__EXECUTE* obj);
enum_func_status xmysqlnd_stmt_execute__bind_one_param_add(XMYSQLND_STMT_OP__EXECUTE* obj, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_one_param(XMYSQLND_STMT_OP__EXECUTE* obj, const unsigned int param_no, const zval * param_zv);
enum_func_status xmysqlnd_stmt_execute__bind_value(XMYSQLND_STMT_OP__EXECUTE* obj, zval * value);
enum_func_status xmysqlnd_stmt_execute__finalize_bind(XMYSQLND_STMT_OP__EXECUTE* obj);

st_xmysqlnd_pb_message_shell xmysqlnd_stmt_execute__get_protobuf_message(XMYSQLND_STMT_OP__EXECUTE* obj);

class Bindings
{
public:
	Bindings();
	~Bindings();

	using Bound_variable = std::pair<util::string, Mysqlx::Datatypes::Scalar*>;
	using Bound_variables = util::vector<Bound_variable>;
	using Bound_values = std::vector<Mysqlx::Datatypes::Scalar*>;

public:
	bool empty() const;
	std::size_t size() const;
	void add_placeholder(const util::string& placeholder);
	void add_placeholders(const util::std_strings& placeholders);
	util::std_strings get_placeholders() const;
	bool bind(const util::string& placeholder, zval* value);
	bool finalize(google::protobuf::RepeatedPtrField< ::Mysqlx::Datatypes::Scalar >* mutable_args);
	Bound_values get_bound_values() const;

private:
	using Bound_variables_it = Bound_variables::iterator;
	Bound_variables_it find_variable(const util::string& var_name);

private:
	Bound_variables bound_variables;
};

struct st_xmysqlnd_crud_collection_op__find
{
	Mysqlx::Crud::Find message;
	Bindings bindings;
	uint32_t ps_message_id;
	st_xmysqlnd_crud_collection_op__find(const util::string_view& schema,
										 const util::string_view& object_name) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.data(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__find() {}
};

struct st_xmysqlnd_crud_collection_op__add
{
	Mysqlx::Crud::Insert message;

	std::vector<zval> docs_zv;

	st_xmysqlnd_crud_collection_op__add(
		const util::string_view& schema,
		const util::string_view& object_name)
	{
		message.mutable_collection()->set_schema(schema.data(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	void add_document(zval* doc);
	void bind_docs();

	~st_xmysqlnd_crud_collection_op__add() {
		for( auto& values_zv : docs_zv ) {
			zval_dtor(&values_zv);
		}
		docs_zv.clear();
	}
};

struct st_xmysqlnd_crud_collection_op__modify
{
	Mysqlx::Crud::Update message;
	Bindings bindings;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_collection_op__modify(const util::string_view& schema,
										   const util::string_view& object_name) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.data(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__modify() {}
};

struct st_xmysqlnd_crud_collection_op__remove
{
	Mysqlx::Crud::Delete message;
	Bindings bindings;
	uint32_t ps_message_id;

	st_xmysqlnd_crud_collection_op__remove(const util::string_view& schema,
										   const util::string_view& object_name) :
		ps_message_id{ 0 }
	{
		message.mutable_collection()->set_schema(schema.data(), schema.length());
		message.mutable_collection()->set_name(object_name.data(), object_name.length());
		message.set_data_model(Mysqlx::Crud::DOCUMENT);
	}

	~st_xmysqlnd_crud_collection_op__remove() {}
};

struct st_xmysqlnd_stmt_op__execute
{
    zval* params{nullptr};
    unsigned int params_allocated;

    Mysqlx::Sql::StmtExecute message;
    uint32_t ps_message_id;

    st_xmysqlnd_stmt_op__execute(const util::string_view& namespace_,
                                 const util::string_view& stmt,
                                 const bool compact_meta)
        : params{nullptr},
          params_allocated{0},
          ps_message_id{0}
    {
        message.set_namespace_(namespace_.data(), namespace_.length());
        message.set_stmt(stmt.data(), stmt.length());
        message.set_compact_metadata(compact_meta);
    }

    enum_func_status bind_one_param(const zval * param_zv);
    enum_func_status bind_one_param(const unsigned int param_no, const zval * param_zv);
    enum_func_status finalize_bind();

    ~st_xmysqlnd_stmt_op__execute()
    {
        if (params) {
            for(unsigned int i{0}; i < params_allocated; ++i ) {
                zval_ptr_dtor(&params[i]);
            }
            mnd_efree(params);
        }
    }
};

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_CRUD_COLLECTION_COMMANDS_H */
