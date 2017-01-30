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
extern "C" {
#include <php.h>
#undef ERROR
#include <zend_exceptions.h>		/* for throwing "not implemented" */
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
#include <zend_smart_str.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include <xmysqlnd/xmysqlnd.h>
#include <xmysqlnd/xmysqlnd_node_session.h>
#include <xmysqlnd/xmysqlnd_node_schema.h>
#include <xmysqlnd/xmysqlnd_node_stmt.h>
#include <xmysqlnd/xmysqlnd_node_collection.h>
#include <xmysqlnd/xmysqlnd_crud_collection_commands.h>
#include <phputils/allocator.h>
#include <phputils/object.h>
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_node_collection__add.h"
#include "mysqlx_exception.h"

static zend_class_entry *mysqlx_node_collection__add_class_entry;

#define DONT_ALLOW_NULL 0
#define NO_PASS_BY_REF 0

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__add__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


struct st_mysqlx_node_collection__add : public mysqlx::phputils::custom_allocable
{
	XMYSQLND_NODE_COLLECTION * collection;
	XMYSQLND_CRUD_COLLECTION_OP__ADD* crud_op;
	zval * docs;
	int  num_of_docs;
};


#define MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_collection__add *) mysqlx_object->ptr; \
	if (!(_to) || !(_to)->collection) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_collection__add::__construct */
static
PHP_METHOD(mysqlx_node_collection__add, __construct)
{
}
/* }}} */


/* {{{ execute_statement */
static enum_func_status
execute_statement(XMYSQLND_NODE_STMT * stmt,zval* return_value)
{
	enum_func_status ret = FAIL;
	if (stmt) {
		zval stmt_zv;
		ZVAL_UNDEF(&stmt_zv);
		mysqlx_new_node_stmt(&stmt_zv, stmt);
		if (Z_TYPE(stmt_zv) == IS_NULL) {
			xmysqlnd_node_stmt_free(stmt, NULL, NULL);
		}
		if (Z_TYPE(stmt_zv) == IS_OBJECT) {
			zval zv;
			ZVAL_UNDEF(&zv);
			zend_long flags = 0;
			mysqlx_node_statement_execute_read_response(Z_MYSQLX_P(&stmt_zv),
								flags, MYSQLX_RESULT, &zv);
			ZVAL_COPY(return_value, &zv);
			zval_dtor(&zv);
			ret = PASS;
		}
		zval_ptr_dtor(&stmt_zv);
	}
	return ret;
}
/* }}} */


#define ID_COLUMN_NAME		"_id"
#define ID_TEMPLATE_PREFIX	"\""ID_COLUMN_NAME"\":\""
#define ID_TEMPLATE_SUFFIX	"\"}"

struct st_parse_for_id_status
{
	zend_bool		found:1;
	zend_bool		empty:1;
	zend_bool		is_string:1;
};

struct my_php_json_parser {
	php_json_parser parser;
	struct st_parse_for_id_status * status;
	HashTable array_of_allocated_obj;
};


/* {{{ xmysqlnd_json_parser_object_update */
static int
xmysqlnd_json_parser_object_update(php_json_parser *parser,
					zval *object,
					zend_string *key,
					zval *zvalue)
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
			status->is_string = TRUE;
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


/* {{{ xmysqlnd_json_parser_object_create */
static int
xmysqlnd_json_parser_object_create(php_json_parser *parser,
								zval *object)
{
	struct my_php_json_parser * php_json_parser = (struct my_php_json_parser*)parser;
	int ret = 0;
	if (parser->scanner.options & PHP_JSON_OBJECT_AS_ARRAY) {
		ret = array_init(object);
	} else {
		ret = object_init(object);
	}
	zend_hash_next_index_insert(&php_json_parser->array_of_allocated_obj,
								object);
	return ret;
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

	zend_hash_init(&parser.array_of_allocated_obj,0,NULL,ZVAL_PTR_DTOR,FALSE);
	php_json_parser_init(&parser.parser,
					&return_value, (char *)json.s, json.l, options, depth);
	own_methods = parser.parser.methods;
	own_methods.object_create = xmysqlnd_json_parser_object_create;
	own_methods.object_update = xmysqlnd_json_parser_object_update;
	own_methods.object_end = xmysqlnd_json_parser_object_end;

	php_json_parser_init_ex(&parser.parser,
					&return_value, (char *)json.s, json.l, options, depth, &own_methods);
	status->found = FALSE;
	status->empty = TRUE;
	status->is_string = FALSE;
	parser.status = status;

	if (php_json_parse(&parser.parser)) {
		if (!status->found) {
	//		JSON_G(error_code) = php_json_parser_error_code(&parser);
			DBG_RETURN(FAIL);
		}
		zend_hash_destroy(&parser.array_of_allocated_obj);
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ add_unique_id_to_json */
static enum_func_status
add_unique_id_to_json(XMYSQLND_NODE_SESSION *session,
			const struct st_parse_for_id_status *status,
			MYSQLND_STRING* to_add,
			const MYSQLND_CSTRING* json)
{
	enum_func_status ret = FAIL;
	char * p = NULL;
	const MYSQLND_CSTRING uuid = session->m->get_uuid(session);

	if (UNEXPECTED(status->empty)) {
		to_add->s = static_cast<char*>(mnd_emalloc(2 /*braces*/ + sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1)); /* allocate a bit more */
		if (to_add->s) {
			p = to_add->s;
			*p++ = '{';
			ret = PASS;
		}
	} else {
		const char * last = json->s + json->l - 1;
		while (last >= json->s && *last != '}') {
			--last;
		}
		if (last >= json->s) {
			to_add->s = static_cast<char*>(mnd_emalloc(json->l + 1 /*comma */+ sizeof(ID_TEMPLATE_PREFIX) - 1 + sizeof(ID_TEMPLATE_SUFFIX) - 1 + XMYSQLND_UUID_LENGTH + 1)); /* allocate a bit more */
			if (to_add->s) {
				p = to_add->s;
				memcpy(p, json->s, last - json->s);
				p += last - json->s;
				*p++ = ',';
				ret = PASS;
			}
		}
	}
	if(ret == PASS && p!= NULL) {
		memcpy(p, ID_TEMPLATE_PREFIX, sizeof(ID_TEMPLATE_PREFIX) - 1);
		p += sizeof(ID_TEMPLATE_PREFIX) - 1;
		memcpy(p, uuid.s, uuid.l);
		p += uuid.l;
		memcpy(p, ID_TEMPLATE_SUFFIX, sizeof(ID_TEMPLATE_SUFFIX) - 1);
		p += sizeof(ID_TEMPLATE_SUFFIX) - 1;
		*p = '\0';
		to_add->l = p - to_add->s;
	}
	return ret;
}
/* }}} */


/* {{{ add_unique_id_to_json */
static MYSQLND_CSTRING
extract_document_id(const MYSQLND_STRING json,
					zend_bool id_is_string)
{
	MYSQLND_STRING res = { NULL, 0 };
	char * beg = NULL,
		 * end = json.s + json.l - 1;
	if( ( beg = strstr(json.s, ID_COLUMN_NAME) ) != NULL) {
		beg += sizeof( ID_COLUMN_NAME ) + 1;
		if( id_is_string ) {
			//Move to the first char after \"
			while( beg < end && *beg != '\"' )
				++beg;
			++beg;//Skip \"
		} else {
			//Skip the spaces, if any
			while( beg < end && *beg == ' ' )
				++beg;
		}
		if( beg < end ) {
			//Now we look for the termination mark
			char * cur = beg;
			if( id_is_string ) {
				while( cur != end ) {
					if( *cur == '\"' ) break;
					++cur;
				}
			} else {
				while( cur != end ) {
					if( *cur == ' ' || *cur == ',' || *cur == '}' ) break;
					++cur;
				}
			}
			end = cur;
			if( end >= beg ){
				res.l = ( end - beg);
				if( res.l > 0 && res.l <= 32 ) {
					res.s = static_cast<char*>(mnd_emalloc( res.l ));
					memcpy(res.s, beg, res.l);
				}
			}
		}
	}
	if( res.s == NULL ) {
		RAISE_EXCEPTION(err_msg_json_fail);
	}
	MYSQLND_CSTRING ret = {
		res.s,
		res.l
	};
	return ret;
}
/* }}} */


/* {{{ xmysqlnd_json_string_find_id */
static MYSQLND_CSTRING
assign_doc_id_to_json(XMYSQLND_NODE_SESSION * session,
			zval* doc) {
	enum_func_status ret = FAIL;
	struct st_parse_for_id_status status;
	zend_bool doc_id_string_type = FALSE;
	MYSQLND_STRING to_add = { NULL, 0 };
	MYSQLND_CSTRING doc_id = { NULL, 0 };
	//Better be sure, perhaps raise an exception if is not a string
	if( Z_TYPE_P(doc) == IS_STRING ){
		MYSQLND_CSTRING json = { Z_STRVAL_P(doc), Z_STRLEN_P(doc) };
		if( PASS == xmysqlnd_json_string_find_id(json, 0, 0, &status)) {
			to_add.s = (char *) json.s;
			to_add.l = json.l ;

			if (!status.found) {
				add_unique_id_to_json(session,
									  &status,
									  &to_add,
									  &json);
				doc_id_string_type = TRUE;
			} else {
				doc_id_string_type = status.is_string;
			}

			if( !status.found ) {
				zval_dtor(doc);
				ZVAL_STRINGL(doc,to_add.s,to_add.l);
			}

			doc_id =  extract_document_id(to_add,
								doc_id_string_type);

			if (to_add.s != json.s) {
				efree(to_add.s);
			}
		}
	} else {
		RAISE_EXCEPTION(err_msg_json_fail);
	}
	return doc_id;
}

/* }}} */

enum add_op_status
{
	ADD_SUCCESS,
	ADD_FAIL,
	ADD_NOOP
};

struct doc_add_op_return_status
{
	enum add_op_status return_status;
	MYSQLND_CSTRING  doc_id;
};

/* {{{ node_collection_add_string */
static struct doc_add_op_return_status
node_collection_add_string(struct st_mysqlx_node_collection__add * const object,
				zval* doc,
				zval* return_value) {
	struct doc_add_op_return_status ret = {
		ADD_FAIL,
		assign_doc_id_to_json(
				object->collection->data->schema->data->session,
				doc)
	};
	if( SUCCESS == xmysqlnd_crud_collection_add__add_doc(object->crud_op,doc) ) {
		ret.return_status = ADD_SUCCESS;
	}
	return ret;
}
/* }}} */


/* {{{ node_collection_add_object*/
static struct doc_add_op_return_status
node_collection_add_object_impl(struct st_mysqlx_node_collection__add * const object,
				zval* doc,
				zval* return_value) {
	smart_str buf = {0};
	JSON_G(error_code) = PHP_JSON_ERROR_NONE;
	JSON_G(encode_max_depth) = PHP_JSON_PARSER_DEFAULT_DEPTH;
	const int encode_flag = (Z_TYPE_P(doc) == IS_OBJECT) ? PHP_JSON_FORCE_OBJECT : 0;
	php_json_encode(&buf, doc, encode_flag);

	if (JSON_G(error_code) != PHP_JSON_ERROR_NONE) {
		smart_str_free(&buf);
		RAISE_EXCEPTION(err_msg_json_fail);
	}

	//TODO marines: there is fockup with lack of terminating zero, which makes troubles in
	// xmysqlnd_json_string_find_id, i.e. php_json_yyparse returns result != 0
	if (buf.s->len < buf.a)
	{
		buf.s->val[buf.s->len] = '\0';
	}
	zval new_doc;
	ZVAL_UNDEF(&new_doc);
	ZVAL_STRINGL(&new_doc, buf.s->val, buf.s->len);
	struct doc_add_op_return_status ret = {
		ADD_FAIL,
		assign_doc_id_to_json(
				object->collection->data->schema->data->session,
				&new_doc)
	};
	if( SUCCESS == xmysqlnd_crud_collection_add__add_doc(object->crud_op, &new_doc) ) {
		ret.return_status = ADD_SUCCESS;
	}
	smart_str_free(&buf);
	zval_dtor(&new_doc);
	return ret;
}
/* }}} */


/* {{{ node_collection_add_object*/
static struct doc_add_op_return_status
node_collection_add_object(struct st_mysqlx_node_collection__add * const object,
				zval* doc,
				zval* return_value) {
	return node_collection_add_object_impl(object,
								doc,return_value);
}
/* }}} */


/* {{{ node_collection_add_array*/
static struct doc_add_op_return_status
node_collection_add_array(struct st_mysqlx_node_collection__add * const object,
				zval* doc,
				zval* return_value) {
	struct doc_add_op_return_status ret = { ADD_FAIL, NULL };
	if( zend_hash_num_elements(Z_ARRVAL_P(doc)) == 0 ) {
		ret.return_status = ADD_NOOP;
	} else {
		ret = node_collection_add_object_impl(object,
								doc,return_value);
	}
	return ret;
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__add::execute() */
static
PHP_METHOD(mysqlx_node_collection__add, execute)
{
	enum_func_status execute_ret_status = PASS;
	struct st_mysqlx_node_collection__add * object;
	zval * object_zv;
	int i = 0, noop_cnt = 0,cur_doc_id_idx = 0;

	DBG_ENTER("mysqlx_node_collection__add::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv,
												mysqlx_node_collection__add_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLLECTION_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if ( !object->collection ) {
		DBG_VOID_RETURN;
	}

	MYSQLND_CSTRING * doc_ids = static_cast<MYSQLND_CSTRING*>(mnd_ecalloc( object->num_of_docs, sizeof(MYSQLND_CSTRING) ));
	if( doc_ids == NULL ) {
		execute_ret_status = FAIL;
	} else {
		struct doc_add_op_return_status ret = { ADD_SUCCESS , NULL };
		for(i = 0 ; i < object->num_of_docs && ret.return_status != ADD_FAIL ; ++i ) {
			ret.return_status = ADD_FAIL;
			switch(Z_TYPE(object->docs[i])) {
			case IS_STRING:
				ret = node_collection_add_string(object,
								&object->docs[i],return_value);
				break;
			case IS_ARRAY:
				ret = node_collection_add_array(object,
								&object->docs[i],return_value);
				break;
			case IS_OBJECT:
				ret = node_collection_add_object(object,
								&object->docs[i],return_value);
				break;
			}
			if( ret.return_status == ADD_NOOP ) {
				++noop_cnt;
			} else {
				doc_ids[ cur_doc_id_idx++ ] = ret.doc_id;
			}
		}
	}

	if( execute_ret_status != FAIL && object->num_of_docs > noop_cnt ) {
		XMYSQLND_NODE_STMT * stmt = object->collection->data->m.add(object->collection,
											object->crud_op);
		stmt->data->assigned_document_ids = doc_ids;
		stmt->data->num_of_assigned_doc_ids = cur_doc_id_idx;
		execute_ret_status =  execute_statement(stmt,return_value);
	} else {
		mnd_efree( doc_ids );
	}

	if (FAIL == execute_ret_status && !EG(exception)) {
		RAISE_EXCEPTION(err_msg_add_doc);
	}

	if(object->crud_op) {
		xmysqlnd_crud_collection_add__destroy(object->crud_op);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__add_methods[] */
static const zend_function_entry mysqlx_node_collection__add_methods[] = {
	PHP_ME(mysqlx_node_collection__add, __construct,	NULL,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__add,	execute,		arginfo_mysqlx_node_collection__add__execute,	ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

#if 0
/* {{{ mysqlx_node_collection__add_property__name */
static zval *
mysqlx_node_collection__add_property__name(const struct st_mysqlx_object * obj, zval * return_value)
{
	const struct st_mysqlx_node_collection__add * object = (const struct st_mysqlx_node_collection__add *) (obj->ptr);
	DBG_ENTER("mysqlx_node_collection__add_property__name");
	if (object->collection && object->collection->data->collection_name.s) {
		ZVAL_STRINGL(return_value, object->collection->data->collection_name.s, object->collection->data->collection_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return NULL; -> isset()===false, value is NULL
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is NULL
		*/
		return_value = NULL;
	}
	DBG_RETURN(return_value);
}
/* }}} */
#endif

static zend_object_handlers mysqlx_object_node_collection__add_handlers;
static HashTable mysqlx_node_collection__add_properties;

const struct st_mysqlx_property_entry mysqlx_node_collection__add_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_node_collection__add_property__name,	NULL},
#endif
	{{NULL,	0}, NULL, NULL}
};

/* {{{ mysqlx_node_collection__add_free_storage */
static void
mysqlx_node_collection__add_free_storage(zend_object * object)
{
	int i;
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_collection__add * inner_obj = (struct st_mysqlx_node_collection__add *) mysqlx_object->ptr;

	if (inner_obj) {
		if (inner_obj->collection) {
			xmysqlnd_node_collection_free(inner_obj->collection, NULL, NULL);
			inner_obj->collection = NULL;
		}
		for(i = 0; i < inner_obj->num_of_docs; ++i ) {
			zval_ptr_dtor(&inner_obj->docs[i]);
			ZVAL_UNDEF(&inner_obj->docs[i]);
		}
		mnd_efree(inner_obj->docs);
		mnd_efree(inner_obj);
	}
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__add_object_allocator */
static zend_object *
php_mysqlx_node_collection__add_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_collection__add_object_allocator");
	st_mysqlx_object* mysqlx_object = mysqlx::phputils::alloc_object<st_mysqlx_node_collection__add>(
		class_type,
		&mysqlx_object_node_collection__add_handlers,
		&mysqlx_node_collection__add_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__add_class */
void
mysqlx_register_node_collection__add_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_collection__add_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_collection__add_handlers.free_obj = mysqlx_node_collection__add_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "NodeCollectionAdd", mysqlx_node_collection__add_methods);
		tmp_ce.create_object = php_mysqlx_node_collection__add_object_allocator;
		mysqlx_node_collection__add_class_entry = zend_register_internal_class(&tmp_ce);
		zend_class_implements(mysqlx_node_collection__add_class_entry, 1, mysqlx_executable_interface_entry);
	}

	zend_hash_init(&mysqlx_node_collection__add_properties, 0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_collection__add_properties, mysqlx_node_collection__add_property_entries);
#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(mysqlx_node_collection__add_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__add_class */
void
mysqlx_unregister_node_collection__add_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_collection__add_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__add */
void
mysqlx_new_node_collection__add(zval * return_value,
						XMYSQLND_NODE_COLLECTION * collection,
						const zend_bool clone,
						zval * docs,
						int num_of_docs)
{
	DBG_ENTER("mysqlx_new_node_collection__add");
	zend_bool op_success = TRUE;
	int i;

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_collection__add_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_collection__add * const object = (struct st_mysqlx_node_collection__add *) mysqlx_object->ptr;
		if( !object ) {
			op_success = FALSE;
		} else {
			object->collection = clone? collection->data->m.get_reference(collection) : collection;
			object->crud_op = xmysqlnd_crud_collection_add__create(mnd_str2c(object->collection->data->schema->data->schema_name),
													mnd_str2c(object->collection->data->collection_name));
			if( !object->crud_op ) {
				op_success = FALSE;
			} else {
				object->docs = static_cast<zval*>(mnd_ecalloc( num_of_docs , sizeof(zval) ));
				for(i = 0; i < num_of_docs; ++i) {
					ZVAL_DUP(&object->docs[i],
									&docs[i]);
				}
				object->num_of_docs = num_of_docs;
			}
		}

		if( op_success == FALSE ) {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			if( object && object->collection ) {
				object->collection->data->m.free_reference(object->collection, NULL, NULL);
			}
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
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
