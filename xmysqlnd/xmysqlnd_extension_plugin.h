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
#ifndef XMYSQLND_EXTENSION_PLUGIN_H
#define XMYSQLND_EXTENSION_PLUGIN_H

extern "C" {
#include <ext/mysqlnd/mysqlnd_structs.h>
}
#include "php_mysql_xdevapi.h"

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_session;
struct st_xmysqlnd_session_data;
struct st_xmysqlnd_schema;
struct st_xmysqlnd_collection;
struct st_xmysqlnd_stmt;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_table;
struct st_xmysqlnd_rowset_buffered;
struct st_xmysqlnd_rowset_fwd;
struct st_xmysqlnd_stmt_result_meta;
struct st_xmysqlnd_rowset;
struct st_xmysqlnd_protocol_frame_codec;

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session_data);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_collection);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_table);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_buffered);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_fwd);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec);
MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list);

struct st_xmysqlnd_plugin__plugin_area_getters
{
	void ** (*get_node_session_area)(const st_xmysqlnd_session* conn, const unsigned int plugin_id);
        void ** (*get_node_session_data_data_area)(const st_xmysqlnd_session_data* conn, const unsigned int plugin_id);
	void ** (*get_node_schema_area)(const st_xmysqlnd_schema* schema, unsigned int plugin_id);
	void ** (*get_node_collection_area)(const st_xmysqlnd_collection* collection, unsigned int plugin_id);
	void ** (*get_node_table_area)(const st_xmysqlnd_table* table, unsigned int plugin_id);
	void ** (*get_node_stmt_area)(const st_xmysqlnd_stmt* stmt, unsigned int plugin_id);
	void ** (*get_node_stmt_result_area)(const st_xmysqlnd_stmt_result* result, unsigned int plugin_id);
	void ** (*get_rowset_buffered_area)(const st_xmysqlnd_rowset_buffered* result, unsigned int plugin_id);
	void ** (*get_rowset_fwd_area)(const st_xmysqlnd_rowset_fwd* result, unsigned int plugin_id);
	void ** (*get_node_query_result_meta_area)(const st_xmysqlnd_stmt_result_meta* result, unsigned int plugin_id);
	void ** (*get_rowset_area)(const st_xmysqlnd_rowset* result, unsigned int plugin_id);
	void ** (*get_pfc_area)(const st_xmysqlnd_protocol_frame_codec* pfc, unsigned int plugin_id);
};

extern struct st_xmysqlnd_plugin__plugin_area_getters xmysqlnd_plugin_area_getters;

#define xmysqlnd_plugin_get_node_session_plugin_area(s, p_id)				xmysqlnd_plugin_area_getters.get_node_session_area((s), (p_id))
#define xmysqlnd_plugin_get_node_session_data_plugin_area(s, p_id)			xmysqlnd_plugin_area_getters.get_node_session_data_data_area((s), (p_id))
#define xmysqlnd_plugin_get_node_schema_plugin_area(s, p_id)				xmysqlnd_plugin_area_getters.get_node_schema_area((s), (p_id))
#define xmysqlnd_plugin_get_node_collection_plugin_area(s, p_id)			xmysqlnd_plugin_area_getters.get_node_collection_area((s), (p_id))
#define xmysqlnd_plugin_get_node_table_plugin_area(s, p_id)					xmysqlnd_plugin_area_getters.get_node_table_area((s), (p_id))
#define xmysqlnd_plugin_get_node_stmt_plugin_area(s, p_id)					xmysqlnd_plugin_area_getters.get_node_stmt_area((s), (p_id))
#define xmysqlnd_plugin_get_node_stmt_result_plugin_area(r, p_id)			xmysqlnd_plugin_area_getters.get_node_stmt_result_area((r), (p_id))
#define xmysqlnd_plugin_get_rowset_buffered_plugin_area(r, p_id)			xmysqlnd_plugin_area_getters.get_rowset_buffered_area((r), (p_id))
#define xmysqlnd_plugin_get_rowset_fwd_plugin_area(r, p_id)					xmysqlnd_plugin_area_getters.get_rowset_fwd_area((r), (p_id))
#define xmysqlnd_plugin_get_node_query_result_meta_plugin_area(m, p_id)		xmysqlnd_plugin_area_getters.get_node_query_result_meta_area((m), (p_id))
#define xmysqlnd_plugin_get_rowset_plugin_area(r, p_id)						xmysqlnd_plugin_area_getters.get_rowset_area((r), (p_id))
#define xmysqlnd_plugin_get_pfc_plugin_area(pfc, p_id)						xmysqlnd_plugin_area_getters.get_pfc_area((pfc), (p_id))


struct st_xmysqlnd_plugin_methods_xetters
{
	struct st_xmnd_object_factory_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const methods);
	} object_factory;

	struct st_xmnd_node_session_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session) * const methods);
	} node_session;

	struct st_xmnd_node_session_data_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session_data) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_session_data) * const methods);
	} node_session_data;

	struct st_xmnd_node_schema_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_schema) * const methods);
	} node_schema;

	struct st_xmnd_node_collection_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_collection) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_collection) * const methods);
	} node_collection;

	struct st_xmnd_node_table_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_table) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_table) * const methods);
	} node_table;

	struct st_xmnd_node_stmt_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt) * const methods);
	} node_stmt;

	struct st_xmnd_node_stmt_result_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result) * const methods);
	} node_stmt_result;

	struct st_xmnd_rowset_buffered_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_buffered) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_buffered) * const methods);
	} rowset_buffered;

	struct st_xmnd_rowset_fwd_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_fwd) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset_fwd) * const methods);
	} rowset_fwd;

	struct st_xmnd_node_query_result_meta_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta) * const methods);
	} node_stmt_result_meta;

	struct st_xmnd_result_field_meta_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) * const methods);
	} result_field_meta;

	struct st_xmnd_rowset_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_rowset) * const methods);
	} rowset;

	struct st_xmnd_pfc_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) * const methods);
	} pfc;

	struct st_xmnd_warning_list_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_warning_list) * const methods);
	} warning_list;

	struct st_xmnd_stmt_execution_state_xetters
	{
		const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) * (*get)();
		void (*set)(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) * const methods);
	} stmt_exec_state;
};

PHP_MYSQL_XDEVAPI_API extern struct st_xmysqlnd_plugin_methods_xetters xmysqlnd_plugin_methods_xetters;


#define xmysqlnd_object_factory_get_methods()			xmysqlnd_plugin_methods_xetters.object_factory.get()
#define xmysqlnd_object_factory_set_methods(m)			xmysqlnd_plugin_methods_xetters.object_factory.set((m))

#define xmysqlnd_node_session_get_methods()				xmysqlnd_plugin_methods_xetters.node_session.get()
#define xmysqlnd_node_session_set_methods(m)			xmysqlnd_plugin_methods_xetters.node_session.set((m))

#define xmysqlnd_node_session_data_get_methods()		xmysqlnd_plugin_methods_xetters.node_session_data.get()
#define xmysqlnd_node_session_data_set_methods(m)		xmysqlnd_plugin_methods_xetters.node_session_data.set((m))

#define xmysqlnd_node_schema_get_methods()				xmysqlnd_plugin_methods_xetters.node_schema.get()
#define xmysqlnd_node_schema_set_methods(m)				xmysqlnd_plugin_methods_xetters.node_schema.set((m))

#define xmysqlnd_node_collection_get_methods()			xmysqlnd_plugin_methods_xetters.node_collection.get()
#define xmysqlnd_node_collection_set_methods(m)			xmysqlnd_plugin_methods_xetters.node_collection.set((m))

#define xmysqlnd_node_table_get_methods()				xmysqlnd_plugin_methods_xetters.node_table.get()
#define xmysqlnd_node_table_set_methods(m)				xmysqlnd_plugin_methods_xetters.node_table.set((m))

#define xmysqlnd_node_stmt_get_methods()				xmysqlnd_plugin_methods_xetters.node_stmt.get()
#define xmysqlnd_node_stmt_set_methods(m)				xmysqlnd_plugin_methods_xetters.node_stmt.set((m))

#define xmysqlnd_node_stmt_result_get_methods()			xmysqlnd_plugin_methods_xetters.node_stmt_result.get()
#define xmysqlnd_node_stmt_result_set_methods(m)		xmysqlnd_plugin_methods_xetters.node_stmt_result.set((m))

#define xmysqlnd_rowset_buffered_get_methods()			xmysqlnd_plugin_methods_xetters.rowset_buffered.get()
#define xmysqlnd_rowset_buffered_set_methods(m)			xmysqlnd_plugin_methods_xetters.rowset_buffered.set((m))

#define xmysqlnd_rowset_fwd_get_methods()				xmysqlnd_plugin_methods_xetters.rowset_fwd.get()
#define xmysqlnd_rowset_fwd_set_methods(m)				xmysqlnd_plugin_methods_xetters.rowset_fwd.set((m))

#define xmysqlnd_node_stmt_result_meta_get_methods()	xmysqlnd_plugin_methods_xetters.node_stmt_result_meta.get()
#define xmysqlnd_node_stmt_result_meta_set_methods(m)	xmysqlnd_plugin_methods_xetters.node_stmt_result_meta.set((m))

#define xmysqlnd_result_field_meta_get_methods()		xmysqlnd_plugin_methods_xetters.result_field_meta.get()
#define xmysqlnd_result_field_meta_set_methods(m)		xmysqlnd_plugin_methods_xetters.result_field_meta.set((m))

#define xmysqlnd_rowset_get_methods()					xmysqlnd_plugin_methods_xetters.rowset.get()
#define xmysqlnd_rowset_set_methods(m)					xmysqlnd_plugin_methods_xetters.rowset.set((m))

#define xmysqlnd_pfc_get_methods()						xmysqlnd_plugin_methods_xetters.pfc.get()
#define xmysqlnd_pfc_set_methods(m)						xmysqlnd_plugin_methods_xetters.pfc.set((m))

#define xmysqlnd_warning_list_get_methods()				xmysqlnd_plugin_methods_xetters.warning_list.get()
#define xmysqlnd_warning_list_set_methods(m)			xmysqlnd_plugin_methods_xetters.warning_list.set((m))

#define xmysqlnd_stmt_execution_state_get_methods()		xmysqlnd_plugin_methods_xetters.stmt_exec_state.get()
#define xmysqlnd_stmt_execution_state_set_methods(m)	xmysqlnd_plugin_methods_xetters.stmt_exec_state.set((m))

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_EXTENSION_PLUGIN_H */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
