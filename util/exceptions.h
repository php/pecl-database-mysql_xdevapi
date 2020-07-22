/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#ifndef MYSQL_XDEVAPI_UTIL_EXCEPTIONS_H
#define MYSQL_XDEVAPI_UTIL_EXCEPTIONS_H

#include "strings.h"
#include <exception>

extern "C" {
struct _zend_class_entry;
struct st_mysqlnd_error_info;
}

namespace mysqlx {

namespace util {

struct xdevapi_exception : public std::runtime_error
{
	enum class Code : unsigned int
	{
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
		remove_fail,
		wrong_param_1,
		wrong_param_2,
		wrong_param_3,
		wrong_param_4,
		wrong_param_string_or_strings,
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
		invalid_table_column_length,
		invalid_table_column_length_decimals,
		invalid_foreign_key,
		unknown_fkey_change_mode,
		invalid_identifier,
		inconsistent_ssl_options,
		invalid_auth_mechanism,
		unknown_lock_waiting_option,
		schema_creation_failed,
		table_creation_failed,
		invalid_timeout,
		timeout_exceeded,
		invalid_argument,
		connection_failure,
		authentication_failure,
		runtime_error,
		session_closed,
		offset_without_limit_not_allowed,
		ps_unknown_message,
		ps_limit_not_supported,
		session_reset_failure,
		conn_attrib_wrong_type,
		conn_attrib_dup_key,
		unknown_client_conn_option,
		unknown_ssl_mode,
		unknown_tls_version,
		openssl_unavailable,
		empty_tls_versions,
		cannot_connect_by_ssl,
		cannot_setup_tls,
		no_valid_cipher_in_list,
		no_valid_ciphersuite_in_list,
		port_nbr_not_allowed_with_srv_uri,
		provided_invalid_uri,
		unix_socket_not_allowed_with_srv,
		url_list_not_allowed,
		out_of_range,
		json_parse_error,
		compression_not_supported,
		compressor_not_available,
		compression_negotiation_failure,
		compression_invalid_algorithm_name,
		object_property_not_exist,
		object_property_invalid_type,
		json_object_expected,
		json_array_expected,
	};

	xdevapi_exception(Code code);
	xdevapi_exception(Code code, int error_number);
	xdevapi_exception(Code code, const string_view& msg);
	xdevapi_exception(unsigned int code, const string_view& sql_state, const string_view& msg);

	unsigned int code;
};

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

void raise_xdevapi_exception(const xdevapi_exception& e);
void raise_doc_ref_exception(const doc_ref_exception& e);
void raise_common_exception(const std::exception& e);
void raise_unknown_exception();

string prepare_reason_msg(unsigned int code, const string_view& sql_state, const string_view& what);
void log_warning(const string& msg);

void set_error_info(
	util::xdevapi_exception::Code code,
	st_mysqlnd_error_info* error_info);

} // namespace util

} // namespace mysqlx


#define MYSQL_XDEVAPI_TRY \
	try

#define MYSQL_XDEVAPI_CATCH \
	catch (const util::xdevapi_exception& e) { \
		util::raise_xdevapi_exception(e); \
	} catch (const util::doc_ref_exception& e) { \
		util::raise_doc_ref_exception(e); \
	} catch (const std::exception& e) { \
		util::raise_common_exception(e); \
	} catch (...) { \
		util::raise_unknown_exception(); \
	}

#endif // MYSQL_XDEVAPI_UTIL_EXCEPTIONS_H
