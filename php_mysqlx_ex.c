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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "php_mysqlx.h"
#include "mysqlx_database_object.h"
#include "mysqlx_schema_object.h"
#include "mysqlx_driver.h"
#include "mysqlx_session.h"
#include "mysqlx_exception.h"
#include "mysqlx_execution_status.h"
#include "mysqlx_field_metadata.h"
#include "mysqlx_node_session.h"
#include "mysqlx_node_schema.h"
#include "mysqlx_node_collection.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_sql_statement_result.h"
#include "mysqlx_node_table.h"
#include "mysqlx_node_table__delete.h"
#include "mysqlx_node_table__insert.h"
#include "mysqlx_node_table__select.h"
#include "mysqlx_node_table__update.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_object.h"
#include "mysqlx_warning.h"


#ifdef MYSQLX_MESSAGE_CLASSES

#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"

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
#include "mysqlx_resultset__column_metadata.h"
#include "mysqlx_resultset__resultset_metadata.h"
#include "mysqlx_resultset__data_row.h"
#include "messages/mysqlx_message__data_fetch_done.h"

#endif


/* {{{ mysqlx_minit_classes */
PHPAPI int
mysqlx_minit_classes(INIT_FUNC_ARGS)
{
	zend_object_handlers mysqlx_std_object_handlers = *zend_get_std_object_handlers();

	mysqlx_std_object_handlers.offset = XtOffsetOf(struct st_mysqlx_object, zo);
	mysqlx_std_object_handlers.free_obj = mysqlx_object_free_storage;
	mysqlx_std_object_handlers.clone_obj = NULL;

	mysqlx_std_object_handlers.read_property = mysqlx_property_get_value;
	mysqlx_std_object_handlers.write_property = mysqlx_property_set_value;
	mysqlx_std_object_handlers.has_property = mysqlx_object_has_property;

	mysqlx_std_object_handlers.get_debug_info = mysqlx_object_get_debug_info;

	mysqlx_register_session_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_database_object_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_schema_object_interface(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_warning_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_exception_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_execution_status_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_field_metadata_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_driver_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_session_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_schema_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_collection_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_sql_statement_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_sql_statement_result_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

	mysqlx_register_node_table_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__delete_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__insert_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__select_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);
	mysqlx_register_node_table__update_class(INIT_FUNC_ARGS_PASSTHRU, &mysqlx_std_object_handlers);

#if MYSQLX_MESSAGE_CLASSES
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
	REGISTER_LONG_CONSTANT("XMYSQLND_CLIENT_SSL", CLIENT_SSL, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */


/* {{{ mysqlx_mshutdown_classes */
PHPAPI int
mysqlx_mshutdown_classes(SHUTDOWN_FUNC_ARGS)
{
#ifdef MYSQLX_MESSAGE_CLASSES
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
	mysqlx_unregister_node_sql_statement_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_collection_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_schema_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_node_session_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_driver_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_field_metadata_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_execution_status_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_exception_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_warning_class(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	mysqlx_unregister_schema_object_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_database_object_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);
	mysqlx_unregister_session_interface(SHUTDOWN_FUNC_ARGS_PASSTHRU);

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

