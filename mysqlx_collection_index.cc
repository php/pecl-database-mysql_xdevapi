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
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {
#include <zend_exceptions.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_index_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection_index.h"
#include "mysqlx_object.h"
#include "util/allocator.h"
#include "util/exceptions.h"
#include "util/object.h"
#include "util/json_utils.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

class Index_definition_parser
{
public:
	Index_definition_parser(
		const util::string_view& index_name,
		const util::string_view& index_desc_json);

public:
	Index_definition run();

private:
	std::optional<Index_definition::Type> parse_type();

	Index_definition::Fields parse_fields();
	Index_field parse_field(const util::zvalue& field_description);

	void verify_field_traits(const util::zvalue& field_description);
	void verify_field_trait(const std::string& field_trait_name);

	void verify_field(const Index_field& field);
	void verify_field_path(const Index_field& field);
	void verify_field_type(const Index_field& field);
	void verify_field_geojson_properties(const Index_field& field);

private:
	util::zvalue index_desc;
	Index_definition index_def;
};

//------------------------------------------------------------------------------

Index_definition_parser::Index_definition_parser(
	const util::string_view& index_name,
	const util::string_view& index_desc_json)
	: index_desc(util::json::parse_document(index_desc_json))
	, index_def(index_name)
{
}

Index_definition Index_definition_parser::run()
{
	if (!index_desc.is_object()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::json_object_expected,
			"index options");
	}

	index_def.type = parse_type();
	index_def.fields = parse_fields();
	return index_def;
}

std::optional<Index_definition::Type> Index_definition_parser::parse_type()
{
	auto index_type = index_desc.get_property("type");
	if (!index_type.has_value()) {
		return std::nullopt;
	}

	using Str_to_type = std::map<std::string, Index_definition::Type>;
	static const Str_to_type str_to_type = {
		{ "INDEX", Index_definition::Type::Index },
		{ "SPATIAL", Index_definition::Type::Spatial }
	};

	auto it = str_to_type.find(index_type.to_obligatory_value<std::string>());
	if (it == str_to_type.end()) {
		throw std::invalid_argument("incorrect index type");
	}

	return it->second;
}

Index_definition::Fields Index_definition_parser::parse_fields()
{
	Index_definition::Fields fields;
	util::zvalue fields_desc = index_desc.get_property("fields");
	if (!fields_desc.is_array()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::json_array_expected,
			"index fields");
	}

	for (const auto& field_desc : fields_desc) {
		const Index_field& field = parse_field(field_desc.second);
		verify_field(field);
		fields.push_back(field);
	}
	return fields;
}

Index_field Index_definition_parser::parse_field(const util::zvalue& field_description)
{
	verify_field_traits(field_description);
	Index_field field{
		field_description.require_property("field").to_obligatory_value<util::string>(),
		field_description.require_property("type").to_obligatory_value<util::string>(),
		field_description.get_property("required").to_optional_value<bool>(),
		field_description.get_property("collation").to_optional_value<util::string>(),
		field_description.get_property("options").to_optional_value<unsigned int>(),
		field_description.get_property("srid").to_optional_value<unsigned int>(),
		field_description.get_property("array").to_optional_value<bool>(),
	};
	return field;
}

void Index_definition_parser::verify_field_traits(const util::zvalue& field_description)
{
	for (const auto& field_trait : field_description.keys()) {
		const std::string field_trait_name{ field_trait.to_obligatory_value<std::string>() };
		verify_field_trait(field_trait_name);
	}
}

void Index_definition_parser::verify_field_trait(const std::string& field_trait_name)
{
	static const std::set<std::string> allowed_traits{
		"field",
		"type",
		"required",
		"collation",
		"options",
		"srid",
		"array",
	};

	if (!allowed_traits.count(field_trait_name)) {
		throw std::invalid_argument(std::string("unsupported field trait '") + field_trait_name + "" );
	}
}

void Index_definition_parser::verify_field(const Index_field& field) {
	verify_field_path(field);
	verify_field_type(field);
	verify_field_geojson_properties(field);
}

void Index_definition_parser::verify_field_path(const Index_field& field) {
	if (field.path.empty()) {
		throw std::invalid_argument("empty field path");
	}
}

void Index_definition_parser::verify_field_type(const Index_field& field) {
	if (field.type.empty()) {
		throw std::invalid_argument("empty field type");
	}
}

void Index_definition_parser::verify_field_geojson_properties(const Index_field& field) {
	if (field.is_geojson()) return;

	if (field.options || field.srid) {
		throw std::invalid_argument(
			"'options' and 'srid' properties are meant for GEOJSON type only");
	}
}

// ---------

Index_definition parse_index_def(
	const util::string_view& index_name,
	const util::string_view& index_desc_json)
{
	Index_definition_parser idx_def_parser(index_name, index_desc_json);
	return idx_def_parser.run();
}

//------------------------------------------------------------------------------

static const enum_hnd_func_status
collection_index_on_error(
	void* /*context*/,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt* const /*stmt*/,
	const unsigned int code,
	const util::string_view& sql_state,
	const util::string_view& message)
{
	throw util::xdevapi_exception(code, sql_state, message);
}

} // anonymous namespace


util::zvalue create_collection_index(
	drv::xmysqlnd_collection* collection,
	const util::string_view& index_name,
	const util::string_view& index_desc_json)
{
	DBG_ENTER("create_collection_index");

	util::zvalue result(false);

	auto session{collection->get_schema()->get_session()};
	const util::string_view schema_name{collection->get_schema()->get_name()};
	const util::string_view collection_name{collection->get_name()};
	Index_definition index_def{parse_index_def(index_name, index_desc_json)};

	const st_xmysqlnd_session_on_error_bind on_error{ collection_index_on_error, nullptr };
	if (drv::collection_create_index_execute(
		session,
		schema_name,
		collection_name,
		index_def,
		on_error)) {
		result = true;
	}

	DBG_RETURN(result);
}

util::zvalue drop_collection_index(
	xmysqlnd_collection* collection,
	const util::string_view& index_name)
{
	try {
		auto session{collection->get_schema()->get_session()};
		const util::string_view schema_name{collection->get_schema()->get_name()};
		const util::string_view collection_name{collection->get_name()};
		const st_xmysqlnd_session_on_error_bind on_error{ collection_index_on_error, nullptr };
		return drv::collection_drop_index_execute(
			session,
			schema_name,
			collection_name,
			index_name,
			on_error);
	} catch (std::exception& e) {
		util::log_warning(e.what());
		return false;
	}
}

} // namespace devapi

} // namespace mysqlx
