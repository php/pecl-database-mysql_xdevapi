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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_session.h"
#include "php_mysql_xdevapi.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace drv {

static zend_bool xmysqlnd_library_initted = FALSE;

/*
	TODO marines
	copied from
	ext/mysqlnd/mysqlnd_statistics.c
	needed due to optional shared (so/dll) build
*/
static const MYSQLND_STRING xmysqlnd_stats_values_names[STAT_LAST] =
{
	{ util::literal_to_mysqlnd_str("bytes_sent") },
	{ util::literal_to_mysqlnd_str("bytes_received") },
	{ util::literal_to_mysqlnd_str("packets_sent") },
	{ util::literal_to_mysqlnd_str("packets_received") },
	{ util::literal_to_mysqlnd_str("protocol_overhead_in") },
	{ util::literal_to_mysqlnd_str("protocol_overhead_out") },
	{ util::literal_to_mysqlnd_str("bytes_received_ok_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_eof_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_rset_header_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_rset_field_meta_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_rset_row_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_prepare_response_packet") },
	{ util::literal_to_mysqlnd_str("bytes_received_change_user_packet") },
	{ util::literal_to_mysqlnd_str("packets_sent_command") },
	{ util::literal_to_mysqlnd_str("packets_received_ok") },
	{ util::literal_to_mysqlnd_str("packets_received_eof") },
	{ util::literal_to_mysqlnd_str("packets_received_rset_header") },
	{ util::literal_to_mysqlnd_str("packets_received_rset_field_meta") },
	{ util::literal_to_mysqlnd_str("packets_received_rset_row") },
	{ util::literal_to_mysqlnd_str("packets_received_prepare_response") },
	{ util::literal_to_mysqlnd_str("packets_received_change_user") },
	{ util::literal_to_mysqlnd_str("result_set_queries") },
	{ util::literal_to_mysqlnd_str("non_result_set_queries") },
	{ util::literal_to_mysqlnd_str("no_index_used") },
	{ util::literal_to_mysqlnd_str("bad_index_used") },
	{ util::literal_to_mysqlnd_str("slow_queries") },
	{ util::literal_to_mysqlnd_str("buffered_sets") },
	{ util::literal_to_mysqlnd_str("unbuffered_sets") },
	{ util::literal_to_mysqlnd_str("ps_buffered_sets") },
	{ util::literal_to_mysqlnd_str("ps_unbuffered_sets") },
	{ util::literal_to_mysqlnd_str("flushed_normal_sets") },
	{ util::literal_to_mysqlnd_str("flushed_ps_sets") },
	{ util::literal_to_mysqlnd_str("ps_prepared_never_executed") },
	{ util::literal_to_mysqlnd_str("ps_prepared_once_executed") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_server_normal") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_server_ps") },
	{ util::literal_to_mysqlnd_str("rows_buffered_from_client_normal") },
	{ util::literal_to_mysqlnd_str("rows_buffered_from_client_ps") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_client_normal_buffered") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_client_normal_unbuffered") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_client_ps_buffered") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_client_ps_unbuffered") },
	{ util::literal_to_mysqlnd_str("rows_fetched_from_client_ps_cursor") },
	{ util::literal_to_mysqlnd_str("rows_affected_normal") },
	{ util::literal_to_mysqlnd_str("rows_affected_ps") },
	{ util::literal_to_mysqlnd_str("rows_skipped_normal") },
	{ util::literal_to_mysqlnd_str("rows_skipped_ps") },
	{ util::literal_to_mysqlnd_str("copy_on_write_saved") },
	{ util::literal_to_mysqlnd_str("copy_on_write_performed") },
	{ util::literal_to_mysqlnd_str("command_buffer_too_small") },
	{ util::literal_to_mysqlnd_str("connect_success") },
	{ util::literal_to_mysqlnd_str("connect_failure") },
	{ util::literal_to_mysqlnd_str("connection_reused") },
	{ util::literal_to_mysqlnd_str("reconnect") },
	{ util::literal_to_mysqlnd_str("pconnect_success") },
	{ util::literal_to_mysqlnd_str("active_connections") },
	{ util::literal_to_mysqlnd_str("active_persistent_connections") },
	{ util::literal_to_mysqlnd_str("explicit_close") },
	{ util::literal_to_mysqlnd_str("implicit_close") },
	{ util::literal_to_mysqlnd_str("disconnect_close") },
	{ util::literal_to_mysqlnd_str("in_middle_of_command_close") },
	{ util::literal_to_mysqlnd_str("explicit_free_result") },
	{ util::literal_to_mysqlnd_str("implicit_free_result") },
	{ util::literal_to_mysqlnd_str("explicit_stmt_close") },
	{ util::literal_to_mysqlnd_str("implicit_stmt_close") },
	{ util::literal_to_mysqlnd_str("mem_emalloc_count") },
	{ util::literal_to_mysqlnd_str("mem_emalloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_ecalloc_count") },
	{ util::literal_to_mysqlnd_str("mem_ecalloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_erealloc_count") },
	{ util::literal_to_mysqlnd_str("mem_erealloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_efree_count") },
	{ util::literal_to_mysqlnd_str("mem_efree_amount") },
	{ util::literal_to_mysqlnd_str("mem_malloc_count") },
	{ util::literal_to_mysqlnd_str("mem_malloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_calloc_count") },
	{ util::literal_to_mysqlnd_str("mem_calloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_realloc_count") },
	{ util::literal_to_mysqlnd_str("mem_realloc_amount") },
	{ util::literal_to_mysqlnd_str("mem_free_count") },
	{ util::literal_to_mysqlnd_str("mem_free_amount") },
	{ util::literal_to_mysqlnd_str("mem_estrndup_count") },
	{ util::literal_to_mysqlnd_str("mem_strndup_count") },
	{ util::literal_to_mysqlnd_str("mem_estrdup_count") },
	{ util::literal_to_mysqlnd_str("mem_strdup_count") },
	{ util::literal_to_mysqlnd_str("mem_edupl_count") },
	{ util::literal_to_mysqlnd_str("mem_dupl_count") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_null") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_bit") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_tinyint") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_short") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_int24") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_int") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_bigint") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_decimal") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_float") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_double") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_date") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_year") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_time") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_datetime") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_timestamp") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_string") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_blob") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_enum") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_set") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_geometry") },
	{ util::literal_to_mysqlnd_str("proto_text_fetched_other") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_null") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_bit") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_tinyint") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_short") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_int24") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_int") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_bigint") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_decimal") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_float") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_double") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_date") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_year") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_time") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_datetime") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_timestamp") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_string") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_json") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_blob") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_enum") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_set") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_geometry") },
	{ util::literal_to_mysqlnd_str("proto_binary_fetched_other") },
	{ util::literal_to_mysqlnd_str("init_command_executed_count") },
	{ util::literal_to_mysqlnd_str("init_command_failed_count") },
	{ util::literal_to_mysqlnd_str("com_quit") },
	{ util::literal_to_mysqlnd_str("com_init_db") },
	{ util::literal_to_mysqlnd_str("com_query") },
	{ util::literal_to_mysqlnd_str("com_field_list") },
	{ util::literal_to_mysqlnd_str("com_create_db") },
	{ util::literal_to_mysqlnd_str("com_drop_db") },
	{ util::literal_to_mysqlnd_str("com_refresh") },
	{ util::literal_to_mysqlnd_str("com_shutdown") },
	{ util::literal_to_mysqlnd_str("com_statistics") },
	{ util::literal_to_mysqlnd_str("com_process_info") },
	{ util::literal_to_mysqlnd_str("com_connect") },
	{ util::literal_to_mysqlnd_str("com_process_kill") },
	{ util::literal_to_mysqlnd_str("com_debug") },
	{ util::literal_to_mysqlnd_str("com_ping") },
	{ util::literal_to_mysqlnd_str("com_time") },
	{ util::literal_to_mysqlnd_str("com_delayed_insert") },
	{ util::literal_to_mysqlnd_str("com_change_user") },
	{ util::literal_to_mysqlnd_str("com_binlog_dump") },
	{ util::literal_to_mysqlnd_str("com_table_dump") },
	{ util::literal_to_mysqlnd_str("com_connect_out") },
	{ util::literal_to_mysqlnd_str("com_register_slave") },
	{ util::literal_to_mysqlnd_str("com_stmt_prepare") },
	{ util::literal_to_mysqlnd_str("com_stmt_execute") },
	{ util::literal_to_mysqlnd_str("com_stmt_send_long_data") },
	{ util::literal_to_mysqlnd_str("com_stmt_close") },
	{ util::literal_to_mysqlnd_str("com_stmt_reset") },
	{ util::literal_to_mysqlnd_str("com_stmt_set_option") },
	{ util::literal_to_mysqlnd_str("com_stmt_fetch") },
	{ util::literal_to_mysqlnd_str("com_deamon") },
	{ util::literal_to_mysqlnd_str("bytes_received_real_data_normal") },
	{ util::literal_to_mysqlnd_str("bytes_received_real_data_ps") }
};

static struct st_mysqlnd_plugin_core xmysqlnd_plugin_core =
{
	{
		MYSQLND_PLUGIN_API_VERSION,
		"mysql_xdevapi",
		MYSQL_XDEVAPI_VERSION_ID,
		PHP_MYSQL_XDEVAPI_VERSION,
		"PHP License 3.01",
		"Andrey Hristov <andrey@mysql.com>",
		{
			nullptr, /* will be filled later */
			xmysqlnd_stats_values_names,
		},
		{
			nullptr /* plugin shutdown */
		}
	}
};


PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_end(void)
{
	if (xmysqlnd_library_initted == TRUE) {
		mysqlnd_stats_end(xmysqlnd_global_stats, 1);
		xmysqlnd_global_stats = nullptr;
		xmysqlnd_library_initted = FALSE;
	}
	xmysqlnd_shutdown_protobuf_library();
}

PHP_MYSQL_XDEVAPI_API void xmysqlnd_library_init(void)
{
	if (xmysqlnd_library_initted == FALSE) {
		xmysqlnd_library_initted = TRUE;

		/* Should be calloc, as mnd_calloc will reference LOCK_access*/
		mysqlnd_stats_init(&xmysqlnd_global_stats, XMYSQLND_STAT_LAST, 1);
		{
			xmysqlnd_plugin_core.plugin_header.plugin_stats.values = xmysqlnd_global_stats;
		}
	}
}

PHP_MYSQL_XDEVAPI_API const char * xmysqlnd_get_client_info()
{
	return PHP_XMYSQLND_VERSION;
}

PHP_MYSQL_XDEVAPI_API unsigned int xmysqlnd_get_client_version()
{
	return XMYSQLND_VERSION_ID;
}

} // namespace drv

} // namespace mysqlx
