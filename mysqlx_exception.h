/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_EXCEPTION_H
#define MYSQLX_EXCEPTION_H

#include "util/strings.h"

namespace mysqlx {

extern const char* GENERAL_SQL_STATE;

namespace devapi {

extern zend_class_entry * mysqlx_exception_class_entry;

void create_exception(int code, const util::string_view& sql_state, const util::string_view& msg);
void create_exception_ex(int code, const util::string_view& sql_state, const char* format, ...);

void mysqlx_register_exception_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers);
void mysqlx_unregister_exception_class(SHUTDOWN_FUNC_ARGS);

//What follows is a list of *general* error message, better to avoid continuos
//duplications of those messages and put them all here.

#define err_msg_invalid_prio_assignment 4000, "You must either assign no priority to any of the routers or give a priority for every router"
#define err_msg_all_routers_failed      4001, "All routers failed."
#define err_msg_prio_values_not_inrange 4007, "The priorities must be between 0 and 100"

#define err_msg_fetch_fail          10000, "Couldn't fetch data"
#define err_msg_meta_fail           10001, "Unable to extract metadata"
#define err_msg_add_doc             10002, "Error adding document"
#define err_msg_json_fail           10003, "Error serializing document to JSON"
#define err_msg_add_index_field_err 10004, "Error while adding an index field"
#define err_msg_add_orderby_fail    10005, "Error while adding a orderby expression"
#define err_msg_add_sort_fail       10006, "Error while adding a sort expression"
#define err_msg_add_where_fail      10007, "Error while adding a where expression"
#define err_msg_bind_fail           10008, "Error while binding a variable"
#define err_msg_merge_fail          10009, "Error while merging"
#define err_msg_unset_fail          10010, "Error while unsetting a variable"
#define err_msg_find_fail           10011, "Find not completely initialized"
#define err_msg_insert_fail         10012, "Insert not completely initialized"
#define err_msg_invalid_type        10013, "Invalid value type"
#define err_msg_modify_fail         10014, "Modify not completely initialized"
#define err_msg_wrong_param_1       10015, "Parameter must be an array of strings"
#define err_msg_wrong_param_2       10016, "Parameter must be a non-negative value"
#define err_msg_wrong_param_3       10017, "Parameter must be a string or array of strings"
#define err_msg_wrong_param_4       10018, "Parameter must be a string."
#define err_msg_add_field           10019, "Error while adding a fields list"
#define err_msg_delete_fail         10020, "Delete not completely initialized"
#define err_msg_update_fail         10021, "Update not completely initialized"
#define err_msg_createindex_fail    10022, "CreateIndex not completely initialized"
#define err_msg_dropindex_fail      10023, "DropIndex not completely initialized"
#define err_msg_arridx_del_fail     10024, "Error while deleting an array index"
#define err_msg_uri_string_fail     10025, "Incorrect URI string provided"
#define err_msg_new_session_fail    10026, "Unable to create a new session"
#define err_msg_internal_error      10027, "Internal error."
#define err_msg_invalid_identifier  10028, "Invalid identifier name provided!"
#define err_msg_wrong_param_5       10029, "Parameter must be a valid JSON."
#define err_msg_wrong_param_6       10032, "Invalid parameter or no parameter provided!"
#define err_msg_invalid_ssl_mode    10033, "Provided two different ssl-modes"
#define err_msg_tls_not_supported_1 10034, "TLS not supported with unix domain sockets."
#define err_msg_unexpected_doc_id   10035, "Unexpected document ID provided."
#define err_msg_inconsistent_ssl_options   10036, "Inconsistent ssl options."
#define err_msg_invalid_auth_mechanism     10037, "Invalid authorization mechanism"
#define err_msg_invalid_attrib_key         10038, "Connection attribute keys cannot start with '_'."
#define err_msg_invalid_attrib_key_size    10039, "Connection attribute keys cannot be longer than 32 characters"
#define err_msg_invalid_attrib_value_size  10039, "Connection attribute keys cannot be longer than 1024 characters"
#define err_msg_invalid_attrib_size        10040, "The connection attribute string is too long."
#define err_msg_invalid_compression_opt    10041, "The provided compression option is not recognized."
#define err_msg_compression_not_supported  10042, "Compression requested but the server does not support it."
#define err_msg_compres_negotiation_failed 10043, "Compression requested but the compression algorithm negotiation failed."

extern void RAISE_EXCEPTION(int errcode, const char* msg);

//This is a very common exception
#define RAISE_EXCEPTION_FETCH_FAIL() RAISE_EXCEPTION(err_msg_fetch_fail)

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_EXCEPTION_H */
