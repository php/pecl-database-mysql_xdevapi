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
#include "ext/json/php_json_parser.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_collection.h"

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
	zval_dtor(object);
	DBG_RETURN(status->found? FAILURE : SUCCESS);
}
/* }}} */


/* {{{ xmysqlnd_json_string_find_id */
static enum_func_status
xmysqlnd_json_string_find_id(const MYSQLND_CSTRING json, zend_long options, zend_long depth, struct st_parse_for_id_status * status) /* {{{ */
{
	php_json_parser_methods own_methods;
	struct my_php_json_parser parser;
	zval return_value;
	DBG_ENTER("xmysqlnd_json_string_find_id");
	ZVAL_UNDEF(&return_value);

	php_json_parser_init(&parser.parser, &return_value, (char *)json.s, json.l, options, depth);
	own_methods = parser.parser.methods;
	own_methods.object_update = xmysqlnd_json_parser_object_update;
	
	php_json_parser_init_ex(&parser.parser, &return_value, (char *)json.s, json.l, options, depth, &own_methods);
	status->found = FALSE;
	status->empty = TRUE;
	parser.status = status;

	if (php_json_yyparse(&parser.parser)) {
		if (!status->found) {
	//		JSON_G(error_code) = php_json_parser_error_code(&parser);
			DBG_RETURN(FAIL);
		}
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::add */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, add)(XMYSQLND_NODE_COLLECTION * const collection, const MYSQLND_CSTRING json)
{
	struct st_parse_for_id_status status;
	enum_func_status ret = PASS;
	DBG_ENTER("xmysqlnd_node_collection::add");
	DBG_INF_FMT("json=%*s", json.l, json.s);
	if (!json.l) {
		DBG_RETURN(FAIL);
	}
	ret = xmysqlnd_json_string_find_id(json, 0, 0, &status);
	if (PASS == ret) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		MYSQLND_STRING to_add = { (char *) json.s, json.l };
		if (!status.found) {
			const MYSQLND_CSTRING uuid = session->m->get_uuid(session);

			if (UNEXPECTED(status.empty)) {
				to_add.s = mnd_emalloc(2 /*braces*/ + sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1) ; /* allocate a bit more */
				if (!to_add.s) {
					DBG_RETURN(FAIL);
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
					DBG_RETURN(FAIL);
				}
				to_add.s = mnd_emalloc(json.l + 1 /*comma */+ sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1) ; /* allocate a bit more */
				if (!to_add.s) {
					DBG_RETURN(FAIL);
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
			struct st_xmysqlnd_msg__collection_insert collection_insert = msg_factory.get__collection_insert(&msg_factory);
			ret = collection_insert.send_request(&collection_insert,
												 mnd_str2c(collection->data->schema->data->schema_name),
												 mnd_str2c(collection->data->collection_name),
												 mnd_str2c(to_add));
			if (PASS == ret) {
				ret = collection_insert.read_response(&collection_insert);
			}
			DBG_INF(ret == PASS? "PASS":"FAIL");
		}
		if (to_add.s != json.s) {
			efree(to_add.s);
		}
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::remove */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, remove)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__REMOVE * op)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_collection::remove");
	if (!op || FAIL == xmysqlnd_crud_collection_remove__finalize_bind(op)) {
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_collection_remove__is_initialized(op)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
		ret = collection_ud.send_delete_request(&collection_ud, xmysqlnd_crud_collection_remove__get_protobuf_message(op));
		if (PASS == ret) {
			ret = collection_ud.read_response(&collection_ud);
		}
		DBG_INF(ret == PASS? "PASS":"FAIL");
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_node_collection::modify */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_node_collection, modify)(XMYSQLND_NODE_COLLECTION * const collection, XMYSQLND_CRUD_COLLECTION_OP__MODIFY * op)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("xmysqlnd_node_collection::modify");
	if (!op || FAIL == xmysqlnd_crud_collection_modify__finalize_bind(op)) {
		DBG_RETURN(ret);
	}
	if (xmysqlnd_crud_collection_modify__is_initialized(op)) {
		XMYSQLND_NODE_SESSION * session = collection->data->schema->data->session;
		const struct st_xmysqlnd_message_factory msg_factory = xmysqlnd_get_message_factory(&session->data->io, session->data->stats, session->data->error_info);
		struct st_xmysqlnd_msg__collection_ud collection_ud = msg_factory.get__collection_ud(&msg_factory);
		ret = collection_ud.send_update_request(&collection_ud, xmysqlnd_crud_collection_modify__get_protobuf_message(op));
		if (PASS == ret) {
			ret = collection_ud.read_response(&collection_ud);
		}
		DBG_INF(ret == PASS? "PASS":"FAIL");
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

	XMYSQLND_METHOD(xmysqlnd_node_collection, add),
	XMYSQLND_METHOD(xmysqlnd_node_collection, remove),
	XMYSQLND_METHOD(xmysqlnd_node_collection, modify),
	XMYSQLND_METHOD(xmysqlnd_node_collection, find),

	XMYSQLND_METHOD(xmysqlnd_node_collection, get_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_reference),
	XMYSQLND_METHOD(xmysqlnd_node_collection, free_contents),
	XMYSQLND_METHOD(xmysqlnd_node_collection, dtor),
MYSQLND_CLASS_METHODS_END;

PHPAPI MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_node_collection);


/* {{{ xmysqlnd_node_collection_create */
PHPAPI XMYSQLND_NODE_COLLECTION *
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
PHPAPI void
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
