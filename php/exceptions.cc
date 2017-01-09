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

namespace mysql
{

namespace php
{

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

xdevapi_exception::xdevapi_exception(const unsigned int code_, const string& sql_state, const string& msg)
	: std::exception(prepare_reason_msg(sql_state, msg).c_str())
	, code(code_)
{
}

docref_exception::docref_exception(const Kind kind_, _zend_class_entry* ce)
	: docref_exception(kind_, php::string("invalid object of class ") + ZSTR_VAL(ce->name))
{
}

docref_exception::docref_exception(const Kind kind_, const string& msg)
	: std::exception(msg.c_str())
	, kind(kind_)
{
}

//------------------------------------------------------------------------------

void throw_xdevapi_exception(const xdevapi_exception& e)
{
	const char* what = e.what();
	zend_throw_exception(mysqlx_exception_class_entry, what, e.code);
}

void throw_doc_ref_exception(const docref_exception& e)
{
	static const std::map<docref_exception::Kind, int> kind_mapping = {
		{ docref_exception::Kind::Strict, E_STRICT },
		{ docref_exception::Kind::Warning, E_WARNING },
		{ docref_exception::Kind::Error, E_ERROR }
	};
	auto it = kind_mapping.find(e.kind);
	assert(it != kind_mapping.end());
	const int kind = it->second;
	const char* what = e.what();
	php_error_docref(nullptr, kind, what);
}

void throw_common_exception(const std::exception& e)
{
	const char* what = e.what();
	const int CommonExceptionCode = 0; //TODO
	zend_throw_exception(mysqlx_exception_class_entry, what, CommonExceptionCode);
}

void throw_unknown_exception()
{
	const char* what = "MySQL XDevAPI - unknown exception";
	const int UnknownExceptionCode = 0; //TODO
	zend_throw_exception(mysqlx_exception_class_entry, what, UnknownExceptionCode);
}

} // namespace php

} // namespace mysql

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
