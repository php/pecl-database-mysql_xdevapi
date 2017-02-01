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
#ifndef XMYSQLND_NODE_SESSION_H
#define XMYSQLND_NODE_SESSION_H

extern "C" {
#include <ext/mysqlnd/mysqlnd_connection.h>
#include <ext/mysqlnd/mysqlnd_enum_n_def.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_vio.h>
}
#include "xmysqlnd_driver.h"
#include "xmysqlnd_protocol_frame_codec.h"
#include "xmysqlnd_node_stmt.h"


namespace mysqlx {

namespace drv {

struct st_xmysqlnd_node_stmt;
struct st_xmysqlnd_node_schema;
struct st_xmysqlnd_stmt_op__execute;

PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_session_module_init();

/* XMYSQLND_NODE_SESSION_DATA::client_capabilities */
#define XMYSQLND_CLIENT_NO_FLAG	0
#define XMYSQLND_FICTIVE_FLAG	1

enum xmysqlnd_node_session_state
{
	NODE_SESSION_ALLOCED = 0,
	NODE_SESSION_NON_AUTHENTICATED = 1,
	NODE_SESSION_READY = 2,
	NODE_SESSION_CLOSE_SENT = 3
};


typedef enum xmysqlnd_node_session_close_type
{
	XMYSQLND_CLOSE_EXPLICIT = 0,
	XMYSQLND_CLOSE_IMPLICIT,
	XMYSQLND_CLOSE_DISCONNECT,
	XMYSQLND_CLOSE_LAST	/* for checking, should always be last */
} enum_xmysqlnd_node_session_close_type;


typedef enum xmysqlnd_server_option
{
	XMYSQLND_SOME_SERVER_OPTION = 0,
} enum_xmysqlnd_server_option;


struct st_xmysqlnd_query_builder
{
	enum_func_status (*create)(struct st_xmysqlnd_query_builder * builder);
	void (*destroy)(struct st_xmysqlnd_query_builder * builder);
	MYSQLND_STRING query;
};



typedef struct st_xmysqlnd_node_session_state XMYSQLND_NODE_SESSION_STATE;
typedef enum xmysqlnd_node_session_state (*func_xmysqlnd_node_session_state__get)(const XMYSQLND_NODE_SESSION_STATE * const state_struct);
typedef void (*func_xmysqlnd_node_session_state__set)(XMYSQLND_NODE_SESSION_STATE * const state_struct, const enum xmysqlnd_node_session_state state);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_state)
{
	func_xmysqlnd_node_session_state__get get;
	func_xmysqlnd_node_session_state__set set;
};

struct st_xmysqlnd_node_session_state
{
	enum xmysqlnd_node_session_state state;

	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_state) *m;
};

#define GET_SESSION_STATE(state_struct)		(state_struct)->m->get((state_struct))
#define SET_SESSION_STATE(state_struct, s)	(state_struct)->m->set((state_struct), (s))
PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_session_state_init(XMYSQLND_NODE_SESSION_STATE * const state);



typedef struct st_xmysqlnd_node_session_options
{
	size_t	unused;
} XMYSQLND_NODE_SESSION_OPTIONS;


typedef struct st_xmysqlnd_level3_io
{
	MYSQLND_VIO * vio;
	XMYSQLND_PFC * pfc;
} XMYSQLND_L3_IO;

typedef struct st_xmysqlnd_node_session XMYSQLND_NODE_SESSION;
typedef struct st_xmysqlnd_node_session_data XMYSQLND_NODE_SESSION_DATA;

typedef enum_func_status	(*func_xmysqlnd_node_session_data__init)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__connect_handshake)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING scheme, const MYSQLND_CSTRING username, const MYSQLND_CSTRING password, const MYSQLND_CSTRING database, const size_t set_capabilities);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__authenticate)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING scheme, const MYSQLND_CSTRING username, const MYSQLND_CSTRING password, const MYSQLND_CSTRING database, const size_t set_capabilities);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__connect)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING hostname, const MYSQLND_CSTRING username, const MYSQLND_CSTRING password, const MYSQLND_CSTRING database, const MYSQLND_CSTRING socket_or_pipe, unsigned int port, size_t set_capabilities);
typedef size_t				(*func_xmysqlnd_node_session_data__escape_string)(XMYSQLND_NODE_SESSION_DATA * const session, char *newstr, const char *escapestr, const size_t escapestr_len);
typedef MYSQLND_STRING		(*func_xmysqlnd_node_session_data__quote_name)(XMYSQLND_NODE_SESSION_DATA * session, const MYSQLND_CSTRING name);


typedef unsigned int		(*func_xmysqlnd_node_session_data__get_error_no)(const XMYSQLND_NODE_SESSION_DATA * const session);
typedef const char *		(*func_xmysqlnd_node_session_data__get_error_str)(const XMYSQLND_NODE_SESSION_DATA * const session);
typedef const char *		(*func_xmysqlnd_node_session_data__get_sqlstate)(const XMYSQLND_NODE_SESSION_DATA * const session);

typedef const char *		(*func_xmysqlnd_node_session_data__get_host_information)(const XMYSQLND_NODE_SESSION_DATA * const session);
typedef const char *		(*func_xmysqlnd_node_session_data__get_protocol_information)(const XMYSQLND_NODE_SESSION_DATA * const session);
typedef const char *		(*func_xmysqlnd_node_session_data__get_charset_name)(const XMYSQLND_NODE_SESSION_DATA * const session);

typedef enum_func_status	(*func_xmysqlnd_node_session_data__set_server_option)(XMYSQLND_NODE_SESSION_DATA * const session, enum_xmysqlnd_server_option option, const char * const value);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__set_client_option)(XMYSQLND_NODE_SESSION_DATA * const session, enum_xmysqlnd_client_option option, const char * const value);
typedef void				(*func_xmysqlnd_node_session_data__free_contents)(XMYSQLND_NODE_SESSION_DATA * session);/* private */
typedef void				(*func_xmysqlnd_node_session_data__free_options)(XMYSQLND_NODE_SESSION_DATA * session);	/* private */
typedef void				(*func_xmysqlnd_node_session_data__dtor)(XMYSQLND_NODE_SESSION_DATA * session);	/* private */

typedef XMYSQLND_NODE_SESSION_DATA *	(*func_xmysqlnd_node_session_data__get_reference)(XMYSQLND_NODE_SESSION_DATA * const session);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__free_reference)(XMYSQLND_NODE_SESSION_DATA * const session);

typedef enum_func_status	(*func_xmysqlnd_node_session_data__send_close)(XMYSQLND_NODE_SESSION_DATA * session);

typedef enum_func_status    (*func_xmysqlnd_node_session_data__ssl_set)(XMYSQLND_NODE_SESSION_DATA * const session, const char * key, const char * const cert, const char * const ca, const char * const capath, const char * const cipher);


typedef enum_func_status	(*func_xmysqlnd_node_session_data__local_tx_start)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__local_tx_end)(XMYSQLND_NODE_SESSION_DATA * session, const size_t this_func, const enum_func_status status);

typedef size_t				(*func_xmysqlnd_node_session_data__negotiate_client_api_capabilities)(XMYSQLND_NODE_SESSION_DATA * const session, const size_t flags);
typedef size_t				(*func_xmysqlnd_node_session_data__get_client_api_capabilities)(const XMYSQLND_NODE_SESSION_DATA * const session);

typedef MYSQLND_STRING		(*func_xmysqlnd_node_session_data__get_scheme)(XMYSQLND_NODE_SESSION_DATA * session, MYSQLND_CSTRING hostname, MYSQLND_CSTRING * socket_or_pipe, unsigned int port, zend_bool * unix_socket, zend_bool * named_pipe);

typedef const enum_hnd_func_status (*func_xmysqlnd_node_session_data__handler_on_error)(void * context, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
typedef const enum_hnd_func_status (*func_xmysqlnd_node_session_data__handler_on_auth_continue)(void * context, const MYSQLND_CSTRING input, MYSQLND_STRING * const output);
typedef size_t				(*func_xmysqlnd_node_session_data__get_client_id)(const XMYSQLND_NODE_SESSION_DATA * const session);
typedef enum_func_status	(*func_xmysqlnd_node_session_data__set_client_id)(void * context, const size_t id);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data)
{
	func_xmysqlnd_node_session_data__init init;
	func_xmysqlnd_node_session_data__get_scheme get_scheme;
	func_xmysqlnd_node_session_data__connect_handshake connect_handshake;
	func_xmysqlnd_node_session_data__authenticate authenticate;
	func_xmysqlnd_node_session_data__connect connect;
	func_xmysqlnd_node_session_data__escape_string escape_string;
	func_xmysqlnd_node_session_data__quote_name quote_name;

	func_xmysqlnd_node_session_data__get_error_no get_error_no;
	func_xmysqlnd_node_session_data__get_error_str get_error_str;
	func_xmysqlnd_node_session_data__get_sqlstate get_sqlstate;

	func_xmysqlnd_node_session_data__get_host_information get_host_information;
	func_xmysqlnd_node_session_data__get_protocol_information get_protocol_information;
	func_xmysqlnd_node_session_data__get_charset_name get_charset_name;

	func_xmysqlnd_node_session_data__set_server_option set_server_option;
	func_xmysqlnd_node_session_data__set_client_option set_client_option;

	func_xmysqlnd_node_session_data__free_contents free_contents;
	func_xmysqlnd_node_session_data__free_options free_options;
	func_xmysqlnd_node_session_data__dtor dtor;

	func_xmysqlnd_node_session_data__get_reference get_reference;
	func_xmysqlnd_node_session_data__free_reference free_reference;

	func_xmysqlnd_node_session_data__send_close send_close;

	func_xmysqlnd_node_session_data__ssl_set ssl_set;

	func_xmysqlnd_node_session_data__local_tx_start local_tx_start;
	func_xmysqlnd_node_session_data__local_tx_end local_tx_end;

	func_xmysqlnd_node_session_data__negotiate_client_api_capabilities negotiate_client_api_capabilities;
	func_xmysqlnd_node_session_data__get_client_api_capabilities get_client_api_capabilities;

	func_xmysqlnd_node_session_data__handler_on_error handler_on_error;	/* export the function which is used as network event handler */
	func_xmysqlnd_node_session_data__handler_on_auth_continue handler_on_auth_continue;
	func_xmysqlnd_node_session_data__get_client_id get_client_id;
	func_xmysqlnd_node_session_data__set_client_id set_client_id;
};


struct st_xmysqlnd_node_session_data
{
/* Operation related */
	XMYSQLND_L3_IO	io;

/* Information related */
	MYSQLND_STRING	hostname;
	MYSQLND_STRING	username;
	MYSQLND_STRING	password;
	MYSQLND_STRING	scheme;
	MYSQLND_STRING	current_db;
	unsigned int	port;
	MYSQLND_STRING	unix_socket;
	char			*server_host_info;
	size_t			client_id;

	const MYSQLND_CHARSET * charset;

	/* If error packet, we use these */
	MYSQLND_ERROR_INFO	* error_info;
	MYSQLND_ERROR_INFO	error_info_impl;

/* Operation related */
	XMYSQLND_NODE_SESSION_STATE state;

	/*
	  How many result sets reference this connection.
	  It won't be freed until this number reaches 0.
	  The last one, please close the door! :-)
	*/
	unsigned int	refcount;

	/* options */
	XMYSQLND_NODE_SESSION_OPTIONS	* options;
	XMYSQLND_NODE_SESSION_OPTIONS	options_impl;

	size_t			client_api_capabilities;

	/* stats */
	MYSQLND_STATS	* stats;
	zend_bool		own_stats;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session_data) * m;

	/* persistent connection */
	zend_bool		persistent;
};


struct st_xmysqlnd_node_session_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt);
	void * ctx;
};

struct st_xmysqlnd_node_session_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt, const struct st_xmysqlnd_node_stmt_result_meta * const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_node_session_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_node_session_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt, const unsigned int code, const MYSQLND_CSTRING sql_state, const MYSQLND_CSTRING message);
	void * ctx;
};


struct st_xmysqlnd_node_session_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_node_session_on_statement_ok_bind
{
	enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_node_stmt * const stmt, const struct st_xmysqlnd_stmt_execution_state * const exec_state);
	void * ctx;
};


struct st_xmysqlnd_node_session_query_bind_variable_bind
{
	const enum_hnd_func_status (*handler)(void * context, XMYSQLND_NODE_SESSION * session, struct st_xmysqlnd_stmt_op__execute * const stmt);
	void * ctx;
};



typedef const enum_func_status	(*func_xmysqlnd_node_session__init)(XMYSQLND_NODE_SESSION * session,
																	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
																	MYSQLND_STATS * stats,
																	MYSQLND_ERROR_INFO * error_info);

typedef const enum_func_status	(*func_xmysqlnd_node_session__connect)(XMYSQLND_NODE_SESSION * session,
																	   MYSQLND_CSTRING hostname,
																	   MYSQLND_CSTRING username,
																	   MYSQLND_CSTRING password,
																	   MYSQLND_CSTRING database,
																	   MYSQLND_CSTRING socket_or_pipe,
																	   const unsigned int port,
																	   const size_t set_capabilities);

typedef const MYSQLND_CSTRING	(*func_xmysqlnd_node_session__get_uuid)(XMYSQLND_NODE_SESSION * const session);
typedef const enum_func_status	(*func_xmysqlnd_node_session__precache_uuids)(XMYSQLND_NODE_SESSION * const session);
typedef const enum_func_status	(*func_xmysqlnd_node_session__create_db)(XMYSQLND_NODE_SESSION * session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_node_session__select_db)(XMYSQLND_NODE_SESSION * session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_node_session__drop_db)(XMYSQLND_NODE_SESSION * session, const MYSQLND_CSTRING db);
typedef const enum_func_status	(*func_xmysqlnd_node_session__query_cb)(XMYSQLND_NODE_SESSION * session,
																		const MYSQLND_CSTRING namespace_,
																		const MYSQLND_CSTRING query,
																		const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder,
																		const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start,
																		const struct st_xmysqlnd_node_session_on_row_bind on_row,
																		const struct st_xmysqlnd_node_session_on_warning_bind on_warning,
																		const struct st_xmysqlnd_node_session_on_error_bind on_error,
																		const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end,
																		const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok);

typedef const enum_func_status	(*func_xmysqlnd_node_session__query_cb_ex)(XMYSQLND_NODE_SESSION * session,
																		   const MYSQLND_CSTRING namespace_,
																		   struct st_xmysqlnd_query_builder * query_builder,
																		   const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder,
																		   const struct st_xmysqlnd_node_session_on_result_start_bind on_result_start,
																		   const struct st_xmysqlnd_node_session_on_row_bind on_row,
																		   const struct st_xmysqlnd_node_session_on_warning_bind on_warning,
																		   const struct st_xmysqlnd_node_session_on_error_bind on_error,
																		   const struct st_xmysqlnd_node_session_on_result_end_bind on_result_end,
																		   const struct st_xmysqlnd_node_session_on_statement_ok_bind on_statement_ok);



typedef const enum_func_status	(*func_xmysqlnd_node_session__query)(XMYSQLND_NODE_SESSION * session,
																	 const MYSQLND_CSTRING namespace_,
																	 const MYSQLND_CSTRING query,
																	 const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder);

typedef zend_ulong			(*func_xmysqlnd_node_session__get_server_version)(XMYSQLND_NODE_SESSION * const session);
typedef const char *		(*func_xmysqlnd_node_session__get_server_information)(const XMYSQLND_NODE_SESSION * const session);

typedef struct st_xmysqlnd_node_stmt *		(*func_xmysqlnd_node_session__create_statement_object)(XMYSQLND_NODE_SESSION * const session);

typedef struct st_xmysqlnd_node_schema *	(*func_xmysqlnd_node_session__create_schema_object)(XMYSQLND_NODE_SESSION * const session, const MYSQLND_CSTRING schema_name);
typedef enum_func_status					(*func_xmysqlnd_node_session__drop_collection)(XMYSQLND_NODE_SESSION * const session,
																							const MYSQLND_CSTRING schema_name,
																							const MYSQLND_CSTRING collection_name,
																							const struct st_xmysqlnd_node_session_on_error_bind on_error);
typedef enum_func_status					(*func_xmysqlnd_node_session__drop_table)(XMYSQLND_NODE_SESSION * const session,
																							const MYSQLND_CSTRING schema_name,
																							const MYSQLND_CSTRING table_name,
																							const struct st_xmysqlnd_node_session_on_error_bind on_error);

typedef const enum_func_status				(*func_xmysqlnd_node_session__close)(XMYSQLND_NODE_SESSION * session, const enum_xmysqlnd_node_session_close_type close_type);

typedef XMYSQLND_NODE_SESSION *	(*func_xmysqlnd_node_session__get_reference)(XMYSQLND_NODE_SESSION * const session);
typedef const enum_func_status	(*func_xmysqlnd_node_session__free_reference)(XMYSQLND_NODE_SESSION * const session);
typedef void					(*func_xmysqlnd_node_session__free_contents)(XMYSQLND_NODE_SESSION * session);/* private */
typedef void					(*func_xmysqlnd_node_session__dtor)(XMYSQLND_NODE_SESSION * session);



MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session)
{
	func_xmysqlnd_node_session__init init;
	func_xmysqlnd_node_session__connect connect;
	func_xmysqlnd_node_session__get_uuid get_uuid;
	func_xmysqlnd_node_session__precache_uuids precache_uuids;
	func_xmysqlnd_node_session__create_db create_db;
	func_xmysqlnd_node_session__select_db select_db;
	func_xmysqlnd_node_session__drop_db drop_db;
	func_xmysqlnd_node_session__query query;
	func_xmysqlnd_node_session__query_cb query_cb;
	func_xmysqlnd_node_session__query_cb_ex query_cb_ex;
	func_xmysqlnd_node_session__get_server_version get_server_version;
	func_xmysqlnd_node_session__get_server_information get_server_information;
	func_xmysqlnd_node_session__create_statement_object create_statement_object;
	func_xmysqlnd_node_session__create_schema_object create_schema_object;
	func_xmysqlnd_node_session__drop_collection drop_collection;
	func_xmysqlnd_node_session__drop_table drop_table;
	func_xmysqlnd_node_session__close close;
	func_xmysqlnd_node_session__get_reference get_reference;
	func_xmysqlnd_node_session__free_reference free_reference;
	func_xmysqlnd_node_session__free_contents free_contents;
	func_xmysqlnd_node_session__dtor dtor;
};

#define XMYSQLND_UUID_LENGTH 32
#define XMYSQLND_UUID_CACHE_ELEMENTS 8

struct st_xmysqlnd_node_session
{
	XMYSQLND_NODE_SESSION_DATA * data;
	char * server_version_string;
	unsigned int refcount;
	struct st_xmysqlnd_node_session_uuid_cache_list
	{
		char * pool;
		unsigned int used:15;
		unsigned int allocated:15;
		zend_bool persistent:1;
	} uuid_cache;
	zend_bool persistent;
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_session) * m;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_session_data);
PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_session);

PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_SESSION * xmysqlnd_node_session_create(const size_t client_flags, const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_SESSION * xmysqlnd_node_session_connect(XMYSQLND_NODE_SESSION * session,
															 const MYSQLND_CSTRING hostname,
															 const MYSQLND_CSTRING username,
															 const MYSQLND_CSTRING password,
															 const MYSQLND_CSTRING database,
															 const MYSQLND_CSTRING socket_or_pipe,
															 unsigned int port,
															 const size_t set_capabilities,
															 const size_t client_api_flags);

extern const MYSQLND_CSTRING namespace_sql;
extern const MYSQLND_CSTRING namespace_xplugin;

extern const struct st_xmysqlnd_node_session_query_bind_variable_bind noop__var_binder;
extern const struct st_xmysqlnd_node_session_on_result_start_bind	noop__on_result_start;
extern const struct st_xmysqlnd_node_session_on_row_bind			noop__on_row;
extern const struct st_xmysqlnd_node_session_on_warning_bind		noop__on_warning;
extern const struct st_xmysqlnd_node_session_on_error_bind			noop__on_error;
extern const struct st_xmysqlnd_node_session_on_result_end_bind		noop__on_result_end;
extern const struct st_xmysqlnd_node_session_on_statement_ok_bind	noop__on_statement_ok;

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_NODE_SESSION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
