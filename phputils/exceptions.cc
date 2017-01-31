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
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "exceptions.h"
#include "mysqlx_exception.h"
#include "types.h"

namespace mysqlx {

namespace phputils {

namespace
{

const char* const general_sql_state = GENERAL_SQL_STATE;

/* {{{ mysqlx::phputils::code_to_err_msg */
const std::map<xdevapi_exception::Code, std::pair<unsigned int, const char* const>> code_to_err_msg = {
	{ xdevapi_exception::Code::fetch_fail, { 10000, "Coulnd't fetch data" }},
	{ xdevapi_exception::Code::meta_fail, { 10001, "Unable to extract metadata" }},
	{ xdevapi_exception::Code::add_doc, { 10002, "Error adding document" }},
	{ xdevapi_exception::Code::json_fail, { 10003, "Error serializing document to JSON" }},
	{ xdevapi_exception::Code::add_index_field_err, { 10004, "Error while adding an index field" }},
	{ xdevapi_exception::Code::add_orderby_fail, { 10005, "Error while adding a orderby expression" }},
	{ xdevapi_exception::Code::add_sort_fail, { 10006, "Error while adding a sort expression" }},
	{ xdevapi_exception::Code::add_where_fail, { 10007, "Error while adding a where expression" }},
	{ xdevapi_exception::Code::bind_fail, { 10008, "Error while binding a variable" }},
	{ xdevapi_exception::Code::merge_fail, { 10009, "Error while merging" }},
	{ xdevapi_exception::Code::unset_fail, { 10010, "Error while unsetting a variable" }},
	{ xdevapi_exception::Code::find_fail, { 10011, "Find not completely initialized" }},
	{ xdevapi_exception::Code::insert_fail, { 10012, "Insert not completely initialized" }},
	{ xdevapi_exception::Code::invalid_type, { 10013, "Invalid value type" }},
	{ xdevapi_exception::Code::modify_fail, { 10014, "Modify not completely initialized" }},
	{ xdevapi_exception::Code::wrong_param_1, { 10015, "Parameter must be an array of strings" }},
	{ xdevapi_exception::Code::wrong_param_2, { 10016, "Parameter must be a non-negative value" }},
	{ xdevapi_exception::Code::wrong_param_3, { 10017, "Parameter must be a string or array of strings" }},
	{ xdevapi_exception::Code::wrong_param_4, { 10018, "Parameter must be a string." }},
	{ xdevapi_exception::Code::add_field, { 10019, "Error while adding a fields list" }},
	{ xdevapi_exception::Code::delete_fail, { 10020, "Delete not completely initialized" }},
	{ xdevapi_exception::Code::update_fail, { 10021, "Update not completely initialized" }},
	{ xdevapi_exception::Code::create_index_fail, { 10022, "CreateIndex not completely initialized" }},
	{ xdevapi_exception::Code::drop_index_fail, { 10023, "DropIndex not completely initialized" }},
	{ xdevapi_exception::Code::arridx_del_fail, { 10024, "Error while deleting an array index" }},
};
/* }}} */

/* {{{ mysqlx::phputils::prepare_reason_msg */
string prepare_reason_msg(const string& sql_state, const string& msg)
{
	ostringstream os;
	os << '[' << general_sql_state << "] " << msg;
	const string& reason = os.str();
	return reason;
}
/* }}} */

} // anonymous namespace

//------------------------------------------------------------------------------

/* {{{ mysqlx::phputils::xdevapi_exception::xdevapi_exception */
xdevapi_exception::xdevapi_exception(Code code)
	: std::runtime_error(prepare_reason_msg(general_sql_state, code_to_err_msg.at(code).second).c_str())
	, code(code_to_err_msg.at(code).first)
{
}
/* }}} */

/* {{{ mysqlx::phputils::xdevapi_exception::xdevapi_exception */
xdevapi_exception::xdevapi_exception(unsigned int code, const string& sql_state, const string& msg)
	: std::runtime_error(prepare_reason_msg(sql_state, msg).c_str())
	, code(code)
{
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ mysqlx::phputils::doc_ref_exception::doc_ref_exception */
doc_ref_exception::doc_ref_exception(Severity severity, _zend_class_entry* ce)
	: doc_ref_exception(severity, phputils::string("invalid object of class ") + ZSTR_VAL(ce->name))
{
}
/* }}} */

/* {{{ mysqlx::phputils::doc_ref_exception::doc_ref_exception */
doc_ref_exception::doc_ref_exception(Severity severity, const string& msg)
	: std::runtime_error(msg.c_str())
	, severity(severity)
{
}
/* }}} */

//------------------------------------------------------------------------------

/* {{{ mysqlx::phputils::raise_xdevapi_exception */
void raise_xdevapi_exception(const xdevapi_exception& e)
{
	const char* what = e.what();
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, e.code);
}
/* }}} */

/* {{{ mysqlx::phputils::raise_doc_ref_exception */
void raise_doc_ref_exception(const doc_ref_exception& e)
{
	static const std::map<doc_ref_exception::Severity, int> severity_mapping = {
		{ doc_ref_exception::Severity::warning, E_WARNING },
		{ doc_ref_exception::Severity::error, E_ERROR }
	};
	const int severity = severity_mapping.at(e.severity);
	const char* what = e.what();
	php_error_docref(nullptr, severity, what);
}
/* }}} */

/* {{{ mysqlx::phputils::raise_common_exception */
void raise_common_exception(const std::exception& e)
{
	const char* what = e.what();
	const int CommonExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, CommonExceptionCode);
}
/* }}} */

/* {{{ mysqlx::phputils::raise_unknown_exception */
void raise_unknown_exception()
{
	const char* what = "MySQL XDevAPI - unknown exception";
	const int UnknownExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, UnknownExceptionCode);
}
/* }}} */

} // namespace phputils

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
