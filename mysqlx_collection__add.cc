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
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__add.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/strings.h"
#include "util/string_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace {

zend_class_entry* collection_add_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_collection__add__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------


/* {{{ execute_statement */
enum_func_status
execute_statement(XMYSQLND_NODE_STMT* stmt,zval* return_value)
{
	enum_func_status ret{FAIL};
	if (stmt) {
		zval stmt_zv;
		ZVAL_UNDEF(&stmt_zv);
		mysqlx_new_node_stmt(&stmt_zv, stmt);
		if (Z_TYPE(stmt_zv) == IS_NULL) {
			xmysqlnd_node_stmt_free(stmt, nullptr, nullptr);
		}
		if (Z_TYPE(stmt_zv) == IS_OBJECT) {
			zval zv;
			ZVAL_UNDEF(&zv);
			zend_long flags{0};
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
#define ID_TEMPLATE_PREFIX	"\"" ID_COLUMN_NAME "\":\""
#define ID_TEMPLATE_SUFFIX	"\"}"

struct st_parse_for_id_status
{
	zend_bool		found:1;
	zend_bool		empty:1;
	zend_bool		is_string:1;
};

struct my_php_json_parser {
	php_json_parser parser;
	st_parse_for_id_status* status;
	HashTable array_of_allocated_obj;
};


/* {{{ xmysqlnd_json_parser_object_update */
int
xmysqlnd_json_parser_object_update(php_json_parser *parser,
					zval *object,
					zend_string *key,
					zval *zvalue)
{
	st_parse_for_id_status* status = ((struct my_php_json_parser *)parser)->status;
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
int
xmysqlnd_json_parser_object_create(php_json_parser *parser,
								zval *object)
{
	struct my_php_json_parser* php_json_parser = (struct my_php_json_parser*)parser;
	int ret{0};
	if (parser->scanner.options & PHP_JSON_OBJECT_AS_ARRAY) {
		ZVAL_UNDEF(object);
		array_init(object);
		ret = Z_TYPE_P(object) == IS_ARRAY ? SUCCESS : FAILURE;
	} else {
		ret = object_init(object);
	}
	zend_hash_next_index_insert(&php_json_parser->array_of_allocated_obj,
								object);
	return ret;
}
/* }}} */


/* {{{ xmysqlnd_json_parser_object_end */
int
xmysqlnd_json_parser_object_end(php_json_parser *parser, zval *object)
{
	DBG_ENTER("xmysqlnd_json_parser_object_end");
	zval_dtor(object);
	return SUCCESS;
}
/* }}} */

enum class Add_op_status
{
	success,
	fail,
	noop
};

struct doc_add_op_return_status
{
	Add_op_status return_status;
	MYSQLND_CSTRING doc_id;
};

/* {{{ node_collection_add_string */
Add_op_status
node_collection_add_string(
	st_xmysqlnd_crud_collection_op__add* add_op,
	zval* doc,
	zval* return_value)
{
	if( PASS == xmysqlnd_crud_collection_add__add_doc(add_op,doc) ) {
		return Add_op_status::success;
	}
	return Add_op_status::fail;
}
/* }}} */


/* {{{ node_collection_add_object*/
Add_op_status
node_collection_add_object_impl(
	st_xmysqlnd_crud_collection_op__add* add_op,
	zval* doc,
	zval* return_value)
{
	zval new_doc;
	Add_op_status ret = Add_op_status::fail;
	util::json::to_zv_string(doc, &new_doc);
	if( PASS == xmysqlnd_crud_collection_add__add_doc(add_op, &new_doc) ) {
		ret = Add_op_status::success;
	}
	zval_dtor(&new_doc);
	return ret;
}
/* }}} */


/* {{{ node_collection_add_object*/
Add_op_status
node_collection_add_object(
	st_xmysqlnd_crud_collection_op__add* add_op,
	zval* doc,
	zval* return_value)
{
	return node_collection_add_object_impl(
				add_op, doc, return_value);
}
/* }}} */


/* {{{ node_collection_add_array*/
Add_op_status
node_collection_add_array(
	st_xmysqlnd_crud_collection_op__add* add_op,
	zval* doc,
	zval* return_value)
{
	Add_op_status ret = Add_op_status::fail;
	if( zend_hash_num_elements(Z_ARRVAL_P(doc)) == 0 ) {
		ret = Add_op_status::noop;
	} else {
		ret = node_collection_add_object_impl(
			add_op, doc, return_value);
	}
	return ret;
}
/* }}} */

} // anonymous namespace

//------------------------------------------------------------------------------


/* {{{ Collection_add::init() */
bool Collection_add::init(
	zval* obj_zv,
	XMYSQLND_NODE_COLLECTION* coll,
	zval* documents,
	int num_of_documents)
{
	if (!obj_zv || !documents || !num_of_documents) return false;

	for (int i{0}; i < num_of_documents; ++i) {
		if (Z_TYPE(documents[i]) != IS_STRING &&
			Z_TYPE(documents[i]) != IS_OBJECT &&
			Z_TYPE(documents[i]) != IS_ARRAY) {
			php_error_docref(nullptr, E_WARNING, "Only strings, objects and arrays can be added. Type is %u", Z_TYPE(documents[i]));
			return false;
		}
	}

	object_zv = obj_zv;
	collection = coll->data->m.get_reference(coll);
	add_op = xmysqlnd_crud_collection_add__create(
		mnd_str2c(collection->data->schema->data->schema_name),
		mnd_str2c(collection->data->collection_name));

	if (!add_op) return false;

	docs = static_cast<zval*>(mnd_ecalloc( num_of_documents, sizeof(zval) ));
	for (int i{0}; i < num_of_documents; ++i) {
		ZVAL_DUP(&docs[i], &documents[i]);
	}
	num_of_docs = num_of_documents;

	return true;
}
/* }}} */


/* {{{ Collection_add::init() */
bool Collection_add::init(
	zval* obj_zv,
	XMYSQLND_NODE_COLLECTION* coll,
	const util::string_view& doc_id,
	zval* doc)
{
	const int num_of_documents = 1;
	if (!init(obj_zv, coll, doc, num_of_documents)) return false;
	return xmysqlnd_crud_collection_add__set_upsert(add_op) == PASS;
}
/* }}} */


/* {{{ Collection_add::~Collection_add() */
Collection_add::~Collection_add()
{
	for (int i{0}; i < num_of_docs; ++i) {
		zval_ptr_dtor(&docs[i]);
		ZVAL_UNDEF(&docs[i]);
	}

	mnd_efree(docs);

	if (add_op) {
		xmysqlnd_crud_collection_add__destroy(add_op);
	}

	if (collection) {
		xmysqlnd_node_collection_free(collection, nullptr, nullptr);
	}
}
/* }}} */


/* {{{ Collection_add::execute() */
void Collection_add::execute(zval* return_value)
{
	enum_func_status execute_ret_status{PASS};
	int noop_cnt{0};

	DBG_ENTER("Collection_add::execute");

	RETVAL_FALSE;

	Add_op_status ret = Add_op_status::success;
	for (int i{0}; i < num_of_docs && ret != Add_op_status::fail ; ++i) {
		ret = Add_op_status::fail;
		switch(Z_TYPE(docs[i])) {
		case IS_STRING:
			ret = node_collection_add_string(
				add_op, &docs[i], return_value);
			break;
		case IS_ARRAY:
			ret = node_collection_add_array(
				add_op, &docs[i], return_value);
			break;
		case IS_OBJECT:
			ret = node_collection_add_object(
				add_op, &docs[i], return_value);
			break;
		}
		if( ret == Add_op_status::noop ) {
			++noop_cnt;
		}
	}

	if ( execute_ret_status != FAIL && num_of_docs > noop_cnt ) {
		XMYSQLND_NODE_STMT* stmt = collection->data->m.add(collection,
														   add_op);
		if( nullptr != stmt ) {
			execute_ret_status =  execute_statement(stmt,return_value);
		} else {
			execute_ret_status = FAIL;
		}
	}
	if (FAIL == execute_ret_status && !EG(exception)) {
		RAISE_EXCEPTION(err_msg_add_doc);
	}
	DBG_VOID_RETURN;
}
/* }}} */


//------------------------------------------------------------------------------


/* {{{ mysqlx_node_collection__add::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__add, __construct)
{
}
/* }}} */


/* {{{ proto mixed mysqlx_node_collection__add::execute() */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_collection__add, execute)
{
	zval* object_zv{nullptr};

	DBG_ENTER("mysqlx_node_collection__add::execute");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
												&object_zv,
												collection_add_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_add& coll_add = util::fetch_data_object<Collection_add>(object_zv);
	coll_add.execute(return_value);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_collection__add_methods[] */
static const zend_function_entry mysqlx_node_collection__add_methods[] = {
	PHP_ME(mysqlx_node_collection__add, __construct,	nullptr,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_node_collection__add,	execute,		arginfo_mysqlx_node_collection__add__execute,	ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};
/* }}} */

#if 0
/* {{{ mysqlx_node_collection__add_property__name */
static zval *
mysqlx_node_collection__add_property__name(const st_mysqlx_object* obj, zval* return_value)
{
	const Collection_add* object = (const Collection_add *) (obj->ptr);
	DBG_ENTER("mysqlx_node_collection__add_property__name");
	if (object->collection && object->collection->data->collection_name.s) {
		ZVAL_STRINGL(return_value, object->collection->data->collection_name.s, object->collection->data->collection_name.l);
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}
/* }}} */
#endif

static zend_object_handlers collection_add_handlers;
static HashTable collection_add_properties;

const st_mysqlx_property_entry collection_add_property_entries[] =
{
#if 0
	{{"name",	sizeof("name") - 1}, mysqlx_node_collection__add_property__name,	nullptr},
#endif
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_collection__add_free_storage */
static void
mysqlx_node_collection__add_free_storage(zend_object* object)
{
	util::free_object<Collection_add>(object);
}
/* }}} */


/* {{{ php_mysqlx_node_collection__add_object_allocator */
static zend_object *
php_mysqlx_node_collection__add_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__add_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_add>(
		class_type,
		&collection_add_handlers,
		&collection_add_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_collection__add_class */
void
mysqlx_register_node_collection__add_class(INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_add_class_entry,
		"CollectionAdd",
		mysqlx_std_object_handlers,
		collection_add_handlers,
		php_mysqlx_node_collection__add_object_allocator,
		mysqlx_node_collection__add_free_storage,
		mysqlx_node_collection__add_methods,
		collection_add_properties,
		collection_add_property_entries,
		mysqlx_executable_interface_entry);

#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(collection_add_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}
/* }}} */


/* {{{ mysqlx_unregister_node_collection__add_class */
void
mysqlx_unregister_node_collection__add_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_add_properties);
}
/* }}} */


/* {{{ mysqlx_new_node_collection__add */
void
mysqlx_new_node_collection__add(
	zval* return_value,
	XMYSQLND_NODE_COLLECTION* collection,
	zval* docs,
	int num_of_docs)
{
	DBG_ENTER("mysqlx_new_node_collection__add");

	if (SUCCESS == object_init_ex(return_value, collection_add_class_entry) && IS_OBJECT == Z_TYPE_P(return_value)) {
		const st_mysqlx_object* const mysqlx_object = Z_MYSQLX_P(return_value);
		Collection_add* const coll_add = static_cast<Collection_add*>(mysqlx_object->ptr);
		if (!coll_add || !coll_add->init(return_value, collection, docs, num_of_docs)) {
			php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
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
