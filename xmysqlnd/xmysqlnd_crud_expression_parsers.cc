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

#include <vector>
#include <string>

#include "xmysqlnd_crud_expression_parsers.h"

#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"

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


#ifdef A0
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
