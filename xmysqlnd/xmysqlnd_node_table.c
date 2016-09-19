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
#include "xmysqlnd_node_table.h"

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

	if (php_json_yyparse(&parser.parser))
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
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, insert)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__INSERT * op)
{
	enum_func_status ret = FAIL;
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
		ret = table_insert.send_insert_request(&table_insert, xmysqlnd_crud_table_insert__get_protobuf_message(op));
		if (PASS == ret)
		{
			ret = table_insert.read_response(&table_insert);
		}
		DBG_INF(ret == PASS ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);

	//struct st_parse_for_id_status status;
	//enum_func_status ret = PASS;
	//DBG_ENTER("xmysqlnd_node_table::insert");
	//DBG_INF_FMT("json=%*s", json.l, json.s);
	//if (!json.l)
	//{
	//	DBG_RETURN(FAIL);
	//}
	//ret = xmysqlnd_json_string_find_id(json, 0, 0, &status);
	//if (PASS == ret)
	//{
	//	XMYSQLND_NODE_SESSION * session = table->data->schema->data->session;
	//	MYSQLND_STRING to_add = {(char *) json.s, json.l};
	//	if (!status.found)
	//	{
	//		const MYSQLND_CSTRING uuid = session->m->get_uuid(session);

	//		if (UNEXPECTED(status.empty))
	//		{
	//			to_add.s = mnd_emalloc(2 /*braces*/ + sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1); /* allocate a bit more */
	//			if (!to_add.s)
	//			{
	//				DBG_RETURN(FAIL);
	//			}
	//			{
	//				char * p = to_add.s;
	//				*p++ = '{';
	//				/* Here STARTS the common part */
	//				memcpy(p, ID_TEMPLATE_PREFIX, sizeof(ID_TEMPLATE_PREFIX) - 1);
	//				p += sizeof(ID_TEMPLATE_PREFIX) - 1;
	//				memcpy(p, uuid.s, uuid.l);
	//				p += uuid.l;
	//				memcpy(p, ID_TEMPLATE_SUFFIX, sizeof(ID_TEMPLATE_SUFFIX) - 1);
	//				p += sizeof(ID_TEMPLATE_SUFFIX) - 1;
	//				*p = '\0';
	//				to_add.l = p - to_add.s;
	//				/* Here ENDS the common part */
	//			}
	//		}
	//		else
	//		{
	//			const char * last = json.s + json.l - 1;
	//			while (last >= json.s && *last != '}')
	//			{
	//				--last;
	//			}
	//			if (last < json.s)
	//			{
	//				DBG_RETURN(FAIL);
	//			}
	//			to_add.s = mnd_emalloc(json.l + 1 /*comma */ + sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1); /* allocate a bit more */
	//			if (!to_add.s)
	//			{
	//				DBG_RETURN(FAIL);
	//			}
	//			{
	//				char * p = to_add.s;
	//				memcpy(p, json.s, last - json.s);
	//				p += last - json.s;
	//				*p++ = ',';
	//				/* Here STARTS the common part */
	//				memcpy(p, ID_TEMPLATE_PREFIX, sizeof(ID_TEMPLATE_PREFIX) - 1);
	//				p += sizeof(ID_TEMPLATE_PREFIX) - 1;
	//				memcpy(p, uuid.s, uuid.l);
	//				p += uuid.l;
	//				memcpy(p, ID_TEMPLATE_SUFFIX, sizeof(ID_TEMPLATE_SUFFIX) - 1);
	//				p += sizeof(ID_TEMPLATE_SUFFIX) - 1;
	//				*p = '\0';
	//				to_add.l = p - to_add.s;
	//				/* Here ENDS the common part */
	//			}
	//		}
	//	}
	//	DBG_INF_FMT("json=%*s", to_add.l, to_add.s);
	//	{
	//		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
	//		struct st_xmysqlnd_msg__collection_add table_insert = msg_factory.get__table_insert(&msg_factory);
	//		ret = table_insert.send_request(&table_insert,
	//			mnd_str2c(table->data->schema->data->schema_name),
	//			mnd_str2c(table->data->table_name),
	//			mnd_str2c(to_add));
	//		if (PASS == ret)
	//		{
	//			ret = table_insert.read_response(&table_insert);
	//		}
	//		DBG_INF(ret == PASS ? "PASS" : "FAIL");
	//	}
	//	if (to_add.s != json.s)
	//	{
	//		efree(to_add.s);
	//	}
	//}
	//DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::opdelete */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, opdelete)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__DELETE * op)
{
	enum_func_status ret = FAIL;
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
		ret = table_ud.send_delete_request(&table_ud, xmysqlnd_crud_table_delete__get_protobuf_message(op));
		if (PASS == ret)
		{
			ret = table_ud.read_response(&table_ud);
		}
		DBG_INF(ret == PASS ? "PASS" : "FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_table::update */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_table, update)(XMYSQLND_NODE_TABLE * const table, XMYSQLND_CRUD_TABLE_OP__UPDATE * op)
{
	enum_func_status ret = FAIL;
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
		ret = table_ud.send_update_request(&table_ud, xmysqlnd_crud_table_update__get_protobuf_message(op));
		if (PASS == ret)
		{
			ret = table_ud.read_response(&table_ud);
		}
		DBG_INF(ret == PASS ? "PASS" : "FAIL");
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

	XMYSQLND_METHOD(xmysqlnd_node_table, insert),
	XMYSQLND_METHOD(xmysqlnd_node_table, opdelete),
	XMYSQLND_METHOD(xmysqlnd_node_table, update),
	XMYSQLND_METHOD(xmysqlnd_node_table, select),

	XMYSQLND_METHOD(xmysqlnd_node_table, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_table, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_table, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_table);


/* {{{ xmysqlnd_node_table_create */
PHPAPI XMYSQLND_NODE_TABLE *
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
PHPAPI void
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
