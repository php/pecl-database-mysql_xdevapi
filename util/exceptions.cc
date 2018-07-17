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
#include "exceptions.h"
#include "mysqlx_exception.h"
#include "types.h"

namespace mysqlx {

namespace util {

namespace
{

const char* const general_sql_state = GENERAL_SQL_STATE;

/* {{{ mysqlx::util::code_to_err_msg */
const std::map<xdevapi_exception::Code, const char* const> code_to_err_msg = {
	{ xdevapi_exception::Code::fetch_fail, "Couldn't fetch data" },
	{ xdevapi_exception::Code::meta_fail, "Unable to extract metadata" },
	{ xdevapi_exception::Code::add_doc, "Error adding document" },
	{ xdevapi_exception::Code::json_fail, "Error serializing document to JSON" },
	{ xdevapi_exception::Code::add_index_field_err, "Error while adding an index field" },
	{ xdevapi_exception::Code::add_orderby_fail, "Error while adding a orderby expression" },
	{ xdevapi_exception::Code::add_sort_fail, "Error while adding a sort expression" },
	{ xdevapi_exception::Code::add_where_fail, "Error while adding a where expression" },
	{ xdevapi_exception::Code::bind_fail, "Error while binding a variable" },
	{ xdevapi_exception::Code::merge_fail, "Error while merging" },
	{ xdevapi_exception::Code::unset_fail, "Error while unsetting a variable" },
	{ xdevapi_exception::Code::find_fail, "Find not completely initialized" },
	{ xdevapi_exception::Code::insert_fail, "Insert not completely initialized" },
	{ xdevapi_exception::Code::invalid_type, "Invalid value type" },
	{ xdevapi_exception::Code::modify_fail, "Modify not completely initialized" },
	{ xdevapi_exception::Code::remove_fail, "Remove not completely initialized" },
	{ xdevapi_exception::Code::wrong_param_1, "Parameter must be an array of strings" },
	{ xdevapi_exception::Code::wrong_param_2, "Parameter must be a non-negative value" },
	{ xdevapi_exception::Code::wrong_param_3, "Parameter must be a string or array of strings" },
	{ xdevapi_exception::Code::wrong_param_4, "Parameter must be a string." },
	{ xdevapi_exception::Code::wrong_param_string_or_strings, "Wrong parameter %s: must be a string or array of strings" },
	{ xdevapi_exception::Code::unsupported_conversion_to_string, "Unsupported zval conversion to string." },
	{ xdevapi_exception::Code::unsupported_default_value_type, "Unsupported zval conversion to string." },
	{ xdevapi_exception::Code::add_field, "Error while adding a fields list" },
	{ xdevapi_exception::Code::delete_fail, "Delete not completely initialized" },
	{ xdevapi_exception::Code::update_fail, "Update not completely initialized" },
	{ xdevapi_exception::Code::create_index_fail, "CreateIndex not completely initialized" },
	{ xdevapi_exception::Code::drop_index_fail, "DropIndex not completely initialized" },
	{ xdevapi_exception::Code::arridx_del_fail, "Error while deleting an array index" },
	{ xdevapi_exception::Code::view_create_fail, "Create view not completely initialized" },
	{ xdevapi_exception::Code::view_alter_fail, "Alter view not completely initialized" },
	{ xdevapi_exception::Code::view_drop_fail, "Drop view not completely initialized" },
	{ xdevapi_exception::Code::invalid_view_algorithm, "Invalid view algorithm" },
	{ xdevapi_exception::Code::invalid_view_security, "Invalid security context" },
	{ xdevapi_exception::Code::invalid_view_check_option, "Invalid view check option" },
	{ xdevapi_exception::Code::invalid_view_columns, "Invalid view columns - expected string or array of strings" },
	{ xdevapi_exception::Code::invalid_view_defined_as, "Invalid view defined as - expected table select or collection find statement" },
	{ xdevapi_exception::Code::invalid_table_column, "Expected table column" },
	{ xdevapi_exception::Code::unknown_table_column_type, "Unknown column type" },
	{ xdevapi_exception::Code::invalid_table_column_length, "Invalid column length" },
	{ xdevapi_exception::Code::invalid_table_column_length_decimals, "cannot set decimals when length is not set" },
	{ xdevapi_exception::Code::invalid_foreign_key, "Invalid foreign key" },
	{ xdevapi_exception::Code::unknown_fkey_change_mode, "Unknown foreign key change mode" },
	{ xdevapi_exception::Code::invalid_identifier, "Invalid MySQL identifier provided" },
	{ xdevapi_exception::Code::inconsistent_ssl_options, "Inconsistent ssl options" },
	{ xdevapi_exception::Code::invalid_auth_mechanism, "Invalid authentication mechanism" },
	{ xdevapi_exception::Code::unknown_lock_waiting_option, "Unknown lock waiting option" },
	{ xdevapi_exception::Code::schema_creation_failed, "Unable to create the schema object" },
	{ xdevapi_exception::Code::table_creation_failed, "Unable to create the table object"},
	{ xdevapi_exception::Code::invalid_timeout, 
		"TypeError: The connection timeout value must be a positive integer (including 0)."},
	{ xdevapi_exception::Code::timeout_exceeded, 
		"TimeoutError: Connection attempt to the server was aborted. Timeout was exceeded."},
	{ xdevapi_exception::Code::invalid_argument, "Invalid argument."},
};
/* }}} */

/* {{{ mysqlx::util::prepare_reason_msg */
string prepare_reason_msg(const string& /*sql_state*/, const string& msg)
{
	ostringstream os;
	os << '[' << general_sql_state << "] " << msg;
	const string& reason = os.str();
	return reason;
}
/* }}} */

} // anonymous namespace

//------------------------------------------------------------------------------

/* {{{ mysqlx::util::xdevapi_exception::xdevapi_exception */
xdevapi_exception::xdevapi_exception(Code code)
	: std::runtime_error(prepare_reason_msg(general_sql_state, code_to_err_msg.at(code)).c_str())
	, code(static_cast<unsigned int>(code))
{
}
/* }}} */

/* {{{ mysqlx::util::xdevapi_exception::xdevapi_exception */
xdevapi_exception::xdevapi_exception(Code code, const string& msg)
	: std::runtime_error(prepare_reason_msg(general_sql_state, msg).c_str())
	, code(static_cast<unsigned int>(code))
{
}
/* }}} */

/* {{{ mysqlx::util::xdevapi_exception::xdevapi_exception */
xdevapi_exception::xdevapi_exception(unsigned int code, const string& sql_state, const string& msg)
	: std::runtime_error(prepare_reason_msg(sql_state, msg).c_str())
	, code(code)
{
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ mysqlx::util::doc_ref_exception::doc_ref_exception */
doc_ref_exception::doc_ref_exception(Severity severity, _zend_class_entry* ce)
	: doc_ref_exception(severity, util::string("invalid object of class ") + ZSTR_VAL(ce->name))
{
}
/* }}} */

/* {{{ mysqlx::util::doc_ref_exception::doc_ref_exception */
doc_ref_exception::doc_ref_exception(Severity severity, const string& msg)
	: std::runtime_error(msg.c_str())
	, severity(severity)
{
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ mysqlx::util::raise_xdevapi_exception */
void raise_xdevapi_exception(const xdevapi_exception& e)
{
	const char* what = e.what();
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, e.code);
}
/* }}} */

/* {{{ mysqlx::util::raise_doc_ref_exception */
void raise_doc_ref_exception(const doc_ref_exception& e)
{
	static const std::map<doc_ref_exception::Severity, int> severity_mapping = {
		{ doc_ref_exception::Severity::warning, E_WARNING },
		{ doc_ref_exception::Severity::error, E_ERROR }
	};
	const int severity = severity_mapping.at(e.severity);
	const char* what = e.what();
	php_error_docref(nullptr, severity, "%s", what);
}
/* }}} */

/* {{{ mysqlx::util::raise_common_exception */
void raise_common_exception(const std::exception& e)
{
	const char* what = e.what();
	const int CommonExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, CommonExceptionCode);
}
/* }}} */

/* {{{ mysqlx::util::raise_unknown_exception */
void raise_unknown_exception()
{
	const char* what = "MySQL XDevAPI - unknown exception";
	const int UnknownExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, UnknownExceptionCode);
}
/* }}} */

/* {{{ mysqlx::util::log_warning */
void log_warning(const string& msg)
{
	php_error_docref(nullptr, E_WARNING, "%s", msg.c_str());
}
/* }}} */

} // namespace util

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
