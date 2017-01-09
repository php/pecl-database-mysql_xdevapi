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
#ifndef MYSQL_XDEVAPI_EXCEPTIONS_H
#define MYSQL_XDEVAPI_EXCEPTIONS_H

#include "strings.h"
#include <exception>

extern "C" {
struct _zend_class_entry;
}

namespace mysql
{

namespace php
{

struct xdevapi_exception : public std::exception
{
	xdevapi_exception(const unsigned int code, const string& sql_state, const string& msg);

	unsigned int code;
};

struct docref_exception : public std::exception
{
	enum class Kind
	{
		Strict,
		Warning,
		Error
	};

	docref_exception(const Kind kind, _zend_class_entry* ce);
	docref_exception(const Kind kind, const string& msg);

	Kind kind;
};

void throw_xdevapi_exception(const xdevapi_exception& e);
void throw_doc_ref_exception(const docref_exception& e);
void throw_common_exception(const std::exception& e);
void throw_unknown_exception();

} // namespace php

} // namespace mysql


#define MYSQL_XDEVAPI_TRY \
	try

#define MYSQL_XDEVAPI_CATCH \
	catch (const mysql::php::xdevapi_exception& e) { \
		mysql::php::throw_xdevapi_exception(e); \
	} catch (const mysql::php::docref_exception& e) { \
		mysql::php::throw_doc_ref_exception(e); \
	} catch (const std::exception& e) { \
		mysql::php::throw_common_exception(e); \
	} catch (...) { \
		mysql::php::throw_unknown_exception(); \
	}

#endif // MYSQL_XDEVAPI_EXCEPTIONS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
