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
#ifndef MYSQL_XDEVAPI_PHPUTILS_EXCEPTIONS_H
#define MYSQL_XDEVAPI_PHPUTILS_EXCEPTIONS_H

#include "strings.h"
#include <exception>

extern "C" {
struct _zend_class_entry;
}

namespace mysqlx {

namespace phputils {

/* {{{ mysqlx::phputils::xdevapi_exception */
struct xdevapi_exception : public std::runtime_error
{
	enum class Code : unsigned int
	{
		not_implemented = 1000,
		fetch_fail = 10000,
		meta_fail,
		add_doc,
		json_fail,
		add_index_field_err,
		add_orderby_fail,
		add_sort_fail,
		add_where_fail,
		bind_fail,
		merge_fail,
		unset_fail,
		find_fail,
		insert_fail,
		invalid_type,
		modify_fail,
		wrong_param_1,
		wrong_param_2,
		wrong_param_3,
		wrong_param_4,
		add_field,
		delete_fail,
		update_fail,
		create_index_fail,
		drop_index_fail,
		arridx_del_fail,
		view_create_fail,
		view_alter_fail,
		view_drop_fail,
		invalid_view_algorithm,
		invalid_view_security,
		invalid_view_check_option,
		invalid_view_columns,
		invalid_view_defined_as,
		unsupported_conversion_to_string,
		unsupported_default_value_type,
		create_table_fail,
		invalid_table_column,
		unknown_table_column_type,
		invalid_foreign_key,
		unknown_fkey_change_mode,
	};

	xdevapi_exception(Code code);
	xdevapi_exception(unsigned int code, const string& sql_state, const string& msg);

	unsigned int code;
};
/* }}} */

/* {{{ mysqlx::phputils::doc_ref_exception */
struct doc_ref_exception : public std::runtime_error
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
/* }}} */

void raise_xdevapi_exception(const xdevapi_exception& e);
void raise_doc_ref_exception(const doc_ref_exception& e);
void raise_common_exception(const std::exception& e);
void raise_unknown_exception();

} // namespace phputils

} // namespace mysqlx


#define MYSQL_XDEVAPI_TRY \
	try

#define MYSQL_XDEVAPI_CATCH \
	catch (const phputils::xdevapi_exception& e) { \
		phputils::raise_xdevapi_exception(e); \
	} catch (const phputils::doc_ref_exception& e) { \
		phputils::raise_doc_ref_exception(e); \
	} catch (const std::exception& e) { \
		phputils::raise_common_exception(e); \
	} catch (...) { \
		phputils::raise_unknown_exception(); \
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
