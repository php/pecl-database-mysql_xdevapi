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
#include "xmysqlnd_zval2any.h"

#include <vector>
#include <string>

#include "xmysqlnd_crud_collection_commands.h"

#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"

struct st_xmysqlnd_crud_collection_op__remove
{
	Mysqlx::Crud::Delete message;

	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;

	st_xmysqlnd_crud_collection_op__remove(const MYSQLND_CSTRING & schema,
										   const MYSQLND_CSTRING & object_name,
										   const bool document_mode = true)
	{
		message.mutable_collection()->set_schema(schema.s, schema.l);
		message.mutable_collection()->set_name(object_name.s, object_name.l);
		message.set_data_model(document_mode? Mysqlx::Crud::DOCUMENT : Mysqlx::Crud::TABLE);
	}

	~st_xmysqlnd_crud_collection_op__remove()
	{}
};

/* {{{ xmysqlnd_crud_collection_remove__create */
extern "C" XMYSQLND_CRUD_COLLECTION_OP__REMOVE *
xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING object_name)
{
	XMYSQLND_CRUD_COLLECTION_OP__REMOVE * ret = NULL;
	DBG_ENTER("xmysqlnd_crud_collection_remove__create");
	DBG_INF_FMT("schema=%*s object_name=%*s", schema.l, schema.s, object_name.l, object_name.s);
	ret = new struct st_xmysqlnd_crud_collection_op__remove(schema, object_name);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__destroy */
extern "C" void
xmysqlnd_crud_collection_remove__destroy(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__destroy");
	delete obj;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_criteria */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_criteria(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING criteria)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_criteria");
	try {
		std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT, false, &obj->placeholders);
		Mysqlx::Expr::Expr * criteria = parser.expr();
		obj->message.set_allocated_criteria(criteria);

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */
	} catch (xmysqlnd::Parser_error &e) {
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_limit */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_limit(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t limit)
{
	obj->message.mutable_limit()->set_row_count(limit);
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_offset */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_offset(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const size_t offset)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__set_offset");
	obj->message.mutable_limit()->set_offset(offset);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__bind_value */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__bind_value(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING name, zval * value)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__bind_value");
	DBG_INF_FMT("name=%*s", name.l, name.s);
	if (!obj->message.has_criteria()) {
		DBG_ERR("No criteria set");
		DBG_RETURN(FAIL);
	}

	const std::string var_name(name.s, name.l);
	const std::vector<std::string>::iterator begin = obj->placeholders.begin();
	const std::vector<std::string>::iterator end = obj->placeholders.end();
	const std::vector<std::string>::const_iterator index = std::find(begin, end, var_name);
	if (index == end) {
		DBG_ERR("No such variable in the expression");
		DBG_RETURN(FAIL);
	}

	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(value, any)) {
		DBG_ERR("Error converting the zval to scalar");
		DBG_RETURN(FAIL);
	}
	any2log(any);

	obj->bound_values[index - begin] = any.release_scalar();

	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__add_sort */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__add_sort(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj, const MYSQLND_CSTRING sort)
{
	DBG_ENTER("xmysqlnd_crud_collection_remove__add_sort");
	DBG_INF_FMT("sort=%*s", sort.l, sort.s);
	try {
		std::string source(sort.s, sort.l);
		xmysqlnd::Orderby_parser parser(source, obj->message.data_model() == Mysqlx::Crud::DOCUMENT);
		parser.parse(*(obj->message.mutable_order()));
	} catch (xmysqlnd::Parser_error &e) {
		DBG_INF("Parser error");
		DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__is_initialized */
extern "C" zend_bool
xmysqlnd_crud_collection_remove__is_initialized(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	const zend_bool ret = obj && obj->message.IsInitialized()? TRUE : FALSE;
	DBG_ENTER("xmysqlnd_crud_collection_remove__is_initialized");
	DBG_INF_FMT("is_initialized=%u", ret);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__get_protobuf_message */
extern "C" struct st_xmysqlnd_pb_message_shell 
xmysqlnd_crud_collection_remove__get_protobuf_message(XMYSQLND_CRUD_COLLECTION_OP__REMOVE * obj)
{
	struct st_xmysqlnd_pb_message_shell ret = { (void *) &obj->message };
	return ret;
}
/* }}} */

#ifdef A0

/* {{{ xmysqlnd_crud_parse_document_path */
PHPAPI XMYSQLND_CRUD_DOCUMENT_PATH
xmysqlnd_crud_parse_document_path(const std::string& source, Mysqlx::Expr::ColumnIdentifier& col_identifier)
{
	XMYSQLND_CRUD_DOCUMENT_PATH doc_path = { NULL, FALSE };
	
	try {
		xmysqlnd::Expression_parser parser(source, true);
		parser.document_path(col_identifier);
		doc_path.valid = TRUE;
	} catch (xmysqlnd::Parser_error &e) {
	
	}
	return doc_path;
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_column_identifier */
PHPAPI XMYSQLND_CRUD_COLUMN_IDENTIFIER
xmysqlnd_crud_parse_column_identifier(const std::string& source)
{
	XMYSQLND_CRUD_COLUMN_IDENTIFIER col_identifier = { NULL, FALSE };
	try {
		xmysqlnd::Expression_parser parser(source, true);
		col_identifier.expr = parser.document_field();
		col_identifier.valid = TRUE;
	} catch (xmysqlnd::Parser_error &e) {

	}
	return col_identifier;
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_table_filter */
PHPAPI XMYSQLND_CRUD_TABLE_FILTER
xmysqlnd_crud_parse_table_filter(const std::string &source, std::vector<std::string>* placeholders)
{
	XMYSQLND_CRUD_TABLE_FILTER table_filter = { NULL, FALSE };
	try {
		xmysqlnd::Expression_parser parser(source, false, false, placeholders);
		table_filter.expr = parser.expr();
		table_filter.valid = TRUE;
	} catch (xmysqlnd::Parser_error &e) {

	}
	return table_filter;
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_collection_column_list */
template<typename Container>
void xmysqlnd_crud_parse_collection_column_list(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, true, false);
  parser.parse(container);
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_collection_column_list_with_alias */
template<typename Container>
void xmysqlnd_crud_parse_collection_column_list_with_alias(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, true, true);
  parser.parse(container);
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_table_column_list */
template<typename Container>
void xmysqlnd_crud_parse_table_column_list(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, false, false);
  parser.parse(container);
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_table_column_list_with_alias */
template<typename Container>
void xmysqlnd_crud_parse_table_column_list_with_alias(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, false, true);
  parser.parse(container);
}
/* }}} */

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
