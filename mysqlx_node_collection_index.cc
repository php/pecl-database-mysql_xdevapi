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
#include "php_api.h"
extern "C" {
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_collection.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_index_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection_index.h"
#include "mysqlx_object.h"
#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"
#include "phputils/string_utils.h"
#include <boost/property_tree/json_parser.hpp>

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

using ptree = boost::property_tree::basic_ptree<phputils::string, phputils::string>;


class Index_definition_parser
{
public:
	Index_definition_parser(
		const phputils::string_view& index_name,
		const phputils::string_view& index_desc_json);

public:
	Index_definition run();

private:
	boost::optional<Index_definition::Type> parse_type();

	Index_definition::Fields parse_fields();
	Index_field parse_field(ptree& field_description);

	void verify_field_traits(ptree& field_description);
	void verify_field_trait(const phputils::string& field_trait_name);

	void verify_field(const Index_field& field);
	void verify_field_path(const Index_field& field);
	void verify_field_type(const Index_field& field);
	void verify_field_geojson_properties(const Index_field& field);

private:
	ptree index_desc;
	Index_definition index_def;
};

//------------------------------------------------------------------------------

Index_definition_parser::Index_definition_parser(
	const phputils::string_view& index_name,
	const phputils::string_view& index_desc_json)
	: index_def(index_name)
{
	phputils::istringstream is(index_desc_json.to_string());
	boost::property_tree::read_json(is, index_desc);
}

Index_definition Index_definition_parser::run()
{
	index_def.type = parse_type();
	index_def.fields = parse_fields();
	return index_def;
}

boost::optional<Index_definition::Type> Index_definition_parser::parse_type()
{
	auto index_type = index_desc.get_optional<phputils::string>("type");
	if (!index_type) return boost::optional<Index_definition::Type>();

	using Str_to_type = std::map<phputils::string, Index_definition::Type>;
	static const Str_to_type str_to_type = {
		{ "INDEX", Index_definition::Type::Index },
		{ "SPATIAL", Index_definition::Type::Spatial }
	};

	auto it = str_to_type.find(index_type.get());
	if (it == str_to_type.end()) {
		throw std::invalid_argument("incorrect index type");
	}

	return it->second;
}

Index_definition::Fields Index_definition_parser::parse_fields()
{
	Index_definition::Fields fields;
	for (ptree::value_type& field_desc : index_desc.get_child("fields")) {
		const Index_field& field = parse_field(field_desc.second);
		verify_field(field);
		fields.push_back(field);
	}
	return fields;
}

Index_field Index_definition_parser::parse_field(ptree& field_description)
{
	verify_field_traits(field_description);
	const Index_field field {
		field_description.get<phputils::string>("field"),
		field_description.get<phputils::string>("type"),
		field_description.get_optional<bool>("required"),
		field_description.get_optional<phputils::string>("collation"),
		field_description.get_optional<unsigned int>("options"),
		field_description.get_optional<unsigned int>("srid")
	};
	return field;
}

void Index_definition_parser::verify_field_traits(ptree& field_description)
{
	for (auto field_trait : field_description) {
		const phputils::string& field_trait_name = field_trait.first.data();
		verify_field_trait(field_trait_name);
	}
}

void Index_definition_parser::verify_field_trait(const phputils::string& field_trait_name)
{
	static const phputils::stringset allowed_traits{
		"field",
		"type",
		"required",
		"collation",
		"options",
		"srid"
	};

	if (!allowed_traits.count(field_trait_name)) {
		throw std::invalid_argument(std::string("unsupported field trait '") + phputils::to_std_string(field_trait_name) + "" );
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
	const phputils::string_view& index_name,
	const phputils::string_view& index_desc_json)
{
	Index_definition_parser idx_def_parser(index_name, index_desc_json);
	return idx_def_parser.run();
}

//------------------------------------------------------------------------------

/* {{{ collection_index_on_error */
static const enum_hnd_func_status
collection_index_on_error(
	void* context,
	XMYSQLND_NODE_SESSION* session,
	st_xmysqlnd_node_stmt* const stmt,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("collection_index_on_error");
	throw phputils::xdevapi_exception(code, phputils::string(sql_state.s, sql_state.l), phputils::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

} // anonymous namespace


/* {{{ create_collection_index */
void create_collection_index(
	drv::st_xmysqlnd_node_collection* collection,
	const phputils::string_view& index_name,
	const phputils::string_view& index_desc_json,
	zval* return_value)
{
	DBG_ENTER("create_collection_index");

	RETVAL_FALSE;

	st_xmysqlnd_node_session* session{collection->data->schema->data->session};
	const phputils::string_view schema_name{collection->data->schema->data->schema_name};
	const phputils::string_view collection_name{collection->data->collection_name};
	Index_definition index_def{parse_index_def(index_name, index_desc_json)};

	const st_xmysqlnd_node_session_on_error_bind on_error{ collection_index_on_error, nullptr };
	if (drv::collection_create_index_execute(
		session,
		schema_name,
		collection_name,
		index_def,
		on_error)) {
		RETVAL_TRUE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ drop_collection_index */
void drop_collection_index(
	const st_xmysqlnd_node_collection* collection,
	const phputils::string_view& index_name,
	zval* return_value)
{
	try {
		st_xmysqlnd_node_session* session{collection->data->schema->data->session};
		const phputils::string_view schema_name{collection->data->schema->data->schema_name};
		const phputils::string_view collection_name{collection->data->collection_name};
		const st_xmysqlnd_node_session_on_error_bind on_error{ collection_index_on_error, nullptr };
		RETVAL_BOOL(drv::collection_drop_index_execute(
			session,
			schema_name,
			collection_name,
			index_name,
			on_error));
	} catch (std::exception& e) {
		phputils::log_warning(phputils::string(e.what()));
		RETVAL_FALSE;
	}
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
