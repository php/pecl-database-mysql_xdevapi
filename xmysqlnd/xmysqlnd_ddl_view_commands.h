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
#ifndef XMYSQLND_DDL_VIEW_COMMANDS_H
#define XMYSQLND_DDL_VIEW_COMMANDS_H

#include "xmysqlnd_crud_commands.h"
#include "proto_gen/mysqlx_crud.pb.h"
#include "phputils/strings.h"

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_node_stmt;

/* {{{ Create_view_cmd */
class Create_view_cmd
{
	public:
		void set_view_name(
			const MYSQLND_CSTRING& schema_name,
			const MYSQLND_CSTRING& view_name);

		void set_definer(const MYSQLND_CSTRING& definer);
		void set_algorithm(const MYSQLND_CSTRING& algorithm);
		void set_security(const MYSQLND_CSTRING& security);
		void set_check_option(const MYSQLND_CSTRING& check_option);

		void add_column(const MYSQLND_CSTRING& column);
		void set_stmt(const Mysqlx::Crud::Find& stmt);

		void set_replace_existing(const bool replace_existing);

		st_xmysqlnd_pb_message_shell get_message();

	private:
		Mysqlx::Crud::CreateView msg;
};
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Alter_view_cmd */
class Alter_view_cmd
{
	public:
		void set_view_name(
			const MYSQLND_CSTRING& schema_name,
			const MYSQLND_CSTRING& view_name);

		void set_definer(const MYSQLND_CSTRING& definer);
		void set_algorithm(const MYSQLND_CSTRING& algorithm);
		void set_security(const MYSQLND_CSTRING& security);
		void set_check_option(const MYSQLND_CSTRING& check_option);

		void add_column(const MYSQLND_CSTRING& column);
		void set_stmt(const Mysqlx::Crud::Find& stmt);

		st_xmysqlnd_pb_message_shell get_message();

	private:
		Mysqlx::Crud::ModifyView msg;
};
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Drop_view_cmd */
class Drop_view_cmd
{
	public:
		void set_view_name(
			const phputils::string_view& schema_name,
			const phputils::string_view& view_name);

		void set_if_exists(const bool if_exists);

		st_xmysqlnd_pb_message_shell get_message();

	private:
		Mysqlx::Crud::DropView msg;
};
/* }}} */

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_DDL_VIEW_COMMANDS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
