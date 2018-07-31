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
#include "util/string_utils.h"
#include <boost/version.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace
{

/*
	in older versions of boost (e.g. 1.53.0 which is at the moment still officially
	delivered as the newest one package for CentOS7) there is bug in boost::property_tree
	it doesn't support strings with custom allocator - somewhere deep in code there is
	applied std::string directly with standard allocator
	compiler fails at conversion std::string <=> util::string (custom allocator)
	in newer versions it is fixed
	the oldest version we've successfully tried is 1.59.0
	and beginning with that version we apply util::string, while for older one std::string
*/
#if 105900 <= BOOST_VERSION
using ptree_string = util::string;
#else
using ptree_string = std::string;
#endif

using ptree = boost::property_tree::basic_ptree<ptree_string, ptree_string>;

class Index_definition_parser
{
public:
	Index_definition_parser(
		const util::string_view& index_name,
		const util::string_view& index_desc_json);

public:
	Index_definition run();

private:
	boost::optional<Index_definition::Type> parse_type();

	Index_definition::Fields parse_fields();
	Index_field parse_field(ptree& field_description);

	void verify_field_traits(ptree& field_description);
	void verify_field_trait(const std::string& field_trait_name);

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
	const util::string_view& index_name,
	const util::string_view& index_desc_json)
	: index_def(index_name)
{
	util::istringstream is(index_desc_json.to_string());
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
	auto index_type = index_desc.get_optional<ptree_string>("type");
	if (!index_type) return boost::optional<Index_definition::Type>();

	using Str_to_type = std::map<std::string, Index_definition::Type>;
	static const Str_to_type str_to_type = {
		{ "INDEX", Index_definition::Type::Index },
		{ "SPATIAL", Index_definition::Type::Spatial }
	};

	auto it = str_to_type.find(util::to_std_string(index_type.get()));
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
	auto collation{ field_description.get_optional<ptree_string>("collation") };
	const Index_field field {
		util::to_string(field_description.get<ptree_string>("field")),
		util::to_string(field_description.get<ptree_string>("type")),
		field_description.get_optional<bool>("required"),
		collation ? util::to_string(collation.get()) : boost::optional<util::string>(),
		field_description.get_optional<unsigned int>("options"),
		field_description.get_optional<unsigned int>("srid"),
		field_description.get_optional<bool>("array"),
	};
	return field;
}

void Index_definition_parser::verify_field_traits(ptree& field_description)
{
	for (auto field_trait : field_description) {
		const std::string field_trait_name{ field_trait.first.data() };
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

/* {{{ collection_index_on_error */
static const enum_hnd_func_status
collection_index_on_error(
	void* /*context*/,
	XMYSQLND_SESSION session,
	xmysqlnd_stmt* const /*stmt*/,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	throw util::xdevapi_exception(code, util::string(sql_state.s, sql_state.l), util::string(message.s, message.l));
}
/* }}} */

} // anonymous namespace


/* {{{ create_collection_index */
void create_collection_index(
	drv::xmysqlnd_collection* collection,
	const util::string_view& index_name,
	const util::string_view& index_desc_json,
	zval* return_value)
{
	DBG_ENTER("create_collection_index");

	RETVAL_FALSE;

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
		RETVAL_TRUE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ drop_collection_index */
void drop_collection_index(
	xmysqlnd_collection* collection,
	const util::string_view& index_name,
	zval* return_value)
{
	try {
		auto session{collection->get_schema()->get_session()};
		const util::string_view schema_name{collection->get_schema()->get_name()};
		const util::string_view collection_name{collection->get_name()};
		const st_xmysqlnd_session_on_error_bind on_error{ collection_index_on_error, nullptr };
		RETVAL_BOOL(drv::collection_drop_index_execute(
			session,
			schema_name,
			collection_name,
			index_name,
			on_error));
	} catch (std::exception& e) {
		util::log_warning(e.what());
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
