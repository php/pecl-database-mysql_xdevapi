/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is_ subject to version 3.01 of the PHP license,      |
  | that is_ bundled with this package in the file LICENSE, and is_        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_DDL_TABLE_DEFS_H
#define XMYSQLND_DDL_TABLE_DEFS_H

#include "xmysqlnd_crud_commands.h"
#include "proto_gen/mysqlx_crud.pb.h"
#include "phputils/strings.h"

namespace mysqlx {

namespace drv {

namespace create_table {

phputils::string quote_identifier(phputils::string raw_id);
phputils::string quote_text(phputils::string raw_text);

} // namespace create_table

using optional_long = std::pair<bool, long>;

/* {{{ Column_def */
struct Column_def
{
	public:
		enum class Kind {
			unknown,
			common,
			generated
		};

		enum class Type {
			unknown,
			bit,
			tiny_int,
			small_int,
			medium_int,
			integer,
			big_int,
			real,
			real_double,
			real_float,
			decimal,
			numeric,
			date,
			time,
			timestamp,
			datetime,
			year,
			character,
			var_char,
			binary,
			var_binary,
			tiny_blob,
			blob,
			medium_blob,
			long_blob,
			tiny_text,
			text,
			medium_text,
			long_text,
			enumeration,
			set,
			json
		};

		static const long Default_length = 0;

	public:
		struct Foreign_key
		{
			phputils::string table;
			phputils::strings fields;
		};

	public:
		void init(
			const phputils::string_ptr& name, 
			const phputils::string_ptr& type, 
			long length = Default_length);
		void init(
			const phputils::string_ptr& name, 
			const phputils::string_ptr& type, 
			const phputils::string_ptr& expression);

		void enable_not_null();
		void enable_unique_index();
		void enable_primary_key();
		void set_comment(const phputils::string_ptr& comment);

		void set_default_value(phputils::string default_value_expr);
		void enable_auto_increment();
		void set_foreign_key(const phputils::string_ptr& table, phputils::strings fields);
		void enable_unsigned();
		void set_decimals(long decimals);
		void set_charset(const phputils::string_ptr& charset);
		void set_collation(const phputils::string_ptr& collation);
		void enable_binary();
		void set_values(phputils::strings values);

		void enable_stored();

	public:
		Kind kind = Kind::unknown;

		phputils::string name;
		Type type = Type::unknown;
		optional_long length;
		phputils::string generated_as_expr;

		bool not_null = false;
		bool unique_index = false;
		bool primary_key = false;
		phputils::string comment;

		phputils::string default_value;
		bool auto_increment = false;
		Foreign_key foreign_key;
		bool is_unsigned = false;
		optional_long decimals;
		phputils::string charset;
		phputils::string collation;
		bool binary = false;
		phputils::strings values;

		bool stored = false;
};
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Foreign_key_def */
struct Foreign_key_def
{
	public:
		enum class Change_mode {
			set_default,
			restricted, 
			cascade, 
			set_null,
			no_action
		};

	public:
		struct Reference
		{
			phputils::string table;
			phputils::strings fields;
		};

	public:
		void set_fields(phputils::strings fields);
		void set_refers_to(const phputils::string_ptr& table, phputils::strings fields);
		void set_on_delete_mode(const phputils::string_ptr& mode);
		void set_on_update_mode(const phputils::string_ptr& mode);

	public:
		phputils::strings fields;
		Reference refers_to;
		Change_mode on_delete_mode = Change_mode::set_default;
		Change_mode on_update_mode = Change_mode::set_default;

};
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Table_def */
struct Table_def
{
	public:
		using Column_defs = phputils::vector<Column_def>;


		struct Index
		{
			phputils::string name;
			phputils::strings columns;
//			bool unique = false;
			bool unique;
		};

		using Indexes = phputils::vector<Index>;


		struct Foreign_key
		{
			phputils::string name;
			Foreign_key_def foreign_key;
		};

		using Foreign_keys = phputils::vector<Foreign_key>;

	public:
		void init(
			const phputils::string_ptr& schema_name,
			const phputils::string_ptr& table_name,
			bool replace_if_exists);

		void add_column(Column_def column_def);
		void set_primary_key(phputils::strings fields);
		void add_index(const phputils::string_ptr& name, phputils::strings fields);
		void add_unique_index(const phputils::string_ptr& name, phputils::strings fields);
		void add_foreign_key(const phputils::string_ptr& name, Foreign_key_def foreign_key);
		void set_initial_auto_increment(long init_auto_increment);
		void set_default_charset(const phputils::string_ptr& charset_name);
		void set_default_collation(const phputils::string_ptr& collation_name);
		void set_comment(const phputils::string_ptr& comment);
		void enable_temporary();
		void set_defined_as(const phputils::string_ptr& defined_as);
		void set_like(const phputils::string_ptr& template_table_name);

	public:
		enum class Kind {
			common,
			defined_as,
			like
		};

		Kind get_kind() const;
		
	public:
		phputils::string schema_name;
		phputils::string table_name;
		bool replace_existing = false;
		Column_defs columns;
		phputils::strings primary_key;
		Indexes indexes;
		Foreign_keys foreign_keys;
		optional_long initial_auto_increment;
		phputils::string default_charset;
		phputils::string default_collation;
		phputils::string comment;
		bool temporary = false;
		phputils::string defined_as;
		phputils::string like_template_table_name;

};
/* }}} */

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_DDL_TABLE_DEFS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
