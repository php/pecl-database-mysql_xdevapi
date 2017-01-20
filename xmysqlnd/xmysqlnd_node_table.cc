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
#include <php.h>
#undef ERROR
#include "ext/json/php_json_parser.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_node_table.h"
#include "xmysqlnd_utils.h"

/* {{{ xmysqlnd_node_table::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, init)(XMYSQLND_NODE_TABLE * const table,
										   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										   XMYSQLND_NODE_SCHEMA * const schema,
										   const MYSQLND_CSTRING table_name,
										   MYSQLND_STATS * const stats,
										   MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_table::init");
	if (!(table->data->schema = schema->data->m.get_reference(schema))) {
		return FAIL;
	}
	table->data->table_name = mnd_dup_cstring(table_name, table->data->persistent);
	DBG_INF_FMT("name=[%d]%*s", table->data->table_name.l, table->data->table_name.l, table->data->table_name.s);

	table->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


struct st_table_exists_in_database_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	const MYSQLND_CSTRING table_name;
	unsigned int counter;
};


/* {{{ table_op_var_binder */
static const enum_hnd_func_status
table_op_var_binder(
	void * context,
	XMYSQLND_NODE_SESSION * session,
	XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret = HND_FAIL;
	struct st_table_exists_in_database_var_binder_ctx * ctx = (struct st_table_exists_in_database_var_binder_ctx *) context;
	const MYSQLND_CSTRING * param = NULL;
	DBG_ENTER("table_op_var_binder");
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			goto bind;
		case 1:{
			param = &ctx->table_name;
			ret = HND_PASS;
bind:
			{
				enum_func_status result;
				zval zv;
				ZVAL_UNDEF(&zv);
				ZVAL_STRINGL(&zv, param->s, param->l);
				DBG_INF_FMT("[%d]=[%*s]", ctx->counter, param->l, param->s);
				result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, ctx->counter, &zv);

				zval_ptr_dtor(&zv);
				if (FAIL == result) {
					ret = FAIL;
				}
			}
			break;
		}
		default:
			assert(!"should not happen");
			break;
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


struct st_table_exists_in_database_ctx
{
	const MYSQLND_CSTRING expected_table_name;
	const MYSQLND_CSTRING expected_object_type;
	zval* exists;
};


/* {{{ table_xplugin_op_on_row */
static const enum_hnd_func_status
table_xplugin_op_on_row(
	void * context,
	XMYSQLND_NODE_SESSION * const session,
	XMYSQLND_NODE_STMT * const stmt,
	const XMYSQLND_NODE_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	struct st_table_exists_in_database_ctx * ctx = (struct st_table_exists_in_database_ctx *) context;
	DBG_ENTER("table_xplugin_op_on_row");
	if (ctx && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const MYSQLND_CSTRING object_type = { Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if (equal_mysqlnd_cstr(&object_name, &ctx->expected_table_name)
			&& equal_mysqlnd_cstr(&object_type, &ctx->expected_object_type))
		{
			ZVAL_TRUE(ctx->exists);
		}
		else
		{
			ZVAL_FALSE(ctx->exists);
		}
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_table::exists_in_database */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, exists_in_database)(
	XMYSQLND_NODE_TABLE * const table,
	struct st_xmysqlnd_node_session_on_error_bind on_error,
	zval* exists)
{
	DBG_ENTER("xmysqlnd_node_table::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };
	XMYSQLND_NODE_SCHEMA * schema = table->data->schema;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	struct st_table_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		mnd_str2c(table->data->table_name),
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	struct st_table_exists_in_database_ctx on_row_ctx = {
		mnd_str2c(table->data->table_name),
		xmysqlnd_object_type_filter__table,
		exists
	};

	const struct st_xmysqlnd_node_session_on_row_bind on_row = { table_xplugin_op_on_row, &on_row_ctx };

	ret = session->m->query_cb(session,
							   namespace_xplugin,
							   query,
							   var_binder,
							   noop__on_result_start,
							   on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
}
/* }}} */


struct st_table_sql_single_result_ctx
{
	zval* result;
};


/* {{{ table_xplugin_op_on_row */
static const enum_hnd_func_status
table_sql_single_result_op_on_row(
	void * context,
	XMYSQLND_NODE_SESSION * const session,
	XMYSQLND_NODE_STMT * const stmt,
	const XMYSQLND_NODE_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	struct st_table_sql_single_result_ctx * ctx = (struct st_table_sql_single_result_ctx *) context;
	DBG_ENTER("table_xplugin_op_on_row");
	if (ctx && row) {
		ZVAL_COPY_VALUE(ctx->result, &row[0]);
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_table::count */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, count)(
	XMYSQLND_NODE_TABLE * const table,
	struct st_xmysqlnd_node_session_on_error_bind on_error,
	zval* counter)
{
	DBG_ENTER("xmysqlnd_node_table::count");
	ZVAL_LONG(counter, 0);

	enum_func_status ret;

	XMYSQLND_NODE_SCHEMA * schema = table->data->schema;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	char* query_str;
	mnd_sprintf(&query_str, 0, "SELECT COUNT(*) FROM %s.%s", schema->data->schema_name.s, table->data->table_name.s);
	if (!query_str) {
		DBG_RETURN(FAIL);
	}
	const MYSQLND_CSTRING query = {query_str, strlen(query_str)};

	struct st_table_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		mnd_str2c(table->data->table_name),
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { table_op_var_binder, &var_binder_ctx };

	struct st_table_sql_single_result_ctx on_row_ctx = {
		counter
	};

	const struct st_xmysqlnd_node_session_on_row_bind on_row = { table_sql_single_result_op_on_row, &on_row_ctx };

	ret = session->m->query_cb(session,
							   namespace_sql,
							   query,
							   noop__var_binder, //var_binder,
							   noop__on_result_start,
							   on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	mnd_sprintf_free(query_str);
	DBG_RETURN(ret);
}
/* }}} */


#define ID_COLUMN_NAME		"_id"
#define ID_TEMPLATE_PREFIX	"\""ID_COLUMN_NAME"\":\""
#define ID_TEMPLATE_SUFFIX	"\"}"

struct st_parse_for_id_status
{
	zend_bool found : 1;
	zend_bool empty : 1;
};

struct my_php_json_parser {
	php_json_parser parser;
	struct st_parse_for_id_status * status;
};


/* {{{ xmysqlnd_json_parser_object_update */
static int
xmysqlnd_json_parser_object_update(php_json_parser *parser, zval *object, zend_string *key, zval *zvalue)
{
	struct st_parse_for_id_status * status = ((struct my_php_json_parser *)parser)->status;
	DBG_ENTER("xmysqlnd_json_parser_object_update");
	/* if JSON_OBJECT_AS_ARRAY is set */
	if (parser->depth == 2 && ZSTR_LEN(key) &&
		!strncmp(ID_COLUMN_NAME, ZSTR_VAL(key), sizeof(ID_COLUMN_NAME) - 1))
	{
		DBG_INF_FMT("FOUND %s", ID_COLUMN_NAME);
		if (Z_TYPE_P(zvalue) == IS_LONG)
		{
			DBG_INF_FMT("value=%llu", Z_LVAL_P(zvalue));
		}
		else if (Z_TYPE_P(zvalue) == IS_STRING)
		{
			DBG_INF_FMT("value=%*s", Z_STRLEN_P(zvalue), Z_STRVAL_P(zvalue));
		}
		status->found = TRUE;
		status->empty = FALSE;
	}
	else if (status->empty == TRUE)
	{
		status->empty = FALSE;
	}
	zend_string_release(key);
	zval_dtor(zvalue);
	zval_dtor(object);
	DBG_RETURN(status->found ? FAILURE : SUCCESS);
}
/* }}} */


/* {{{ xmysqlnd_json_string_find_id */
static enum_func_status
xmysqlnd_json_string_find_id(const MYSQLND_CSTRING json, zend_long options, zend_long depth, struct st_parse_for_id_status * status)
{
	php_json_parser_methods own_methods;
	struct my_php_json_parser parser;
	zval return_value;
	DBG_ENTER("xmysqlnd_json_string_find_id");
	ZVAL_UNDEF(&return_value);

	php_json_parser_init(&parser.parser, &return_value, (char *) json.s, json.l, options, depth);
	own_methods = parser.parser.methods;
	own_methods.object_update = xmysqlnd_json_parser_object_update;

	php_json_parser_init_ex(&parser.parser, &return_value, (char *) json.s, json.l, options, depth, &own_methods);
	status->found = FALSE;
	status->empty = TRUE;
	parser.status = status;

	if (php_json_parse(&parser.parser))
	{
		if (!status->found)
		{
			//		JSON_G(error_code) = php_json_parser_error_code(&parser);
			DBG_RETURN(FAIL);
		}
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_table::insert */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_table, insert)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__INSERT * op)
{
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_table::opinsert");
	if (!op || FAIL == xmysqlnd_crud_table_insert__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_insert__is_initialized(op))
	{
		XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__table_insert table_insert = msg_factory.get__table_insert(&msg_factory);
		if (PASS == table_insert.send_insert_request(&table_insert, xmysqlnd_crud_table_insert__get_protobuf_message(op)))
		{
			//ret = table_insert.read_response(&table_insert);

			XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
			XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
			stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != NULL ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::opdelete */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_table, opdelete)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__DELETE * op)
{
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_table::opdelete");
	if (!op || FAIL == xmysqlnd_crud_table_delete__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_delete__is_initialized(op))
	{
		XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == table_ud.send_delete_request(&table_ud, xmysqlnd_crud_table_delete__get_protobuf_message(op)))
		{
			//ret = table_ud.read_response(&table_ud);
			XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
			XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
			stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != NULL ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::update */
static XMYSQLND_NODE_STMT *
XMYSQLND_METHOD(xmysqlnd_node_table, update)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__UPDATE * op)
{
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_table::update");
	if (!op || FAIL == xmysqlnd_crud_table_update__finalize_bind(op))
	{
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_table_update__is_initialized(op))
	{
		XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud table_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == table_ud.send_update_request(&table_ud, xmysqlnd_crud_table_update__get_protobuf_message(op)))
		{
			//ret = table_ud.read_response(&table_ud);
			XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
			XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
			stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != NULL ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::select */
static struct st_xmysqlnd_node_stmt *
XMYSQLND_METHOD(xmysqlnd_node_table, select)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__SELECT * op)
{
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_table::select");
	if (!op || FAIL == xmysqlnd_crud_table_select__finalize_bind(op))
	{
		DBG_RETURN(stmt);
	}
	if (xmysqlnd_crud_table_select__is_initialized(op))
	{
		XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
		stmt = session->m->create_statement_object(session);
		if (FAIL == stmt->data->m.send_raw_message(stmt, xmysqlnd_crud_table_select__get_protobuf_message(op), session->data->stats, session->data->error_info))
		{
			xmysqlnd_node_stmt_free(stmt, session->data->stats, session->data->error_info);
			stmt = NULL;
		}
	}
	DBG_RETURN(stmt);
}
/* }}} */

/* {{{ xmysqlnd_node_table::get_reference */
static XMYSQLND_NODE_TABLE *
XMYSQLND_METHOD(xmysqlnd_node_table, get_reference)(XMYSQLND_NODE_TABLE * const table)
{
	DBG_ENTER("xmysqlnd_node_table::get_reference");
	++table->data->refcount;
	DBG_INF_FMT("table=%p new_refcount=%u", table, table->data->refcount);
	DBG_RETURN(table);
}
/* }}} */


/* {{{ xmysqlnd_node_table::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, free_reference)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_table::free_reference");
	DBG_INF_FMT("table=%p old_refcount=%u", table, table->data->refcount);
	if (!(--table->data->refcount)) {
		table->data->m.dtor(table, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_table, free_contents)(XMYSQLND_NODE_TABLE * const table)
{
	const zend_bool pers = table->data->persistent;
	DBG_ENTER("xmysqlnd_node_table::free_contents");
	if (table->data->table_name.s) {
		mnd_pefree(table->data->table_name.s, pers);
		table->data->table_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_table::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_table, dtor)(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_table::dtor");
	if (table) {
		table->data->m.free_contents(table);

		xmysqlnd_node_schema_free(table->data->schema, stats, error_info);

		mnd_pefree(table->data, table->data->persistent);
		mnd_pefree(table, table->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_table)
	XMYSQLND_METHOD(xmysqlnd_node_table, init),
	XMYSQLND_METHOD(xmysqlnd_node_table, exists_in_database),
	XMYSQLND_METHOD(xmysqlnd_node_table, count),

	XMYSQLND_METHOD(xmysqlnd_node_table, insert),
	XMYSQLND_METHOD(xmysqlnd_node_table, opdelete),
	XMYSQLND_METHOD(xmysqlnd_node_table, update),
	XMYSQLND_METHOD(xmysqlnd_node_table, select),

	XMYSQLND_METHOD(xmysqlnd_node_table, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_table, dtor),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_table);


/* {{{ xmysqlnd_node_table_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_TABLE *
xmysqlnd_node_table_create(XMYSQLND_NODE_SCHEMA * schema,
						   const MYSQLND_CSTRING table_name,
						   const zend_bool persistent,
						   const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
						   MYSQLND_STATS * const stats,
						   MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_TABLE * ret = NULL;
	DBG_ENTER("xmysqlnd_node_table_create");
	if (table_name.s && table_name.l) {
		ret = object_factory->get_node_table(object_factory, schema, table_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->data->m.get_reference(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_table_free(XMYSQLND_NODE_TABLE * const table, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_table_free");
	DBG_INF_FMT("table=%p  table->data=%p  dtor=%p", table, table? table->data:NULL, table? table->data->m.dtor:NULL);
	if (table) {
		table->data->m.free_reference(table, stats, error_info);
	}
	DBG_VOID_RETURN;
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
