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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_session.h"
#include "xmysqlnd_schema.h"
#include "xmysqlnd_collection.h"
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd_utils.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include "xmysqlnd_index_collection_commands.h"
#include "mysqlx_object.h"
#include "mysqlx_class_properties.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/pb_utils.h"
#include "util/strings.h"
#include "util/string_utils.h"
#include "util/types.h"

#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"

#include <boost/algorithm/string/predicate.hpp>

namespace mysqlx {

namespace drv {

bool Index_field::is_geojson() const
{
	return boost::iequals(type, "GEOJSON");
}

bool Index_field::is_required() const
{
	return required ? required.value() : is_geojson();
}

// -----------------------------------------------------------------------------

Index_definition::Index_definition(const util::string_view& index_name)
	: name(index_name)
	, is_unique(false) //TODO temporary - shouldn't be needed in future version of server
{
	if (index_name.empty())	{
		throw std::invalid_argument("empty index name");
	}
}

std::optional<util::string> Index_definition::get_type_str() const
{
	using Type_to_str = std::map<Index_definition::Type, std::string>;
	static const Type_to_str type_to_str = {
		{ Index_definition::Type::Index, "INDEX" },
		{ Index_definition::Type::Spatial, "SPATIAL" }
	};

	if (type) {
		return util::to_string(type_to_str.at(type.value()));
	}
	return std::nullopt;
}

/****************************** COLLECTION.CREATE_INDEX() *******************************************************/

namespace
{

struct collection_create_index_var_binder_ctx
{
	const util::string_view schema_name;
	const util::string_view collection_name;
	const Index_definition& index_def;
};

class Bind_create_index_args
{
public:
	Bind_create_index_args(
		Mysqlx::Sql::StmtExecute& stmt_message,
		const collection_create_index_var_binder_ctx& ctx);

public:
	void run();

private:
	void bind_index_args();
	void bind_index_fields();

private:
	const collection_create_index_var_binder_ctx& ctx;
	util::pb::Object* idx_obj{nullptr};

};

Bind_create_index_args::Bind_create_index_args(
	Mysqlx::Sql::StmtExecute& stmt_message,
	const collection_create_index_var_binder_ctx& ctx)
	: ctx{ctx}
	, idx_obj{util::pb::add_object_arg(stmt_message)}
{
}

void Bind_create_index_args::run()
{
	bind_index_args();
	bind_index_fields();
}

void Bind_create_index_args::bind_index_args()
{
	const Index_definition& index_def = ctx.index_def;
	util::pb::add_field_to_object("schema", ctx.schema_name, idx_obj);
	util::pb::add_field_to_object("collection", ctx.collection_name, idx_obj);
	util::pb::add_field_to_object("name", index_def.name, idx_obj);
	util::pb::add_optional_field_to_object("type", index_def.get_type_str(), idx_obj);
	util::pb::add_optional_field_to_object("unique", index_def.is_unique, idx_obj);
}

void Bind_create_index_args::bind_index_fields()
{
	std::unique_ptr<util::pb::Array> fields{std::make_unique<util::pb::Array>()};

	for (auto field : ctx.index_def.fields) {
		std::unique_ptr<util::pb::Object> pb_obj{std::make_unique<util::pb::Object>()};

		util::pb::add_field_to_object("member", field.path, pb_obj);
		util::pb::add_field_to_object("type", field.type, pb_obj);
		util::pb::add_field_to_object("required", field.is_required(), pb_obj);
		if (field.is_geojson()) {
			util::pb::add_optional_field_to_object("options", field.options, pb_obj);
			util::pb::add_optional_field_to_object("srid", field.srid, pb_obj);
		}
		util::pb::add_optional_field_to_object("array", field.is_array, pb_obj);

		util::pb::add_value_to_array(pb_obj.release(), fields);
	}

	util::pb::add_field_to_object("constraint", fields.release(), idx_obj);
}

const enum_hnd_func_status
collection_create_index_var_binder(
	void* context,
	XMYSQLND_SESSION session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	DBG_ENTER("collection_create_index_var_binder");
	collection_create_index_var_binder_ctx* ctx
		= static_cast<collection_create_index_var_binder_ctx*>(context);

	Mysqlx::Sql::StmtExecute& stmt_message = xmysqlnd_stmt_execute__get_pb_msg(stmt_execute);

	Bind_create_index_args bind_args(stmt_message, *ctx);
	bind_args.run();

	DBG_RETURN(HND_PASS);
}

} // anonymous namespace

bool collection_create_index_execute(
	XMYSQLND_SESSION session,
	const util::string_view& schema_name,
	const util::string_view& collection_name,
	const Index_definition& index_def,
	st_xmysqlnd_session_on_error_bind on_error)
{
	DBG_ENTER("collection_create_index_execute");

	constexpr util::string_view query("create_collection_index");

	collection_create_index_var_binder_ctx var_binder_ctx{
		schema_name,
		collection_name,
		index_def
	};

	const st_xmysqlnd_session_query_bind_variable_bind var_binder{
		collection_create_index_var_binder,
		&var_binder_ctx
	};

	const enum_func_status ret = session->query_cb(
		namespace_mysqlx,
		query,
		var_binder,
		noop__on_result_start,
		noop__on_row,
		noop__on_warning,
		on_error,
		noop__on_result_end,
		noop__on_statement_ok);

	DBG_RETURN(ret == PASS);
}

/****************************** COLLECTION.DROP_INDEX() *******************************************************/

namespace
{

struct collection_drop_index_var_binder_ctx
{
	const util::string_view schema_name;
	const util::string_view collection_name;
	const util::string_view index_name;
};

const enum_hnd_func_status
collection_drop_index_var_binder(
	void* context,
	XMYSQLND_SESSION session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	DBG_ENTER("collection_drop_index_var_binder");

	collection_drop_index_var_binder_ctx* ctx
		= static_cast<collection_drop_index_var_binder_ctx*>(context);

	Mysqlx::Sql::StmtExecute& stmt_message = xmysqlnd_stmt_execute__get_pb_msg(stmt_execute);

	util::pb::Object* idx_obj{util::pb::add_object_arg(stmt_message)};

	util::pb::add_field_to_object("schema", ctx->schema_name, idx_obj);
	util::pb::add_field_to_object("collection", ctx->collection_name, idx_obj);
	util::pb::add_field_to_object("name", ctx->index_name, idx_obj);

	DBG_RETURN(HND_PASS);
}

} // anonymous namespace

bool collection_drop_index_execute(
	XMYSQLND_SESSION session,
	const util::string_view& schema_name,
	const util::string_view& collection_name,
	const util::string_view& index_name,
	st_xmysqlnd_session_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_collection_drop_index__execute");

	constexpr util::string_view query("drop_collection_index");

	collection_drop_index_var_binder_ctx var_binder_ctx{
		schema_name,
		collection_name,
		index_name
	};

	const st_xmysqlnd_session_query_bind_variable_bind var_binder{
		collection_drop_index_var_binder,
		&var_binder_ctx
	};

	const enum_func_status ret
		= session->query_cb(
			namespace_mysqlx,
			query,
			var_binder,
			noop__on_result_start,
			noop__on_row,
			noop__on_warning,
			on_error,
			noop__on_result_end,
			noop__on_statement_ok);

	DBG_RETURN(ret == PASS);
}

} // namespace drv

} // namespace mysqlx
