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
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {
#include <zend_exceptions.h>
}
#include "exceptions.h"
#include "mysqlx_exception.h"
#include "types.h"
#include "string_utils.h"

namespace mysqlx {

namespace util {

namespace
{

constexpr string_view General_sql_state{ "HY000" };
constexpr string_view Unknown_error_message{ "Unknown error" };

const std::map<xdevapi_exception::Code, const char* const> code_to_err_msg{
	{ xdevapi_exception::Code::fetch_fail, "Couldn't fetch data" },
	{ xdevapi_exception::Code::meta_fail, "Unable to extract metadata" },
	{ xdevapi_exception::Code::add_doc, "Error adding document" },
	{ xdevapi_exception::Code::json_fail, "Error serializing document to JSON, code:" },
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
	{ xdevapi_exception::Code::table_creation_failed, "Unable to create the table object" },
	{ xdevapi_exception::Code::invalid_timeout,
		"TypeError: The connection timeout value must be a positive integer (including 0)." },
	{ xdevapi_exception::Code::timeout_exceeded,
		"TimeoutError: Connection attempt to the server was aborted. Timeout was exceeded." },
	{ xdevapi_exception::Code::invalid_argument, "Invalid argument." },
	{ xdevapi_exception::Code::connection_failure, "Connection failure." },
	{ xdevapi_exception::Code::authentication_failure, "Authentication failure." },
	{ xdevapi_exception::Code::runtime_error, "Run-time error." },
	{ xdevapi_exception::Code::session_closed, "Session closed." },
	{ xdevapi_exception::Code::offset_without_limit_not_allowed,
		"The use of 'offset' without 'limit' is not allowed" },
	{ xdevapi_exception::Code::ps_unknown_message,
		"Attempting to prepare a statement for an unknown message!" },
	{ xdevapi_exception::Code::ps_limit_not_supported,
		"Limit not supported for this message, prepared statement failed!" },
	{ xdevapi_exception::Code::session_reset_failure, "Session reset failure." },
	{ xdevapi_exception::Code::conn_attrib_wrong_type,
		"The value of \"connection-attributes\" must be either a boolean or a list of key-value pairs" },
	{ xdevapi_exception::Code::conn_attrib_dup_key,
		"Duplicate key used in the \"connection-attributes\" option." },
	{ xdevapi_exception::Code::unknown_client_conn_option,
		"Unknown client connection option in Uri:" },
	{ xdevapi_exception::Code::unknown_ssl_mode, "Unknown SSL mode:" },
	{ xdevapi_exception::Code::unknown_tls_version, "Unknown TLS version:" },
	{ xdevapi_exception::Code::openssl_unavailable,
		"trying to setup secure connection while OpenSSL is not available" },
	{ xdevapi_exception::Code::empty_tls_versions,
		"at least one TLS protocol version must be specified in tls-versions list" },
	{ xdevapi_exception::Code::cannot_connect_by_ssl, "Cannot connect to MySQL by using SSL" },
	{ xdevapi_exception::Code::cannot_setup_tls,
		"Negative response from the server, not able to setup TLS." },
	{ xdevapi_exception::Code::no_valid_cipher_in_list,
		"No valid cipher found in the ssl ciphers list." },
	{ xdevapi_exception::Code::no_valid_ciphersuite_in_list,
		"No valid cipher suite found in the tls ciphersuites list." },
	{ xdevapi_exception::Code::port_nbr_not_allowed_with_srv_uri,
		"Specifying a port number with DNS SRV lookup is not allowed" },
	{ xdevapi_exception::Code::provided_invalid_uri,
		"Incorrect URI string provided" },
	{ xdevapi_exception::Code::unix_socket_not_allowed_with_srv,
		"Using Unix domain sockets with DNS SRV lookup is not allowed" },
	{ xdevapi_exception::Code::url_list_not_allowed,
		"URI with a list of URL not allowed." },
	{ xdevapi_exception::Code::out_of_range, "out of range" },
	{ xdevapi_exception::Code::json_parse_error, "json parse error, code:" },
	{ xdevapi_exception::Code::compression_not_supported,
		"Compression requested but the server does not support it." },
	{ xdevapi_exception::Code::compressor_not_available,
		"To enable given compression algorithm please build mysql_xdevapi with proper switch like --with-(zlib|lz4|zstd)=[DIR]." },
	{ xdevapi_exception::Code::compression_negotiation_failure,
		"Compression requested but the compression algorithm negotiation failed." },
	{ xdevapi_exception::Code::compression_invalid_algorithm_name, "Invalid algorithm name:" },
	{ xdevapi_exception::Code::object_property_not_exist,
		"Object property doesn't exist:" },
	{ xdevapi_exception::Code::object_property_invalid_type,
		"Invalid type of object property, expected type is:" },
	{ xdevapi_exception::Code::json_object_expected,
		"Invalid format of document, JSON object expected: " },
	{ xdevapi_exception::Code::json_array_expected,
		"Invalid format of document, JSON array expected: " },
};

string_view to_sql_state(const string_view& sql_state)
{
	return sql_state.empty() ? General_sql_state : sql_state;
}

string to_error_msg(xdevapi_exception::Code code, const string_view& what)
{
	string msg;
	auto it{ code_to_err_msg.find(code) };
	if (it != code_to_err_msg.end()) {
		msg = it->second;
	}

	if (!what.empty()) {
		if (!msg.empty()) msg += ' ';
		msg += what;
	}

	return msg.empty() ? string{Unknown_error_message} : msg;
}

string to_error_msg(unsigned int code, const string_view& what)
{
	return to_error_msg(static_cast<xdevapi_exception::Code>(code), what);
}

} // anonymous namespace

//------------------------------------------------------------------------------

xdevapi_exception::xdevapi_exception(Code code)
	: xdevapi_exception(code, string_view{})
{
}

xdevapi_exception::xdevapi_exception(Code code, int error_number)
	: xdevapi_exception(code, std::to_string(error_number))
{
}

xdevapi_exception::xdevapi_exception(Code code, const string_view& msg)
	: xdevapi_exception(static_cast<int>(code), General_sql_state, msg)
{
}

xdevapi_exception::xdevapi_exception(unsigned int code, const string_view& sql_state, const string_view& msg)
	: std::runtime_error(prepare_reason_msg(code, sql_state, msg).c_str())
	, code(code)
{
}

//------------------------------------------------------------------------------

doc_ref_exception::doc_ref_exception(Severity severity, _zend_class_entry* ce)
	: doc_ref_exception(severity, util::string("invalid object of class ") + ZSTR_VAL(ce->name))
{
}

doc_ref_exception::doc_ref_exception(Severity severity, const string& msg)
	: std::runtime_error(msg.c_str())
	, severity(severity)
{
}

//------------------------------------------------------------------------------

void raise_xdevapi_exception(const xdevapi_exception& e)
{
	const char* what = e.what();
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, e.code);
}

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

void raise_common_exception(const std::exception& e)
{
	const char* what = e.what();
	const int CommonExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, CommonExceptionCode);
}

void raise_unknown_exception()
{
	const char* what = "MySQL XDevAPI - unknown exception";
	const int UnknownExceptionCode = 0; //TODO
	zend_throw_exception(devapi::mysqlx_exception_class_entry, what, UnknownExceptionCode);
}

//------------------------------------------------------------------------------

string prepare_reason_msg(unsigned int code, const string_view& sql_state, const string_view& what)
{
	ostringstream os;
	os << '[' << code << "][" << to_sql_state(sql_state) << "] " << to_error_msg(code, what);
	const string& reason = os.str();
	return reason;
}

void log_warning(const string& msg)
{
	php_error_docref(nullptr, E_WARNING, "%s", msg.c_str());
}

void set_error_info(util::xdevapi_exception::Code code, MYSQLND_ERROR_INFO* error_info)
{
	error_info->error_no = static_cast<unsigned int>(code);
	strcpy(error_info->sqlstate, General_sql_state.data());
	error_info->error[0] = 0;
}

} // namespace util

} // namespace mysqlx
