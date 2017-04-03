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
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_utils.h"
#include "xmysqlnd_table_create.h"
#include "xmysqlnd_ddl_table_defs.h"

#include "phputils/exceptions.h"
#include "phputils/strings.h"
#include "phputils/string_utils.h"

#include <boost/algorithm/string/join.hpp>
#include <iostream>

#define NIY
#ifdef  NIY

namespace mysqlx {

namespace drv {

namespace create_table
{

namespace {

/* {{{ to_data_type */
phputils::string to_data_type(Column_def::Type column_type)
{
	static const std::map<Column_def::Type, std::string> data_type_to_str = {
		{ Column_def::Type::bit, "BIT" },
		{ Column_def::Type::tiny_int, "TINYINT" },
		{ Column_def::Type::small_int, "SMALLINT" },
		{ Column_def::Type::medium_int, "MEDIUMINT" },
		{ Column_def::Type::integer, "INTEGER" },
		{ Column_def::Type::big_int, "BIGINT" },
		{ Column_def::Type::real, "REAL" },
		{ Column_def::Type::real_double, "DOUBLE" },
		{ Column_def::Type::real_float, "FLOAT" },
		{ Column_def::Type::decimal, "DECIMAL" },
		{ Column_def::Type::numeric, "NUMERIC" },
		{ Column_def::Type::date, "DATE" },
		{ Column_def::Type::time, "TIME" },
		{ Column_def::Type::timestamp, "TIMESTAMP" },
		{ Column_def::Type::datetime, "DATETIME" },
		{ Column_def::Type::year, "YEAR" },
		{ Column_def::Type::character, "CHARACTER" },
		{ Column_def::Type::var_char, "VARCHAR" },
		{ Column_def::Type::binary, "BINARY" },
		{ Column_def::Type::var_binary, "VARBINARY" },
		{ Column_def::Type::tiny_blob, "TINYBLOB" },
		{ Column_def::Type::blob, "BLOB" },
		{ Column_def::Type::medium_blob, "MEDIUMBLOB" },
		{ Column_def::Type::long_blob, "LONGBLOB" },
		{ Column_def::Type::tiny_text, "TINYTEXT" },
		{ Column_def::Type::text, "TEXT" },
		{ Column_def::Type::medium_text, "MEDIUMTEXT" },
		{ Column_def::Type::long_text, "LONGTEXT" },
		{ Column_def::Type::enumeration, "ENUM" },
		{ Column_def::Type::set, "SET" },
		{ Column_def::Type::json, "JSON" }
	};

	auto it = data_type_to_str.find(column_type);
	if (it == data_type_to_str.cend()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::unknown_table_column_type);
	}

	return phputils::to_string(it->second);
}
/* }}} */


/* {{{ to_reference_option */
phputils::string to_reference_option(Foreign_key_def::Change_mode fk_change_mode)
{
	static const std::map<Foreign_key_def::Change_mode, std::string> str_to_reference_option = {
		{ Foreign_key_def::Change_mode::set_default, "" }, //empty intentionally
		{ Foreign_key_def::Change_mode::restricted, "RESTRICT" },
		{ Foreign_key_def::Change_mode::cascade, "CASCADE" },
		{ Foreign_key_def::Change_mode::set_null, "SET NULL" },
		{ Foreign_key_def::Change_mode::no_action, "NO ACTION" }
	};

	auto it = str_to_reference_option.find(fk_change_mode);
	if (it == str_to_reference_option.cend()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::unknown_fkey_change_mode);
	}

	return phputils::to_string(it->second);
}
/* }}} */

//------------------------------------------------------------------------------

namespace chr
{
const char whitespace = ' ';
const char* comma = ",";
}

/* {{{ query_stream_manip */
struct query_stream_manip
{
	query_stream_manip() = default;
	query_stream_manip(phputils::string txt)
		: text(std::move(txt))
	{
	}

	static query_stream_manip empty()
	{
		return query_stream_manip();
	}

	phputils::string text;
};
/* }}} */


/* {{{ operator<< */
std::ostream& operator<<(std::ostream& os, query_stream_manip qbm)
{
	return os << qbm.text;
}
/* }}} */


/* {{{ token */
query_stream_manip token(const char* token)
{
	return (phputils::to_string(token) + chr::whitespace);
}
/* }}} */


/* {{{ attrib */
query_stream_manip attrib(bool enable, const char* symbol)
{
	if (enable) {
		return token(symbol);
	}
	return query_stream_manip::empty();
}
/* }}} */


/* {{{ attrib */
query_stream_manip attrib(
	bool enable,
	const char* symbol_if_enabled,
	const char* symbol_if_disabled)
{
	return token(enable ? symbol_if_enabled : symbol_if_disabled);
}
/* }}} */


/* {{{ value */
query_stream_manip value(
	const phputils::string& text,
	const char postfix = chr::whitespace)
{
	if (text.empty()) return query_stream_manip::empty();
	return text + postfix;
}
/* }}} */


/* {{{ value */
query_stream_manip value(
	const char* label,
	const phputils::string& text)
{
	if (text.empty()) return query_stream_manip::empty();
	return (phputils::to_string(label) + chr::whitespace + text + chr::whitespace);
}
/* }}} */


/* {{{ value */
query_stream_manip value(const char* symbol, const optional_long& value)
{
	if (value.first) {
		phputils::ostringstream os;
		os << symbol << chr::whitespace << value.second << chr::whitespace;
		return os.str();
	}
	return query_stream_manip::empty();
}
/* }}} */


/* {{{ values */
query_stream_manip values(
	const char* label,
	const phputils::strings& values)
{
	if (values.empty()) return query_stream_manip::empty();

	phputils::ostringstream os;

	if (label) {
		os << label << chr::whitespace;
	}

	os << '(' << boost::algorithm::join(values, chr::comma) << ')' << chr::whitespace;

	return os.str();
}
/* }}} */


/* {{{ values */
query_stream_manip values(
	const phputils::strings& vals)
{
	return values(nullptr, vals);
}
/* }}} */

//------------------------------------------------------------------------------

/*
	build SQL os basing on following reference:
	https://dev.mysql.com/doc/refman/5.7/en/create-table.html
*/
/* {{{ create_table_on_error */
class Query_builder
{
	public:
		phputils::string execute(const Table_def& tbl);

	private:
		void stream_table_common(const Table_def& tbl_def);
		void stream_table_defined_as(const Table_def& tbl_def);
		void stream_table_like(const Table_def& tbl_def);

		void stream_create_prefix(const Table_def& tbl_def);
		void stream_create_definitions(const Table_def& tbl_def);
		void stream_create_def_separator();
		void stream_table_options(const Table_def& tbl_def);

		void stream_column_def(const Column_def& column);
		void stream_primary_key(const Table_def& tbl_def);
		void stream_index(const Table_def::Index& index);
		void stream_foreign_key(const Table_def::Foreign_key& tbl_fkey);
		void stream_reference_definition(const Foreign_key_def& fk_def);

		void stream_column_def_common(const Column_def& column);
		void stream_generated_column_def(const Column_def& column);
		void stream_data_type(const Column_def& column);
		void stream_column_reference_def(const Column_def::Foreign_key& foreign_key);

	private:
		phputils::ostringstream os;

		bool first_create_definition = true;

};
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Query_builder::execute */
phputils::string Query_builder::execute(const Table_def& tbl_def)
{
	switch (tbl_def.get_kind()) {
		case Table_def::Kind::common:
			stream_table_common(tbl_def);
			break;

		case Table_def::Kind::defined_as:
			stream_table_defined_as(tbl_def);
			break;

		case Table_def::Kind::like:
			stream_table_like(tbl_def);
			break;

		default:
			assert(!"uknown kind of Table_def!");
	}

	return os.str();
}
/* }}} */


/*
	CREATE [TEMPORARY] TABLE [IF NOT EXISTS] tbl_name
		(create_definition,...)
		[table_options]
*/
/* {{{ Query_builder::stream_table_common */
void Query_builder::stream_table_common(const Table_def& tbl_def)
{
	stream_create_prefix(tbl_def);
	stream_create_definitions(tbl_def);
	stream_table_options(tbl_def);
}
/* }}} */


/*
	CREATE [TEMPORARY] TABLE [IF NOT EXISTS] tbl_name
		[(create_definition,...)]
		[table_options]
		[AS] query_expression
*/
/* {{{ Query_builder::stream_table_defined_as */
void Query_builder::stream_table_defined_as(const Table_def& tbl_def)
{
	stream_create_prefix(tbl_def);
	stream_create_definitions(tbl_def);
	stream_table_options(tbl_def);
	os << value("AS", tbl_def.defined_as);
}
/* }}} */


/*
	CREATE [TEMPORARY] TABLE [IF NOT EXISTS] tbl_name
		{ LIKE old_tbl_name | (LIKE old_tbl_name) }
*/
/* {{{ Query_builder::stream_table_like */
void Query_builder::stream_table_like(const Table_def& tbl_def)
{
	stream_create_prefix(tbl_def);
	os << value("LIKE", tbl_def.like_template_table_name);
}
/* }}} */


/*
	CREATE [TEMPORARY] TABLE [IF NOT EXISTS] tbl_name
*/
/* {{{ Query_builder::stream_create_prefix */
void Query_builder::stream_create_prefix(const Table_def& tbl_def)
{
	os << token("CREATE")
		<< attrib(tbl_def.temporary, "TEMPORARY")
		<< token("TABLE")
		<< attrib(!tbl_def.replace_existing, "IF NOT EXISTS")
		<< value(tbl_def.schema_name, '.')
		<< value(tbl_def.table_name);
}
/* }}} */


/*
	(create_definition,...)

	create_definition:
		col_name column_definition
	  | [CONSTRAINT [symbol]] PRIMARY KEY [index_type] (index_col_name,...)
		  [index_option] ...
	  | {INDEX|KEY} [index_name] [index_type] (index_col_name,...)
		  [index_option] ...
	  | [CONSTRAINT [symbol]] UNIQUE [INDEX|KEY]
		  [index_name] [index_type] (index_col_name,...)
		  [index_option] ...
	  | [CONSTRAINT [symbol]] FOREIGN KEY
		  [index_name] (index_col_name,...) reference_definition
	  | CHECK (expr)
*/
/* {{{ Query_builder::stream_create_definitions */
void Query_builder::stream_create_definitions(const Table_def& tbl_def)
{
	os << token("(");

	for (auto& column_def : tbl_def.columns) {
		stream_create_def_separator();
		stream_column_def(column_def);
	}

	stream_primary_key(tbl_def);

	for (auto& index : tbl_def.indexes) {
		stream_create_def_separator();
		stream_index(index);
	}

	for (auto& foreign_key : tbl_def.foreign_keys) {
		stream_create_def_separator();
		stream_foreign_key(foreign_key);
	}

	os << token(")");
}
/* }}} */


/* {{{ Query_builder::stream_create_definitions */
void Query_builder::stream_create_def_separator()
{
	if (first_create_definition) {
		first_create_definition = false;
	} else {
		os << token(chr::comma);
	}
}
/* }}} */


/*
	table_option:
	  | AUTO_INCREMENT [=] value
	  | [DEFAULT] CHARACTER SET [=] charset_name
	  | [DEFAULT] COLLATE [=] collation_name
	  | COMMENT [=] 'string'
*/
/* {{{ Query_builder::stream_table_options */
void Query_builder::stream_table_options(const Table_def& tbl_def)
{
	os << value("AUTO_INCREMENT", tbl_def.initial_auto_increment)
		<< value("DEFAULT CHARACTER SET", tbl_def.default_charset)
		<< value("DEFAULT COLLATE", tbl_def.default_collation)
		<< value("COMMENT", tbl_def.comment);
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Query_builder::stream_column_def */
void Query_builder::stream_column_def(const Column_def& column_def)
{
	os << value(column_def.name);
	stream_data_type(column_def);

	if (column_def.kind == Column_def::Kind::common) {
		stream_column_def_common(column_def);
	} else {
		assert(column_def.kind == Column_def::Kind::generated);
		stream_generated_column_def(column_def);
	}
}
/* }}} */


/*
	| [CONSTRAINT [symbol]] PRIMARY KEY [index_type] (index_col_name,...)
		[index_option] ...
*/
/* {{{ Query_builder::stream_primary_key */
void Query_builder::stream_primary_key(const Table_def& tbl_def)
{
	if (!tbl_def.primary_key.empty()) {
		stream_create_def_separator();
		os << values("PRIMARY KEY", tbl_def.primary_key);
	}
}
/* }}} */


/*
	| [CONSTRAINT [symbol]] UNIQUE [INDEX|KEY]
		[index_name] [index_type] (index_col_name,...)
		[index_option] ...
*/
/* {{{ Query_builder::stream_index */
void Query_builder::stream_index(const Table_def::Index& index)
{
	if (index.unique) {
		os << token("UNIQUE");
	}

	os << token("INDEX") << value(index.name) << values(index.columns);
}
/* }}} */


/*
	| [CONSTRAINT [symbol]] FOREIGN KEY
		[index_name] (index_col_name,...) reference_definition
*/
/* {{{ Query_builder::stream_foreign_keys */
void Query_builder::stream_foreign_key(const Table_def::Foreign_key& tbl_fkey)
{
	os << token("FOREIGN KEY") << value(tbl_fkey.name);

	const Foreign_key_def& fk_def = tbl_fkey.foreign_key;
	os << values(fk_def.fields);
	stream_reference_definition(fk_def);
}
/* }}} */


/*
	reference_definition:
		REFERENCES tbl_name (index_col_name,...)
		  [ON DELETE reference_option]
		  [ON UPDATE reference_option]
*/
/* {{{ Query_builder::stream_reference_definition */
void Query_builder::stream_reference_definition(const Foreign_key_def& fk_def)
{
	os << token("REFERENCES") << value(fk_def.refers_to.table)
		<< values(fk_def.refers_to.fields)
		<< value("ON DELETE", to_reference_option(fk_def.on_delete_mode))
		<< value("ON UPDATE", to_reference_option(fk_def.on_update_mode));
}
/* }}} */

//------------------------------------------------------------------------------

/*
	column_definition:
		data_type [NOT NULL | NULL] [DEFAULT default_value]
		  [AUTO_INCREMENT] [UNIQUE [KEY] | [PRIMARY] KEY]
		  [COMMENT 'string']
		  [reference_definition]
*/
/* {{{ Query_builder::stream_column_def_common */
void Query_builder::stream_column_def_common(const Column_def& column_def)
{
	os << attrib(column_def.not_null, "NOT NULL", "NULL")
		<< value("DEFAULT", column_def.default_value)
		<< attrib(column_def.auto_increment, "AUTO_INCREMENT")
		<< attrib(column_def.unique_index, "UNIQUE KEY")
		<< attrib(column_def.primary_key, "PRIMARY KEY")
		<< value("COMMENT", column_def.comment);

	stream_column_reference_def(column_def.foreign_key);
}
/* }}} */


/*
  | data_type [GENERATED ALWAYS] AS (expression)
      [VIRTUAL | STORED] [UNIQUE [KEY]] [COMMENT comment]
      [NOT NULL | NULL] [[PRIMARY] KEY]
*/
/* {{{ Query_builder::stream_generated_column_def */
void Query_builder::stream_generated_column_def(const Column_def& column_def)
{
	os << value("GENERATED ALWAYS AS", column_def.generated_as_expr)
		<< attrib(column_def.stored, "STORED")
		<< attrib(column_def.unique_index, "UNIQUE KEY")
		<< value("COMMENT", column_def.comment)
		<< attrib(column_def.not_null, "NOT NULL", "NULL")
		<< attrib(column_def.primary_key, "PRIMARY KEY");
}
/* }}} */


/*
	data_type:
		BIT[(length)]
	  | TINYINT[(length)] [UNSIGNED] [ZEROFILL]
	  | SMALLINT[(length)] [UNSIGNED] [ZEROFILL]
	  | MEDIUMINT[(length)] [UNSIGNED] [ZEROFILL]
	  | INT[(length)] [UNSIGNED] [ZEROFILL]
	  | INTEGER[(length)] [UNSIGNED] [ZEROFILL]
	  | BIGINT[(length)] [UNSIGNED] [ZEROFILL]
	  | REAL[(length,decimals)] [UNSIGNED] [ZEROFILL]
	  | DOUBLE[(length,decimals)] [UNSIGNED] [ZEROFILL]
	  | FLOAT[(length,decimals)] [UNSIGNED] [ZEROFILL]
	  | DECIMAL[(length[,decimals])] [UNSIGNED] [ZEROFILL]
	  | NUMERIC[(length[,decimals])] [UNSIGNED] [ZEROFILL]
	  | DATE
	  | TIME[(fsp)]
	  | TIMESTAMP[(fsp)]
	  | DATETIME[(fsp)]
	  | YEAR
	  | CHAR[(length)] [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | VARCHAR(length) [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | BINARY[(length)]
	  | VARBINARY(length)
	  | TINYBLOB
	  | BLOB
	  | MEDIUMBLOB
	  | LONGBLOB
	  | TINYTEXT [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | TEXT [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | MEDIUMTEXT [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | LONGTEXT [BINARY]
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | ENUM(value1,value2,value3,...)
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | SET(value1,value2,value3,...)
		  [CHARACTER SET charset_name] [COLLATE collation_name]
	  | JSON
	  | spatial_type
*/
/* {{{ Query_builder::stream_data_type */
void Query_builder::stream_data_type(const Column_def& column_def)
{
	os << to_data_type(column_def.type);
	if (column_def.length.first) {
		os << '(' << column_def.length.second;
		if (column_def.decimals.first) {
			os << ',' << column_def.decimals.second;
		}
		os << token(")");
	} else if (!column_def.values.empty()) {
		os << values(column_def.values);
	} else {
		os << chr::whitespace;
	}

	os << attrib(column_def.is_unsigned, "UNSIGNED")
		<< attrib(column_def.binary, "BINARY")
		<< value("CHARACTER SET", column_def.charset)
		<< value("COLLATE", column_def.collation);
}
/* }}} */


/*
	reference_definition:
		REFERENCES tbl_name (index_col_name,...)
*/
/* {{{ Query_builder::reference_definition */
void Query_builder::stream_column_reference_def(const Column_def::Foreign_key& foreign_key)
{
	if (!foreign_key.table.empty()) {
		os << value("REFERENCES", foreign_key.table)
			<< values(foreign_key.fields);
	}
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ create_table_on_error */
const enum_hnd_func_status create_table_on_error(
	void * context,
	XMYSQLND_NODE_SESSION* const session,
	XMYSQLND_NODE_STMT* const stmt,
	const unsigned int code,
	const MYSQLND_CSTRING sql_state,
	const MYSQLND_CSTRING message)
{
	DBG_ENTER("create_table_on_error");
	throw phputils::xdevapi_exception(
		code,
		phputils::string(sql_state.s, sql_state.l),
		phputils::string(message.s, message.l));
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */

} // anonymous namespace


/* {{{ execute */
bool execute(
	st_xmysqlnd_node_session* session,
	const Table_def& table_def)
{
	DBG_ENTER("create_table");

	Query_builder qb;
	const phputils::string& create_table_query = qb.execute(table_def);
	const MYSQLND_CSTRING sql_query = {create_table_query.c_str(), create_table_query.length()};
	st_xmysqlnd_node_session_on_error_bind on_error = {create_table_on_error, nullptr};
	enum_func_status ret = session->m->query_cb(
		session,
		namespace_sql,
		sql_query,
		noop__var_binder,
		noop__on_result_start,
		noop__on_row,
		noop__on_warning,
		on_error,
		noop__on_result_end,
		noop__on_statement_ok);

	DBG_RETURN(ret == PASS);
}
/* }}} */


/* {{{ get_sql_query */
phputils::string get_sql_query(const Table_def& table_def)
{
	Query_builder qb;
	return qb.execute(table_def);
}
/* }}} */

} // namespace create_table

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

#endif
