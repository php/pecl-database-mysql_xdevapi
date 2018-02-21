/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#include "php_api.h"
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
}
#include "php_mysqlx.h"
#include "php_mysqlx_ex.h"
#include "mysqlx_crud_operation_bindable.h"
#include "mysqlx_crud_operation_limitable.h"
#include "mysqlx_crud_operation_skippable.h"
#include "mysqlx_crud_operation_sortable.h"
#include "mysqlx_database_object.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_driver.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_node_session.h"
//FILIP: #include "mysqlx_session.h"
#include "mysqlx_executable.h"
#include "mysqlx_exception.h"
#include "mysqlx_execution_status.h"
#include "mysqlx_expression.h"
#include "mysqlx_field_metadata.h"
#include "mysqlx_base_session.h"
#include "mysqlx_x_session.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_collection__add.h"
#include "mysqlx_node_collection__find.h"
#include "mysqlx_node_collection__modify.h"
#include "mysqlx_node_collection__remove.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_base_result.h"
#include "mysqlx_node_doc_result.h"
#include "mysqlx_node_result.h"
#include "mysqlx_node_row_result.h"
#include "mysqlx_node_sql_statement_result.h"
#include "mysqlx_node_column_result.h"
#include "mysqlx_node_table.h"
#include "mysqlx_node_table__delete.h"
#include "mysqlx_node_table__insert.h"
#include "mysqlx_node_table__select.h"
#include "mysqlx_node_table__update.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_object.h"
#include "mysqlx_warning.h"


#ifdef MYSQL_XDEVAPI_MESSAGE_CLASSES

#include "messages/mysqlx_node_connection.h"
#include "messages/mysqlx_node_pfc.h"

#include "messages/mysqlx_resultset__column_metadata.h"
#include "messages/mysqlx_resultset__resultset_metadata.h"
#include "messages/mysqlx_resultset__data_row.h"

#include "messages/mysqlx_message__ok.h"
#include "messages/mysqlx_message__error.h"
#include "messages/mysqlx_message__auth_start.h"
#include "messages/mysqlx_message__auth_continue.h"
#include "messages/mysqlx_message__auth_ok.h"
#include "messages/mysqlx_message__capabilities_get.h"
#include "messages/mysqlx_message__capabilities_set.h"
#include "messages/mysqlx_message__capability.h"
#include "messages/mysqlx_message__capabilities.h"
#include "messages/mysqlx_message__stmt_execute.h"
#include "messages/mysqlx_message__stmt_execute_ok.h"
#include "messages/mysqlx_message__data_fetch_done.h"

#endif

namespace mysqlx {

namespace devapi {

using namespace drv;

/* {{{ mysqlx_minit_classes */
PHP_MYSQL_XDEVAPI_API int
mysqlx_minit_classes(INIT_FUNC_ARGS)
{
	zend_object_handlers mysqlx_std_object_handlers = *zend_get_std_object_handlers();

	mysqlx_std_object_handlers.offset = XtOffsetOf(struct st_mysqlx_object, zo);
	mysqlx_std_object_handlers.free_obj = mysqlx_object_free_storage;
	mysqlx_std_object_handlers.clone_obj = nullptr;

	mysqlx_std_object_handlers.read_property = mysqlx_property_get_value;
	mysqlx_std_object_handlers.write_property = mysqlx_property_set_value;
	mysqlx_std_object_handlers.has_property = mysqlx_object_has_property;

	mysqlx_std_object_handlers.get_debug_info = mysqlx_object_get_debug_info;

	mysqlx_register_database_object_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_executable_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_schema_object_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_session_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_crud_operation_bindable_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_crud_operation_limitable_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_crud_operation_skippable_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_crud_operation_sortable_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_warning_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_exception_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_execution_status_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_expression_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_field_metadata_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_driver_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_base_session_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_x_session_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_session_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_schema_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_collection_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_collection__add_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_collection__find_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_collection__modify_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_collection__remove_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_statement_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_sql_statement_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_base_result_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_doc_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_row_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_sql_statement_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_column_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_table_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__delete_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__insert_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__select_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__update_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);


#if MYSQL_XDEVAPI_MESSAGE_CLASSES
	using namespace msg;

	mysqlx_register_node_connection_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_pfc_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__ok_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__error_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__capability_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__capabilities_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__capabilities_get_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__capabilities_set_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_message__auth_start_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__auth_continue_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__auth_ok_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_message__stmt_execute_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__stmt_execute_ok_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_column_metadata_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_resultset_metadata_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_data_row_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_message__data_fetch_done_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
#endif

	/* xmysqlnd_real_connect flags */
	REGISTER_LONG_CONSTANT("MYSQLX_CLIENT_SSL", CLIENT_SSL, CONST_CS | CONST_PERSISTENT);

	// column types
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_DECIMAL", FIELD_TYPE_DECIMAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_TINY", FIELD_TYPE_TINY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_SHORT", FIELD_TYPE_SHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_SMALLINT", FIELD_TYPE_SMALLINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_MEDIUMINT", FIELD_TYPE_MEDIUMINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_INT", FIELD_TYPE_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_BIGINT", FIELD_TYPE_BIGINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_LONG", FIELD_TYPE_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_FLOAT", FIELD_TYPE_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_DOUBLE", FIELD_TYPE_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_NULL", FIELD_TYPE_NULL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_TIMESTAMP", FIELD_TYPE_TIMESTAMP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_LONGLONG", FIELD_TYPE_LONGLONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_INT24", FIELD_TYPE_INT24, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_DATE", FIELD_TYPE_DATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_TIME", FIELD_TYPE_TIME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_DATETIME", FIELD_TYPE_DATETIME	, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_YEAR", FIELD_TYPE_YEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_NEWDATE", FIELD_TYPE_NEWDATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_ENUM", FIELD_TYPE_ENUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_SET", FIELD_TYPE_SET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_TINY_BLOB", FIELD_TYPE_TINY_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_MEDIUM_BLOB", FIELD_TYPE_MEDIUM_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_LONG_BLOB", FIELD_TYPE_LONG_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_BLOB", FIELD_TYPE_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_VAR_STRING", FIELD_TYPE_VAR_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_STRING", FIELD_TYPE_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_CHAR", FIELD_TYPE_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_BYTES", FIELD_TYPE_BYTES, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_INTERVAL", FIELD_TYPE_INTERVAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_GEOMETRY", FIELD_TYPE_GEOMETRY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_JSON", FIELD_TYPE_JSON, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_NEWDECIMAL", FIELD_TYPE_NEWDECIMAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLX_TYPE_BIT", FIELD_TYPE_BIT, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */


/* {{{ mysqlx_mshutdown_classes */
PHP_MYSQL_XDEVAPI_API int
mysqlx_mshutdown_classes(SHUTDOWN_FUNC_ARGS)
{
#ifdef MYSQL_XDEVAPI_MESSAGE_CLASSES
	using namespace msg;

	mysqlx_unregister_message__data_fetch_done_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_data_row_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_resultset_metadata_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_column_metadata_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_message__stmt_execute_ok_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__stmt_execute_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_message__auth_ok_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__auth_continue_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__auth_start_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_message__capabilities_set_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__capabilities_get_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__capabilities_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__capability_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__error_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_message__ok_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_pfc_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_connection_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
#endif

	mysqlx_unregister_node_table__update_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_table__select_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_table__insert_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_table__delete_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_table_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_sql_statement_result_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_row_result_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_result_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_doc_result_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_base_result_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_sql_statement_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_statement_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_column_result_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_node_collection_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_collection__add_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_collection__find_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_collection__modify_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_collection__remove_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_node_schema_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_session_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_x_session_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_base_session_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_driver_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_field_metadata_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_expression_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_execution_status_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_exception_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_warning_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_crud_operation_sortable_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_crud_operation_limitable_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_crud_operation_skippable_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_crud_operation_bindable_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_schema_object_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_session_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_executable_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_database_object_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

