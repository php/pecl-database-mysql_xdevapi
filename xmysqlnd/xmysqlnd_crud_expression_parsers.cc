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

#include "xmysqlnd_crud_expression_parsers.h"

#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"

struct xmysqlnd_crud_collection__remove
{
	std::string schema;
	std::string name;
	bool is_collection:1;
	bool criteria_set:1;
	Mysqlx::Expr::Expr * criteria;
	size_t limit;
	size_t offset;
	Mysqlx::Expr::Expr * order;
	std::vector<std::string> placeholders;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	
	xmysqlnd_crud_collection__remove(const std::string & schema_, const std::string &name_) :
		schema(schema_),
		name(name_),
		is_collection(true),
		criteria_set(false),
		criteria(NULL),
		limit(0),
		offset(0),
		order(NULL)
	{}

	~xmysqlnd_crud_collection__remove()
	{
		delete criteria;
	}
};

/* {{{ xmysqlnd_crud_collection_remove__create */
extern "C" struct xmysqlnd_crud_collection__remove *
xmysqlnd_crud_collection_remove__create(const MYSQLND_CSTRING schema, const MYSQLND_CSTRING collection)
{
	return new struct xmysqlnd_crud_collection__remove(std::string(schema.s, schema.l), std::string(collection.s, collection.l));
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__destroy */
extern "C" void
xmysqlnd_crud_collection_remove__destroy(struct xmysqlnd_crud_collection__remove * obj)
{
	delete obj;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_criteria */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_criteria(struct xmysqlnd_crud_collection__remove * obj, const MYSQLND_CSTRING criteria)
{
	try {
		std::string source(criteria.s, criteria.l);
		xmysqlnd::Expression_parser parser(source, true, false, &obj->placeholders);
		obj->criteria = parser.expr();

		if (obj->bound_values.size()) {
			obj->bound_values.clear();
		}
		obj->bound_values.resize(obj->placeholders.size(), NULL); /* fill with NULLs */

		obj->criteria_set = true;
	} catch (xmysqlnd::Parser_error &e) {
		return FAIL;
	}
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_limit */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_limit(struct xmysqlnd_crud_collection__remove * obj, const size_t limit)
{
	obj->limit = limit;
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__set_offset */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__set_offset(struct xmysqlnd_crud_collection__remove * obj, const size_t offset)
{
	obj->offset = offset;
	return PASS;
}
/* }}} */


/* {{{ xmysqlnd_crud_collection_remove__bind_value */
extern "C" enum_func_status
xmysqlnd_crud_collection_remove__bind_value(struct xmysqlnd_crud_collection__remove * obj, const MYSQLND_CSTRING name, zval * value)
{
	if (!obj->criteria_set) {
		return FAIL;
	}

	const std::string var_name(name.s, name.l);
	const std::vector<std::string>::iterator begin = obj->placeholders.begin();
	const std::vector<std::string>::iterator end = obj->placeholders.end();
	const std::vector<std::string>::const_iterator index = std::find(begin, end, var_name);
	if (index == end) {
		return FAIL;
	}

	Mysqlx::Datatypes::Any any;
	if (FAIL == zval2any(value, any)) {
		return FAIL;

	}
	obj->bound_values[index - begin] = any.release_scalar();

	return PASS;
}
/* }}} */


#ifdef A0
/* {{{ xmysqlnd_crud_parse_collection_filter */
PHPAPI XMYSQLND_CRUD_COLLECTION_FILTER
xmysqlnd_crud_parse_collection_filter(const std::string &source, std::vector<std::string>* placeholders)
{
	XMYSQLND_CRUD_COLLECTION_FILTER filter = { NULL, FALSE };
	
	try {
		xmysqlnd::Expression_parser parser(source, true, false, placeholders);
		filter.expr = parser.expr();
		filter.valid = TRUE;
	} catch (xmysqlnd::Parser_error &e) {
	
	}
	return filter;
}
/* }}} */


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


/* {{{ xmysqlnd_crud_parse_collection_sort_column */
template<typename Container>
void xmysqlnd_crud_parse_collection_sort_column(Container &container, const std::string &source)
{
  xmysqlnd::Orderby_parser parser(source, true);
  return parser.parse(container);
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_table_sort_column */
template<typename Container>
void xmysqlnd_crud_parse_table_sort_column(Container &container, const std::string &source)
{
  xmysqlnd::Orderby_parser parser(source, false);
  return parser.parse(container);
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
