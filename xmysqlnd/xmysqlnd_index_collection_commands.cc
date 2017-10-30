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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_node_session.h"
#include "xmysqlnd_node_schema.h"
#include "xmysqlnd_node_collection.h"
#include "xmysqlnd_node_stmt.h"
#include "xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd_utils.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_wireprotocol.h"

#include "xmysqlnd_index_collection_commands.h"
#include "mysqlx_object.h"
#include "mysqlx_class_properties.h"
#include "proto_gen/mysqlx_sql.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "phputils/allocator.h"
#include "phputils/object.h"
#include "phputils/strings.h"
#include "phputils/types.h"

#include "xmysqlnd/crud_parsers/mysqlx_crud_parser.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"

namespace mysqlx {

namespace drv {

namespace
{

template<typename String>
bool is_empty(const String& mystr)
{
	return (mystr.s == nullptr) || (mystr.l == 0);
}

} // anonymous namespace


/****************************** COLLECTION.CREATE_INDEX() *******************************************************/

struct st_index_field
{
	phputils::string doc_path;
	phputils::string column_type;
	zend_bool is_required = FALSE;

	st_index_field()
	{
	}

	st_index_field(
		const MYSQLND_CSTRING & doc_path_,
		const MYSQLND_CSTRING & column_type_,
		const zend_bool is_required_)
		: doc_path(doc_path_.s, doc_path_.l)
		, column_type(column_type_.s, column_type_.l)
		, is_required(is_required_)
	{
	}
};

using st_index_fields = phputils::vector<st_index_field>;

struct st_xmysqlnd_collection_op__create_index : phputils::custom_allocable
{
	phputils::string schema_name;
	phputils::string collection_name;
	phputils::string index_name;
	zend_bool is_unique = FALSE;

	st_index_fields index_fields;

	st_xmysqlnd_collection_op__create_index(
		const MYSQLND_CSTRING & schema_,
		const MYSQLND_CSTRING & object_name_)
		: schema_name(schema_.s, schema_.l)
		, collection_name(object_name_.s, object_name_.l)
	{
	}

	zend_bool is_initialized() const
	{
		zend_bool ret = !schema_name.empty() && !collection_name.empty() && !index_name.empty() && !index_fields.empty();
		return ret;
	}

};


/* {{{ xmysqlnd_collection_create_index__create */
XMYSQLND_COLLECTION_OP__CREATE_INDEX *
xmysqlnd_collection_create_index__create(const MYSQLND_CSTRING schema_name, const MYSQLND_CSTRING collection_name)
{
	XMYSQLND_COLLECTION_OP__CREATE_INDEX* ret = nullptr;
	DBG_ENTER("xmysqlnd_collection_create_index__create");
	DBG_INF_FMT("schema_name=%*s collection_name=%*s", schema_name.l, schema_name.s, collection_name.l, collection_name.s);
	ret = new st_xmysqlnd_collection_op__create_index(schema_name, collection_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__destroy */
void
xmysqlnd_collection_create_index__destroy(XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj)
{
	DBG_ENTER("xmysqlnd_collection_create_index__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__set_index_name */
enum_func_status
xmysqlnd_collection_create_index__set_index_name(XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj, const MYSQLND_CSTRING index_name)
{
	DBG_ENTER("xmysqlnd_collection_create_index__set_index_name");
	obj->index_name.assign(index_name.s, index_name.l);
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__set_index_name */
enum_func_status
xmysqlnd_collection_create_index__set_unique(XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj, const zend_bool is_unique)
{
	DBG_ENTER("xmysqlnd_collection_create_index__set_unique");
	obj->is_unique = is_unique;
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__add_field */
enum_func_status
xmysqlnd_collection_create_index__add_field(
	XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj,
	MYSQLND_CSTRING doc_path,
	MYSQLND_CSTRING column_type,
	zend_bool is_required)
{
	DBG_ENTER("xmysqlnd_collection_create_index__add_field");
	if (!is_empty_str(doc_path) && !is_empty_str(column_type)) {
		obj->index_fields.emplace_back(
			st_index_field(
				doc_path,
				column_type,
				is_required));
		DBG_RETURN(PASS);
	} else {
		DBG_RETURN(FAIL);
	}
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__is_initialized */
zend_bool
xmysqlnd_collection_create_index__is_initialized(XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj)
{
	const zend_bool ret = obj && obj->is_initialized() ? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_collection_create_index__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ st_collection_create_collection_index_var_binder_ctx */
struct st_collection_create_collection_index_var_binder_ctx
{
	st_xmysqlnd_collection_op__create_index* index_op;
	unsigned int counter;
};
/* }}} */


/* {{{ collection_index_bind_bool_param */
static enum_hnd_func_status
collection_index_bind_bool_param(
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute,
	const unsigned int param_no,
	const zend_bool param)
{
	DBG_ENTER("collection_index_bind_bool_param");
	enum_hnd_func_status ret = HND_FAIL;

	zval zv;
	ZVAL_UNDEF(&zv);
	ZVAL_BOOL(&zv, param);
	DBG_INF_FMT("[%d]=[%d]", param_no, param);
	enum_func_status result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, param_no, &zv);

	zval_ptr_dtor(&zv);
	if (FAIL == result) {
		ret = HND_FAIL;
	} else {
		ret = HND_AGAIN;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ collection_index_bind_string_param */
static enum_hnd_func_status
collection_index_bind_string_param(
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute,
	const unsigned int param_no,
	const phputils::string& param)
{
	DBG_ENTER("collection_index_bind_string_param");
	enum_hnd_func_status ret = HND_FAIL;

	zval zv;
	ZVAL_UNDEF(&zv);
	ZVAL_STRINGL(&zv, param.c_str(), param.length());
	DBG_INF_FMT("[%d]=[%*s]", param_no, param.length(), param.c_str());
	enum_func_status result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, param_no, &zv);

	zval_ptr_dtor(&zv);
	if (FAIL == result) {
		ret = HND_FAIL;
	} else {
		ret = HND_AGAIN;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ collection_index_bind_field_param */
static enum_hnd_func_status
collection_index_bind_field_param(
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute,
	const unsigned int param_no,
	const phputils::string& rawPath)
{
	DBG_ENTER("collection_index_bind_field_param");
	enum_hnd_func_status ret = HND_FAIL;

	phputils::string fieldPath;

	try {
		const phputils::string source(rawPath.length() ? rawPath.c_str() : "$", rawPath.length() ? rawPath.length() : sizeof("$") - 1);
		old_parser_api::Expression_parser parser(source.c_str(), true);
		std::unique_ptr<Mysqlx::Expr::Expr> docField(parser.document_field());
		if (docField->has_identifier()) {
			auto& id = docField->identifier();
			if (0 < id.document_path_size()) {
				fieldPath = rawPath;
			}
		}
	} catch (old_parser_api::Parser_error &e) {
		php_error_docref(NULL, E_WARNING, "Error while parsing, details: %s", e.what());
		DBG_ERR_FMT("%s", e.what());
		DBG_ERR("Parser error for document field");
		DBG_RETURN(HND_FAIL);
	}

	if (!fieldPath.empty()) {
		zval zv;
		ZVAL_UNDEF(&zv);
		ZVAL_STRINGL(&zv, fieldPath.c_str(), fieldPath.length());
		DBG_INF_FMT("[%d]=[%*s]", param_no, fieldPath.length(), fieldPath.c_str());

		const enum_func_status result = xmysqlnd_stmt_execute__bind_one_param(stmt_execute, param_no, &zv);

		zval_ptr_dtor(&zv);
		if (FAIL == result) {
			ret = HND_FAIL;
		} else {
			ret = HND_AGAIN;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ collection_index_bind_fields */
static const enum_hnd_func_status
collection_index_bind_fields(
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute,
	const unsigned int counter,
	const st_index_fields& index_fields)
{
	enum_hnd_func_status ret = HND_FAIL;
	DBG_ENTER("collection_index_bind_fields");

	const unsigned int GlobalParamsCount = 4;
	const unsigned int ParamsPerField = 3;
	assert(GlobalParamsCount <= counter);
	const unsigned int fieldNo = (counter - GlobalParamsCount) / ParamsPerField;
	const unsigned int fieldParamNo = (counter - GlobalParamsCount) % ParamsPerField;
	assert(fieldNo < index_fields.size());
	const st_index_field& field = index_fields[fieldNo];

	switch (fieldParamNo) {
		case 0:
			ret = collection_index_bind_field_param(stmt_execute, counter, field.doc_path);
			break;

		case 1:
			ret = collection_index_bind_string_param(stmt_execute, counter, field.column_type);
			break;

		case 2: {
			ret = collection_index_bind_bool_param(stmt_execute, counter, field.is_required);
			const bool lastFieldAndParam = (ret == HND_AGAIN) && (fieldNo + 1 == index_fields.size());
			if (lastFieldAndParam) {
				ret = HND_PASS;
			}
			break;
		}

		default:
			assert(!"fieldParamNo too big!");
			break;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ collection_create_index_var_binder */
static const enum_hnd_func_status
collection_create_index_var_binder(
	void* context,
	XMYSQLND_NODE_SESSION* session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	enum_hnd_func_status ret = HND_FAIL;
	st_collection_create_collection_index_var_binder_ctx* ctx = static_cast<st_collection_create_collection_index_var_binder_ctx*>(context);
	st_xmysqlnd_collection_op__create_index* index_op = ctx->index_op;

	const phputils::string* param = nullptr;
	DBG_ENTER("collection_create_index_var_binder");
	switch (ctx->counter) {
		case 0:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->schema_name);
			break;

		case 1:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->collection_name);
			break;

		case 2:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->index_name);
			break;

		case 3:
			ret = collection_index_bind_bool_param(stmt_execute, ctx->counter, index_op->is_unique);
			break;

		default:
			ret = collection_index_bind_fields(stmt_execute, ctx->counter, index_op->index_fields);
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_collection_create_index__execute */
enum_func_status
xmysqlnd_collection_create_index__execute(
	st_xmysqlnd_node_session* const session,
	XMYSQLND_COLLECTION_OP__CREATE_INDEX* obj,
	st_xmysqlnd_node_session_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_collection_create_index__execute");

	enum_func_status ret;
	static const MYSQLND_CSTRING query = {"create_collection_index", sizeof("create_collection_index") - 1 };

	st_collection_create_collection_index_var_binder_ctx var_binder_ctx = { obj, 0};
	const st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_create_index_var_binder, &var_binder_ctx };

	ret = session->m->query_cb(session,
							   namespace_xplugin,
							   query,
							   var_binder,
							   noop__on_result_start,
							   noop__on_row, //on_row,
							   noop__on_warning,
							   on_error,
							   noop__on_result_end,
							   noop__on_statement_ok);

	DBG_RETURN(ret);
}
/* }}} */

/****************************** COLLECTION.DROP_INDEX() *******************************************************/

namespace
{

/* {{{ collection_drop_index_data */
struct collection_drop_index_data : phputils::custom_allocable
{
	phputils::string schema_name;
	phputils::string collection_name;
	phputils::string index_name;

	collection_drop_index_data(
		const MYSQLND_CSTRING& schema,
		const MYSQLND_CSTRING& collection,
		const phputils::string_view& index)
		: schema_name(schema.s, schema.l)
		, collection_name(collection.s, collection.l)
		, index_name(index.to_string())
	{
	}

	bool is_initialized() const
	{
		return !schema_name.empty() && !collection_name.empty() && !index_name.empty();
	}
};
/* }}} */


/* {{{ st_collection_drop_collection_index_var_binder_ctx */
struct st_collection_drop_collection_index_var_binder_ctx
{
	collection_drop_index_data* index_op;
	unsigned int counter;
};
/* }}} */


/* {{{ collection_drop_index_var_binder */
static const enum_hnd_func_status
collection_drop_index_var_binder(
	void* context,
	XMYSQLND_NODE_SESSION* session,
	XMYSQLND_STMT_OP__EXECUTE* const stmt_execute)
{
	enum_hnd_func_status ret = HND_FAIL;
	st_collection_drop_collection_index_var_binder_ctx* ctx = static_cast<st_collection_drop_collection_index_var_binder_ctx*>(context);
	collection_drop_index_data* index_op = ctx->index_op;

	const phputils::string* param = nullptr;
	DBG_ENTER("collection_drop_index_var_binder");
	switch (ctx->counter) {
		case 0:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->schema_name);
			break;

		case 1:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->collection_name);
			break;

		case 2:
			ret = collection_index_bind_string_param(stmt_execute, ctx->counter, index_op->index_name);
			if (ret == HND_AGAIN) {
				ret = HND_PASS;
			}
			break;

		default:
			assert(!"incorrect counter - should not happen!");
	}
	++ctx->counter;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_collection_drop_index__execute */
enum_func_status
xmysqlnd_collection_drop_index__execute(
	st_xmysqlnd_node_session* const session,
	collection_drop_index_data* obj,
	st_xmysqlnd_node_session_on_error_bind on_error)
{
	DBG_ENTER("xmysqlnd_collection_drop_index__execute");

	static const MYSQLND_CSTRING query = {"drop_collection_index", sizeof("drop_collection_index") - 1 };

	st_collection_drop_collection_index_var_binder_ctx var_binder_ctx = { obj, 0};
	const st_xmysqlnd_node_session_query_bind_variable_bind var_binder = { collection_drop_index_var_binder, &var_binder_ctx };

	const enum_func_status ret
		= session->m->query_cb(
			session,
			namespace_xplugin,
			query,
			var_binder,
			noop__on_result_start,
			noop__on_row,
			noop__on_warning,
			on_error,
			noop__on_result_end,
			noop__on_statement_ok);

	DBG_RETURN(ret);

	return ret;
}
/* }}} */

} // anonymous namespace


/* {{{ collection_drop_index */
bool collection_drop_index(
	XMYSQLND_NODE_COLLECTION* collection,
	const phputils::string_view& index_name,
	st_xmysqlnd_node_session_on_error_bind on_error)
{
	collection_drop_index_data op_drop_index(
		mnd_str2c(collection->data->schema->data->schema_name),
		mnd_str2c(collection->data->collection_name),
		index_name);

	if (!op_drop_index.is_initialized()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::drop_index_fail);
	}

	st_xmysqlnd_node_session* session = collection->data->schema->data->session;
	return (PASS == drv::xmysqlnd_collection_drop_index__execute(session, &op_drop_index, on_error));
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
