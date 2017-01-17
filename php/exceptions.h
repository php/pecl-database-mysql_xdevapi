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

namespace mysqlx {

namespace phputils {

struct xdevapi_exception : public std::exception
{
	xdevapi_exception(unsigned int code, const string& sql_state, const string& msg);

	unsigned int code;
};

struct doc_ref_exception : public std::exception
{
	enum class Severity
	{
		warning,
		error
	};

	doc_ref_exception(Severity severity, _zend_class_entry* ce);
	doc_ref_exception(Severity severity, const string& msg);

	Severity severity;
};

void raise_xdevapi_exception(const xdevapi_exception& e);
void raise_doc_ref_exception(const doc_ref_exception& e);
void raise_common_exception(const std::exception& e);
void raise_unknown_exception();

} // namespace phputils

} // namespace mysqlx


#define MYSQL_XDEVAPI_TRY \
	try

#define MYSQL_XDEVAPI_CATCH \
	catch (const mysqlx::phputils::xdevapi_exception& e) { \
		mysqlx::phputils::raise_xdevapi_exception(e); \
	} catch (const mysqlx::phputils::doc_ref_exception& e) { \
		mysqlx::phputils::raise_doc_ref_exception(e); \
	} catch (const std::exception& e) { \
		mysqlx::phputils::raise_common_exception(e); \
	} catch (...) { \
		mysqlx::phputils::raise_unknown_exception(); \
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
