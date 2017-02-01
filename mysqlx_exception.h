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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_EXCEPTION_H
#define MYSQLX_EXCEPTION_H

namespace mysqlx {

namespace devapi {

extern zend_class_entry * mysqlx_exception_class_entry;

void mysqlx_new_exception(const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING msg);
void mysqlx_new_exception_ex(const unsigned int code, const MYSQLND_CSTRING sql_state, const char * const format, ...);

void mysqlx_register_exception_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers);
void mysqlx_unregister_exception_class(SHUTDOWN_FUNC_ARGS);

#define GENERAL_SQL_STATE "HY000" //Same as for the server

//What follows is a list of *general* error message, better to avoid continuos
//duplications of those messages and put them all here.

#define err_msg_fetch_fail          10000, "Coulnd't fetch data"
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

extern void RAISE_EXCEPTION(const int errcode, const char * const msg);

//This is a very common exception
#define RAISE_EXCEPTION_FETCH_FAIL() RAISE_EXCEPTION(err_msg_fetch_fail)

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_EXCEPTION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
