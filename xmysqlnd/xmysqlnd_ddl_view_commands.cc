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
#include <boost/algorithm/string/compare.hpp>

#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include "xmysqlnd_ddl_view_commands.h"
#include "mysqlx_object.h"
#include "mysqlx_class_properties.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "crud_parsers/expression_parser.h"

#include "phputils/allocator.h"
#include "phputils/object.h"
#include "phputils/strings.h"
#include "phputils/string_utils.h"
#include "phputils/types.h"

namespace mysqlx {

namespace drv {

namespace {

/* {{{ to_algorithm */
::Mysqlx::Crud::ViewAlgorithm to_algorithm(const MYSQLND_CSTRING& algorithm_str)
{
	static const std::map<std::string, ::Mysqlx::Crud::ViewAlgorithm, phputils::iless> str_to_algorithm = {
		{ "undefined", ::Mysqlx::Crud::UNDEFINED },
		{ "merge", ::Mysqlx::Crud::MERGE },
		{ "temptable", ::Mysqlx::Crud::TEMPTABLE }
	};

	auto it = str_to_algorithm.find(algorithm_str.s);
	if (it == str_to_algorithm.cend()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_algorithm);
	}

	return it->second;
}
/* }}} */


/* {{{ to_security */
::Mysqlx::Crud::ViewSqlSecurity to_security(const MYSQLND_CSTRING& security_str)
{
	static const std::map<std::string, ::Mysqlx::Crud::ViewSqlSecurity, phputils::iless> str_to_security = {
		{ "invoker", ::Mysqlx::Crud::INVOKER },
		{ "definer", ::Mysqlx::Crud::DEFINER }
	};

	auto it = str_to_security.find(security_str.s);
	if (it == str_to_security.end()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_security);
	}

	return it->second;
}
/* }}} */


/* {{{ to_check_option */
::Mysqlx::Crud::ViewCheckOption to_check_option(const MYSQLND_CSTRING& check_option_str)
{
	static const std::map<std::string, ::Mysqlx::Crud::ViewCheckOption, phputils::iless> str_to_check_option = {
		{ "local", ::Mysqlx::Crud::LOCAL },
		{ "cascaded", ::Mysqlx::Crud::CASCADED }
	};

	auto it = str_to_check_option.find(check_option_str.s);
	if (it == str_to_check_option.end()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::invalid_view_check_option);
	}

	return it->second;
}
/* }}} */

} // anonymous namespace {

//------------------------------------------------------------------------------

/* {{{ Create_view_cmd::set_view_name */
void Create_view_cmd::set_view_name(
	const MYSQLND_CSTRING& schema_name,
	const MYSQLND_CSTRING& view_name)
{
	msg.mutable_collection()->set_schema(schema_name.s, schema_name.l);
	msg.mutable_collection()->set_name(view_name.s, view_name.l);
}
/* }}} */


/* {{{ Create_view_cmd::set_definer */
void Create_view_cmd::set_definer(const MYSQLND_CSTRING& definer)
{
	msg.set_definer(definer.s, definer.l);
}
/* }}} */


/* {{{ Create_view_cmd::set_definer */
void Create_view_cmd::set_algorithm(const MYSQLND_CSTRING& algorithm)
{
	msg.set_algorithm(to_algorithm(algorithm));
}
/* }}} */


/* {{{ Create_view_cmd::set_security */
void Create_view_cmd::set_security(const MYSQLND_CSTRING& security)
{
	msg.set_security(to_security(security));
}
/* }}} */


/* {{{ Create_view_cmd::set_check_option */
void Create_view_cmd::set_check_option(const MYSQLND_CSTRING& check_option)
{
	msg.set_check(to_check_option(check_option));
}
/* }}} */


/* {{{ Create_view_cmd::add_column */
void Create_view_cmd::add_column(const MYSQLND_CSTRING& column)
{
	msg.add_column(column.s, column.l);
}
/* }}} */


/* {{{ Create_view_cmd::set_stmt */
void Create_view_cmd::set_stmt(const Mysqlx::Crud::Find& stmt)
{
	msg.mutable_stmt()->CopyFrom(stmt);
}
/* }}} */


/* {{{ Create_view_cmd::set_replace_existing */
void Create_view_cmd::set_replace_existing(const bool replace_existing)
{
	msg.set_replace_existing(replace_existing);
}
/* }}} */


/* {{{ Create_view_cmd::get_message */
st_xmysqlnd_pb_message_shell Create_view_cmd::get_message()
{
	if (!msg.IsInitialized()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::view_create_fail);
	}

	st_xmysqlnd_pb_message_shell pb_msg { &msg, COM_CRUD_CREATE_VIEW };
	return pb_msg;
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Alter_view_cmd::set_view_name */
void Alter_view_cmd::set_view_name(
	const MYSQLND_CSTRING& schema_name,
	const MYSQLND_CSTRING& view_name)
{
	msg.mutable_collection()->set_schema(schema_name.s, schema_name.l);
	msg.mutable_collection()->set_name(view_name.s, view_name.l);
}
/* }}} */


/* {{{ Alter_view_cmd::set_definer */
void Alter_view_cmd::set_definer(const MYSQLND_CSTRING& definer)
{
	msg.set_definer(definer.s, definer.l);
}
/* }}} */


/* {{{ Alter_view_cmd::set_definer */
void Alter_view_cmd::set_algorithm(const MYSQLND_CSTRING& algorithm)
{
	msg.set_algorithm(to_algorithm(algorithm));
}
/* }}} */


/* {{{ Alter_view_cmd::set_security */
void Alter_view_cmd::set_security(const MYSQLND_CSTRING& security)
{
	msg.set_security(to_security(security));
}
/* }}} */


/* {{{ Alter_view_cmd::set_check_option */
void Alter_view_cmd::set_check_option(const MYSQLND_CSTRING& check_option)
{
	msg.set_check(to_check_option(check_option));
}
/* }}} */


/* {{{ Alter_view_cmd::add_column */
void Alter_view_cmd::add_column(const MYSQLND_CSTRING& column)
{
	msg.add_column(column.s, column.l);
}
/* }}} */


/* {{{ Alter_view_cmd::set_stmt */
void Alter_view_cmd::set_stmt(const Mysqlx::Crud::Find& stmt)
{
	msg.mutable_stmt()->CopyFrom(stmt);
}
/* }}} */


/* {{{ Alter_view_cmd::get_message */
st_xmysqlnd_pb_message_shell Alter_view_cmd::get_message()
{
	if (!msg.IsInitialized()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::view_alter_fail);
	}

	st_xmysqlnd_pb_message_shell pb_msg { &msg, COM_CRUD_MODIFY_VIEW };
	return pb_msg;
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ Drop_view_cmd::set_view_name */
void Drop_view_cmd::set_view_name(
	const phputils::string_input_param& schema_name,
	const phputils::string_input_param& view_name)
{
	msg.mutable_collection()->set_schema(schema_name.c_str(), schema_name.length());
	msg.mutable_collection()->set_name(view_name.c_str(), view_name.length());
}
/* }}} */


/* {{{ Drop_view_cmd::set_if_exists */
void Drop_view_cmd::set_if_exists(const bool if_exists)
{
	msg.set_if_exists(if_exists);
}
/* }}} */


/* {{{ Drop_view_cmd::get_message */
st_xmysqlnd_pb_message_shell Drop_view_cmd::get_message()
{
	if (!msg.IsInitialized()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::view_drop_fail);
	}

	st_xmysqlnd_pb_message_shell pb_msg { &msg, COM_CRUD_DROP_VIEW };
	return pb_msg;
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
