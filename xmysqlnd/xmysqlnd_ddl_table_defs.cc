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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include <cctype>

#include <boost/algorithm/string/replace.hpp>

#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_ddl_table_defs.h"

#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/string_utils.h"
#include "phputils/strings.h"
#include "phputils/types.h"

namespace mysqlx {

namespace drv {

namespace {

/* {{{ to_column_type */
Column_def::Type to_column_type(const phputils::string_ptr& column_type_str)
{
	static const std::map<std::string, Column_def::Type, phputils::iless> str_to_column_type = {
		{ "Bit", Column_def::Type::bit },
		{ "TinyInt", Column_def::Type::tiny_int },
		{ "SmallInt", Column_def::Type::small_int },
		{ "MediumInt", Column_def::Type::medium_int },
		{ "Int", Column_def::Type::integer },
		{ "Integer", Column_def::Type::integer },
		{ "BigInt", Column_def::Type::big_int },
		{ "Real", Column_def::Type::real },
		{ "Double", Column_def::Type::real_double },
		{ "Float", Column_def::Type::real_float },
		{ "Decimal", Column_def::Type::decimal },
		{ "Numeric", Column_def::Type::numeric },
		{ "Date", Column_def::Type::date },
		{ "Time", Column_def::Type::time },
		{ "Timestamp", Column_def::Type::timestamp },
		{ "Datetime", Column_def::Type::datetime },
		{ "Year", Column_def::Type::year },
		{ "Char", Column_def::Type::character },
		{ "VarChar", Column_def::Type::var_char },
		{ "String", Column_def::Type::var_char },
		{ "Binary", Column_def::Type::binary },
		{ "VarBinary", Column_def::Type::var_binary },
		{ "TinyBlob", Column_def::Type::tiny_blob },
		{ "Blob", Column_def::Type::blob },
		{ "MediumBlob", Column_def::Type::medium_blob },
		{ "LongBlob", Column_def::Type::long_blob },
		{ "TinyText", Column_def::Type::tiny_text },
		{ "Text", Column_def::Type::text },
		{ "MediumText", Column_def::Type::medium_text },
		{ "LongText", Column_def::Type::long_text },
		{ "Enum", Column_def::Type::enumeration },
		{ "Set", Column_def::Type::set },
		{ "Json", Column_def::Type::json },
	};

	auto it = str_to_column_type.find(column_type_str.c_str());
	if (it == str_to_column_type.cend()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::unknown_table_column_type);
	}

	return it->second;
}
/* }}} */


/* {{{ to_fkey_change_mode */
Foreign_key_def::Change_mode to_fkey_change_mode(const phputils::string_ptr& change_mode_str)
{
	static const std::map<std::string, Foreign_key_def::Change_mode, phputils::iless> str_to_change_mode = {
		{ "Restrict", Foreign_key_def::Change_mode::restricted },
		{ "Cascade", Foreign_key_def::Change_mode::cascade },
		{ "Set Null", Foreign_key_def::Change_mode::set_null },
		{ "No Action", Foreign_key_def::Change_mode::no_action }
	};

	auto it = str_to_change_mode.find(change_mode_str.c_str());
	if (it == str_to_change_mode.cend()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::unknown_fkey_change_mode);
	}

	return it->second;
}
/* }}} */


/* {{{ escape */
phputils::string escape(phputils::string value)
{
	// https://dev.mysql.com/doc/refman/5.7/en/string-literals.html
	static const std::map<std::string, std::string> char_to_escape_sequence = {
		{ "\b", "\\b" }, // A backspace character
		{ "\n", "\\n" }, // A newline (linefeed) character
		{ "\r", "\\r" }, // A carriage return character
		{ "\t", "\\t" }, // A tab character
		{ "\\", "\\\\" }, // A backslash (\) character
	};

	for (auto& chr_to_escape : char_to_escape_sequence) {
		boost::replace_all(value, chr_to_escape.first, chr_to_escape.second);
	}

	return value;
}
/* }}} */


/* {{{ quote */
phputils::string quote(phputils::string text, bool always_quote)
{
	// https://dev.mysql.com/doc/refman/5.7/en/string-literals.html
	const bool single_quotation = text.find('\'') != phputils::string::npos;
	const bool double_quotation = text.find('"') != phputils::string::npos;

	if (single_quotation && !double_quotation) {
		return '"' + text + '"';
	}

	if (!single_quotation && double_quotation) {
		return '\'' + text + '\'';
	}

	if (single_quotation && double_quotation) {
		boost::replace_all(text, "'", "''");
		return '\'' + text + '\'';
	}

	assert(!single_quotation && !double_quotation);
	if (always_quote) {
		return '\'' + text + '\'';
	}

	return text;
}
/* }}} */

} // anonymous namespace {

//------------------------------------------------------------------------------

namespace create_table {

/* {{{ quote_identifier */
phputils::string quote_identifier(phputils::string raw_id)
{
	return quote(raw_id, false);
}
/* }}} */


/* {{{ quote_text */
phputils::string quote_text(phputils::string raw_text)
{
	return quote(escape(raw_text), true);
}
/* }}} */

} // namespace create_table

//------------------------------------------------------------------------------

/* {{{ Column_def::init */
void Column_def::init(
	const phputils::string_ptr& n,
	const phputils::string_ptr& t,
	long len)
{
	kind = Kind::common;
	name = create_table::quote_identifier(n.to_string());
	type = to_column_type(t);
	if (len != Default_length) {
		length = {true, len};
	}
}
/* }}} */


/* {{{ Column_def::init */
void Column_def::init(
	const phputils::string_ptr& n,
	const phputils::string_ptr& t,
	const phputils::string_ptr& expression)
{
	kind = Kind::generated;
	name = create_table::quote_identifier(n.to_string());
	type = to_column_type(t);
	generated_as_expr = expression.to_string();
}
/* }}} */


/* {{{ Column_def::enable_not_null */
void Column_def::enable_not_null()
{
	not_null = true;
}
/* }}} */


/* {{{ Column_def::enable_unique_index */
void Column_def::enable_unique_index()
{
	unique_index = true;
}
/* }}} */


/* {{{ Column_def::enable_primary_key */
void Column_def::enable_primary_key()
{
	primary_key = true;
	not_null = true;
}
/* }}} */


/* {{{ Column_def::set_comment */
void Column_def::set_comment(const phputils::string_ptr& commt)
{
	comment = create_table::quote_text(commt.to_string());
}
/* }}} */


/* {{{ Column_def::set_default_value */
void Column_def::set_default_value(phputils::string default_value_expr)
{
	default_value = std::move(default_value_expr);
}
/* }}} */


/* {{{ Column_def::enable_auto_increment */
void Column_def::enable_auto_increment()
{
	auto_increment = true;
}
/* }}} */


/* {{{ Column_def::set_foreign_key */
void Column_def::set_foreign_key(const phputils::string_ptr& table, phputils::strings fields)
{
	foreign_key = Foreign_key{table.to_string(), std::move(fields)};
}
/* }}} */


/* {{{ Column_def::enable_unsigned */
void Column_def::enable_unsigned()
{
	is_unsigned = true;
}
/* }}} */


/* {{{ Column_def::set_decimals */
void Column_def::set_decimals(long dec_size)
{
	decimals = {true, dec_size};
}
/* }}} */


/* {{{ Column_def::set_charset */
void Column_def::set_charset(const phputils::string_ptr& chrset)
{
	charset = chrset.to_string();
}
/* }}} */


/* {{{ Column_def::set_collation */
void Column_def::set_collation(const phputils::string_ptr& coll)
{
	collation = coll.to_string();
}
/* }}} */


/* {{{ Column_def::enable_binary */
void Column_def::enable_binary()
{
	binary = true;
}
/* }}} */


/* {{{ Column_def::set_values */
void Column_def::set_values(phputils::strings v)
{
	values = std::move(v);
}
/* }}} */


/* {{{ Column_def::enable_stored */
void Column_def::enable_stored()
{
	stored = true;
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Foreign_key_def::set_fields */
void Foreign_key_def::set_fields(phputils::strings f)
{
	fields = std::move(f);
}
/* }}} */


/* {{{ Foreign_key_def::set_refers_to */
void Foreign_key_def::set_refers_to(
	const phputils::string_ptr& ref_table,
	phputils::strings ref_fields)
{
	refers_to.table = ref_table.to_string();
	refers_to.fields = std::move(ref_fields);
}
/* }}} */


/* {{{ Foreign_key_def::set_on_delete_mode */
void Foreign_key_def::set_on_delete_mode(const phputils::string_ptr& mode)
{
	on_delete_mode = to_fkey_change_mode(mode);
}
/* }}} */


/* {{{ Foreign_key_def::set_on_update_mode */
void Foreign_key_def::set_on_update_mode(const phputils::string_ptr& mode)
{
	on_update_mode = to_fkey_change_mode(mode);
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Table_def::init */
void Table_def::init(
	const phputils::string_ptr& s,
	const phputils::string_ptr& t,
	bool replace_if_exists)
{
	schema_name = create_table::quote_identifier(s.to_string());
	table_name = create_table::quote_identifier(t.to_string());
	replace_existing = replace_if_exists;
}
/* }}} */


/* {{{ Table_def::add_column */
void Table_def::add_column(Column_def column_def)
{
	columns.push_back(std::move(column_def));
}
/* }}} */


/* {{{ Table_def::set_primary_key */
void Table_def::set_primary_key(phputils::strings fields)
{
	primary_key = std::move(fields);
}
/* }}} */


/* {{{ Table_def::add_index */
void Table_def::add_index(const phputils::string_ptr& name, phputils::strings fields)
{
	indexes.push_back(Index{name.to_string(), std::move(fields), false});
}
/* }}} */


/* {{{ Table_def::add_unique_index */
void Table_def::add_unique_index(const phputils::string_ptr& name, phputils::strings fields)
{
	indexes.push_back(Index{name.to_string(), std::move(fields), true});
}
/* }}} */


/* {{{ Table_def::add_foreign_key */
void Table_def::add_foreign_key(const phputils::string_ptr& name, Foreign_key_def foreign_key)
{
	foreign_keys.push_back(Foreign_key{name.to_string(), std::move(foreign_key)});
}
/* }}} */


/* {{{ Table_def::set_initial_auto_increment */
void Table_def::set_initial_auto_increment(long init_auto_increment)
{
	initial_auto_increment = {true, init_auto_increment};
}
/* }}} */


/* {{{ Table_def::set_default_charset */
void Table_def::set_default_charset(const phputils::string_ptr& charset_name)
{
	default_charset = create_table::quote_text(charset_name.to_string());
}
/* }}} */


/* {{{ Table_def::set_default_collation */
void Table_def::set_default_collation(const phputils::string_ptr& collation_name)
{
	default_collation = create_table::quote_text(collation_name.to_string());
}
/* }}} */


/* {{{ Table_def::set_comment */
void Table_def::set_comment(const phputils::string_ptr& cmnt)
{
	comment = create_table::quote_text(cmnt.to_string());
}
/* }}} */


/* {{{ Table_def::enable_temporary */
void Table_def::enable_temporary()
{
	temporary = true;
}
/* }}} */


/* {{{ Table_def::set_defined_as */
void Table_def::set_defined_as(const phputils::string_ptr& def_as)
{
	defined_as = def_as.to_string();
}
/* }}} */


/* {{{ Table_def::set_like */
void Table_def::set_like(const phputils::string_ptr& template_table_name)
{
	like_template_table_name = template_table_name.to_string();
}
/* }}} */


/* {{{ Table_def::set_defined_as */
Table_def::Kind Table_def::get_kind() const
{
	if (!defined_as.empty()) {
		return Table_def::Kind::defined_as;
	} else if (!like_template_table_name.empty()) {
		return Table_def::Kind::like;
	} else {
		return Table_def::Kind::common;
	}
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
