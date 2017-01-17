/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
extern "C"
{
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

string prepare_reason_msg(const string& sql_state, const string& msg)
{
	ostringstream os;
	os << '[' << sql_state << "] " << msg;
	const string& reason = os.str();
	return reason;
}

} // anonymous namespace

xdevapi_exception::xdevapi_exception(unsigned int code, const string& sql_state, const string& msg)
	: std::exception(prepare_reason_msg(sql_state, msg).c_str())
	, code(code)
{
}

doc_ref_exception::doc_ref_exception(Severity severity, _zend_class_entry* ce)
	: doc_ref_exception(severity, phputils::string("invalid object of class ") + ZSTR_VAL(ce->name))
{
}

doc_ref_exception::doc_ref_exception(Severity severity, const string& msg)
	: std::exception(msg.c_str())
	, severity(severity)
{
}

//------------------------------------------------------------------------------

void raise_xdevapi_exception(const xdevapi_exception& e)
{
	const char* what = e.what();
	zend_throw_exception(mysqlx_exception_class_entry, what, e.code);
}

void raise_doc_ref_exception(const doc_ref_exception& e)
{
	static const std::map<doc_ref_exception::Severity, int> severity_mapping = {
		{ doc_ref_exception::Severity::warning, E_WARNING },
		{ doc_ref_exception::Severity::error, E_ERROR }
	};
	auto it = severity_mapping.find(e.severity);
	assert(it != severity_mapping.end());
	const int severity = it->second;
	const char* what = e.what();
	php_error_docref(nullptr, severity, what);
}

void raise_common_exception(const std::exception& e)
{
	const char* what = e.what();
	const int CommonExceptionCode = 0; //TODO
	zend_throw_exception(mysqlx_exception_class_entry, what, CommonExceptionCode);
}

void raise_unknown_exception()
{
	const char* what = "MySQL XDevAPI - unknown exception";
	const int UnknownExceptionCode = 0; //TODO
	zend_throw_exception(mysqlx_exception_class_entry, what, UnknownExceptionCode);
}

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
