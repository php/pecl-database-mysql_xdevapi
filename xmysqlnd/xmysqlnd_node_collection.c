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
#include "xmysqlnd_node_collection.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_utils.h"

/* {{{ xmysqlnd_node_collection::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, init)(XMYSQLND_NODE_COLLECTION * const collection,
										  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
										  XMYSQLND_NODE_SCHEMA * const schema,
										  const MYSQLND_CSTRING collection_name,
										  MYSQLND_STATS * const stats,
										  MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_node_collection::init");
	if (!(collection->data->schema = schema->data->m.get_reference(schema))) {
		return FAIL;
	}
	collection->data->collection_name = mnd_dup_cstring(collection_name, collection->data->persistent);
	DBG_INF_FMT("name=[%d]%*s", collection->data->collection_name.l, collection->data->collection_name.l, collection->data->collection_name.s);

	collection->data->object_factory = object_factory;

	DBG_RETURN(PASS);
}
/* }}} */


struct st_collection_exists_in_database_var_binder_ctx
{
	const MYSQLND_CSTRING schema_name;
	const MYSQLND_CSTRING collection_name;
	unsigned int counter;
};


/* {{{ collection_op_var_binder */
static const enum_hnd_func_status
collection_op_var_binder(
	void * context, 
	XMYSQLND_NODE_SESSION * session, 
	XMYSQLND_STMT_OP__EXECUTE * const stmt_execute)
{
	enum_hnd_func_status ret = HND_FAIL;
	struct st_collection_exists_in_database_var_binder_ctx * ctx = (struct st_collection_exists_in_database_var_binder_ctx *) context;
	const MYSQLND_CSTRING * param = NULL;
	DBG_ENTER("collection_op_var_binder");
	switch (ctx->counter) {
		case 0:
			param = &ctx->schema_name;
			ret = HND_AGAIN;
			goto bind;
		case 1:{
			param = &ctx->collection_name;
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


struct st_collection_exists_in_database_ctx
{
	const MYSQLND_CSTRING expected_collection_name;
	const MYSQLND_CSTRING expected_object_type;
	zval* exists;
};


/* {{{ collection_xplugin_op_on_row */
static const enum_hnd_func_status
collection_xplugin_op_on_row(
	void * context,
	XMYSQLND_NODE_SESSION * const session,
	XMYSQLND_NODE_STMT * const stmt,
	const XMYSQLND_NODE_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	struct st_collection_exists_in_database_ctx * ctx = (struct st_collection_exists_in_database_ctx *) context;
	DBG_ENTER("collection_xplugin_op_on_row");
	if (ctx && row) {
		const MYSQLND_CSTRING object_name = { Z_STRVAL(row[0]), Z_STRLEN(row[0]) };
		const MYSQLND_CSTRING object_type = { Z_STRVAL(row[1]), Z_STRLEN(row[1]) };

		if (equal_mysqlnd_cstr(&object_name, &ctx->expected_collection_name)
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


/* {{{ xmysqlnd_node_collection::exists_in_database */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, exists_in_database)(
	XMYSQLND_NODE_COLLECTION * const collection,
	struct st_xmysqlnd_node_session_on_error_bind on_error, 
	zval* exists)
{
	DBG_ENTER("xmysqlnd_node_collection::exists_in_database");
	ZVAL_FALSE(exists);

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"list_objects", sizeof("list_objects") - 1 };
	XMYSQLND_NODE_SCHEMA * schema = collection->data->schema;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	struct st_collection_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		mnd_str2c(collection->data->collection_name),
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	struct st_collection_exists_in_database_ctx on_row_ctx = { 
		mnd_str2c(collection->data->collection_name),
		xmysqlnd_object_type_filter__collection,
		exists
	};

	const struct st_xmysqlnd_node_session_on_row_bind on_row = { collection_xplugin_op_on_row, &on_row_ctx };

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


struct st_collection_sql_single_result_ctx
{
	zval* result;
};


/* {{{ collection_xplugin_op_on_row */
static const enum_hnd_func_status
collection_sql_single_result_op_on_row(
	void * context,
	XMYSQLND_NODE_SESSION * const session,
	XMYSQLND_NODE_STMT * const stmt,
	const XMYSQLND_NODE_STMT_RESULT_META * const meta,
	const zval * const row,
	MYSQLND_STATS * const stats,
	MYSQLND_ERROR_INFO * const error_info)
{
	struct st_collection_sql_single_result_ctx * ctx = (struct st_collection_sql_single_result_ctx *) context;
	DBG_ENTER("collection_xplugin_op_on_row");
	if (ctx && row) {
		ZVAL_COPY_VALUE(ctx->result, &row[0]);
	}
	DBG_RETURN(HND_AGAIN);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::count */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, count)(
	XMYSQLND_NODE_COLLECTION * const collection,
	struct st_xmysqlnd_node_session_on_error_bind on_error, 
	zval* counter)
{
	DBG_ENTER("xmysqlnd_node_collection::count");
	ZVAL_LONG(counter, 0);

	enum_func_status ret;

	XMYSQLND_NODE_SCHEMA * schema = collection->data->schema;
	XMYSQLND_NODE_SESSION * session = schema->data->session;

	char* query_str;
	mnd_sprintf(&query_str, 0, "SELECT COUNT(*) FROM %s.%s", schema->data->schema_name.s, collection->data->collection_name.s);
	if (!query_str) {
		DBG_RETURN(FAIL);
	}
	const MYSQLND_CSTRING query = {query_str, strlen(query_str)};

	struct st_collection_exists_in_database_var_binder_ctx var_binder_ctx = {
		mnd_str2c(schema->data->schema_name),
		mnd_str2c(collection->data->collection_name),
		0
	};
	const struct st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_op_var_binder, &var_binder_ctx };

	struct st_collection_sql_single_result_ctx on_row_ctx = { 
		counter
	};

	const struct st_xmysqlnd_node_session_on_row_bind on_row = { collection_sql_single_result_op_on_row, &on_row_ctx };

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
	zend_bool found:1;
	zend_bool empty:1;
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
		if (Z_TYPE_P(zvalue) == IS_LONG) {
			DBG_INF_FMT("value=%llu", Z_LVAL_P(zvalue));
		} else if (Z_TYPE_P(zvalue) == IS_STRING) {
			DBG_INF_FMT("value=%*s", Z_STRLEN_P(zvalue), Z_STRVAL_P(zvalue));
		}
		status->found = TRUE;
		status->empty = FALSE;
	} else if (status->empty == TRUE) {
		status->empty = FALSE;
	}
	zend_string_release(key);
	zval_dtor(zvalue);
	DBG_RETURN(status->found? FAILURE : SUCCESS);
}
/* }}} */

/* {{{ xmysqlnd_json_parser_object_end */
static int
xmysqlnd_json_parser_object_end(php_json_parser *parser, zval *object)
{
	DBG_ENTER("xmysqlnd_json_parser_object_end");
	zval_dtor(object);
	return SUCCESS;
}

/* {{{ xmysqlnd_json_string_find_id */
static enum_func_status
xmysqlnd_json_string_find_id(const MYSQLND_CSTRING json, zend_long options, zend_long depth, struct st_parse_for_id_status * status)
{
	php_json_parser_methods own_methods;
	struct my_php_json_parser parser;
	zval return_value;
	DBG_ENTER("xmysqlnd_json_string_find_id");
	ZVAL_UNDEF(&return_value);

	php_json_parser_init(&parser.parser, &return_value, (char *)json.s, json.l, options, depth);
	own_methods = parser.parser.methods;
	own_methods.object_update = xmysqlnd_json_parser_object_update;
	own_methods.object_end = xmysqlnd_json_parser_object_end;
	
	php_json_parser_init_ex(&parser.parser, &return_value, (char *)json.s, json.l, options, depth, &own_methods);
	status->found = FALSE;
	status->empty = TRUE;
	parser.status = status;

	if (php_json_parse(&parser.parser)) {
		if (!status->found) {
	//		JSON_G(error_code) = php_json_parser_error_code(&parser);
			DBG_RETURN(FAIL);
		}
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::add */
static XMYSQLND_NODE_STMT * 
XMYSQLND_METHOD(xmysqlnd_node_collection, add)(XMYSQLND_NODE_COLLECTION * const collection, const MYSQLND_CSTRING json)
{
	struct st_parse_for_id_status status;
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_collection::add");
	DBG_INF_FMT("json=%*s", json.l, json.s);
	if (!json.l) {
		DBG_RETURN(NULL);
	}
	if (PASS == xmysqlnd_json_string_find_id(json, 0, 0, &status)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		MYSQLND_STRING to_add = { (char *) json.s, json.l };
		if (!status.found) {
			const MYSQLND_CSTRING uuid = session->m->get_uuid(session);

			if (UNEXPECTED(status.empty)) {
				to_add.s = mnd_emalloc(2 /*braces*/ + sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1) ; /* allocate a bit more */
				if (!to_add.s) {
					DBG_RETURN(NULL);
				}
				{
					char * p = to_add.s;
					*p++ = '{';
					/* Here STARTS the common part */
					memcpy(p, ID_TEMPLATE_PREFIX, sizeof(ID_TEMPLATE_PREFIX) - 1);
					p += sizeof(ID_TEMPLATE_PREFIX) - 1;
					memcpy(p, uuid.s, uuid.l);
					p += uuid.l;
					memcpy(p, ID_TEMPLATE_SUFFIX, sizeof(ID_TEMPLATE_SUFFIX) - 1);
					p += sizeof(ID_TEMPLATE_SUFFIX) - 1;
					*p = '\0';
					to_add.l = p - to_add.s;
					/* Here ENDS the common part */
				}
			} else {
				const char * last = json.s + json.l - 1;
				while (last >= json.s && *last != '}') {
					--last;
				}
				if (last < json.s) {
					DBG_RETURN(NULL);
				}
				to_add.s = mnd_emalloc(json.l + 1 /*comma */+ sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1) ; /* allocate a bit more */
				if (!to_add.s) {
					DBG_RETURN(NULL);
				}
				{
					char * p = to_add.s;
					memcpy(p, json.s, last - json.s);
					p += last - json.s;
					*p++ = ',';
					/* Here STARTS the common part */
					memcpy(p, ID_TEMPLATE_PREFIX, sizeof(ID_TEMPLATE_PREFIX) - 1);
					p += sizeof(ID_TEMPLATE_PREFIX) - 1;
					memcpy(p, uuid.s, uuid.l);
					p += uuid.l;
					memcpy(p, ID_TEMPLATE_SUFFIX, sizeof(ID_TEMPLATE_SUFFIX) - 1);
					p += sizeof(ID_TEMPLATE_SUFFIX) - 1;
					*p = '\0';
					to_add.l = p - to_add.s;
					/* Here ENDS the common part */
				}
			}
		}
		DBG_INF_FMT("json=%*s", to_add.l, to_add.s);
		{
			const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
			struct st_xmysqlnd_msg__collection_add collection_add = msg_factory.get__collection_add(&msg_factory);
			enum_func_status request_ret = collection_add.send_request(&collection_add,
												 mnd_str2c(collection->data->schema->data->schema_name),
												 mnd_str2c(collection->data->collection_name),
												 mnd_str2c(to_add));
			if (PASS == request_ret) {
				//ret = collection_add.read_response(&collection_add);
				XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
				XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
				stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
				ret = stmt;
			}
			DBG_INF(ret != NULL? "PASS":"FAIL");
		}
		if (to_add.s != json.s) {
			efree(to_add.s);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::remove */
static XMYSQLND_NODE_STMT * 
XMYSQLND_METHOD(xmysqlnd_node_collection, remove)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__REMOVE * op)
{
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_collection::remove");
	if (!op || FAIL == xmysqlnd_crud_collection_remove__finalize_bind(op)) {
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_collection_remove__is_initialized(op)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == collection_ud.send_delete_request(&collection_ud, xmysqlnd_crud_collection_remove__get_protobuf_message(op))) {
			//ret = collection_ud.read_response(&collection_ud);
			XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
			XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
			stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != NULL? "PASS":"FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::modify */
static XMYSQLND_NODE_STMT * 
XMYSQLND_METHOD(xmysqlnd_node_collection, modify)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__MODIFY * op)
{
	XMYSQLND_NODE_STMT * ret = NULL;
	DBG_ENTER("xmysqlnd_node_collection::modify");
	if (!op || FAIL == xmysqlnd_crud_collection_modify__finalize_bind(op)) {
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_collection_modify__is_initialized(op)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
		if (PASS == collection_ud.send_update_request(&collection_ud, xmysqlnd_crud_collection_modify__get_protobuf_message(op))) {
			//ret = collection_ud.read_response(&collection_ud);
			XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
			XMYSQLND_NODE_STMT * stmt = session->m->create_statement_object(session);
			stmt->data->msg_stmt_exec = msg_factory.get__sql_stmt_execute(&msg_factory);
			ret = stmt;
		}
		DBG_INF(ret != NULL? "PASS":"FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::find */
static struct st_xmysqlnd_node_stmt *
XMYSQLND_METHOD(xmysqlnd_node_collection, find)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__FIND * op)
{
	XMYSQLND_NODE_STMT * stmt = NULL;
	DBG_ENTER("xmysqlnd_node_collection::find");
	if (!op || FAIL == xmysqlnd_crud_collection_find__finalize_bind(op)) {
		DBG_RETURN(stmt);
	}
	if (xmysqlnd_crud_collection_find__is_initialized(op)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		stmt = session->m->create_statement_object(session);
		if (FAIL == stmt->data->m.send_raw_message(stmt, xmysqlnd_crud_collection_find__get_protobuf_message(op), session->data->stats, session->data->error_info)) {
			xmysqlnd_node_stmt_free(stmt, session->data->stats, session->data->error_info);
			stmt = NULL;			
		}
	}
	DBG_RETURN(stmt);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::get_reference */
static XMYSQLND_NODE_COLLECTION *
XMYSQLND_METHOD(xmysqlnd_node_collection, get_reference)(XMYSQLND_NODE_COLLECTION * const collection)
{
	DBG_ENTER("xmysqlnd_node_collection::get_reference");
	++collection->data->refcount;
	DBG_INF_FMT("collection=%p new_refcount=%u", collection, collection->data->refcount);
	DBG_RETURN(collection);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::free_reference */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, free_reference)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_collection::free_reference");
	DBG_INF_FMT("collection=%p old_refcount=%u", collection, collection->data->refcount);
	if (!(--collection->data->refcount)) {
		collection->data->m.dtor(collection, stats, error_info);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_node_collection, free_contents)(XMYSQLND_NODE_COLLECTION * const collection)
{
	const zend_bool pers = collection->data->persistent;
	DBG_ENTER("xmysqlnd_node_collection::free_contents");
	if (collection->data->collection_name.s) {
		mnd_pefree(collection->data->collection_name.s, pers);
		collection->data->collection_name.s = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_node_collection::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_node_collection, dtor)(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_collection::dtor");
	if (collection) {
		collection->data->m.free_contents(collection);

		xmysqlnd_node_schema_free(collection->data->schema, stats, error_info);

		mnd_pefree(collection->data, collection->data->persistent);
		mnd_pefree(collection, collection->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_node_collection)
	XMYSQLND_METHOD(xmysqlnd_node_collection, init),
	XMYSQLND_METHOD(xmysqlnd_node_collection, exists_in_database),
	XMYSQLND_METHOD(xmysqlnd_node_collection, count),

	XMYSQLND_METHOD(xmysqlnd_node_collection, add),
	XMYSQLND_METHOD(xmysqlnd_node_collection, remove),
	XMYSQLND_METHOD(xmysqlnd_node_collection, modify),
	XMYSQLND_METHOD(xmysqlnd_node_collection, find),

	XMYSQLND_METHOD(xmysqlnd_node_collection, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_collection, dtor),
MYSQLND_CLASS_METHODS_END;

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_collection);


/* {{{ xmysqlnd_node_collection_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_COLLECTION *
xmysqlnd_node_collection_create(XMYSQLND_NODE_SCHEMA * schema,
								const MYSQLND_CSTRING collection_name,
								const zend_bool persistent,
								const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								MYSQLND_STATS * const stats,
								MYSQLND_ERROR_INFO * const error_info)
{
	XMYSQLND_NODE_COLLECTION * ret = NULL;
	DBG_ENTER("xmysqlnd_node_collection_create");
	if (collection_name.s && collection_name.l) {
		ret = object_factory->get_node_collection(object_factory, schema, collection_name, persistent, stats, error_info);
		if (ret) {
			ret = ret->data->m.get_reference(ret);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_node_collection_free(XMYSQLND_NODE_COLLECTION * const collection, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_node_collection_free");
	DBG_INF_FMT("collection=%p  collection->data=%p  dtor=%p", collection, collection? collection->data:NULL, collection? collection->data->m.dtor:NULL);
	if (collection) {
		collection->data->m.free_reference(collection, stats, error_info);
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
