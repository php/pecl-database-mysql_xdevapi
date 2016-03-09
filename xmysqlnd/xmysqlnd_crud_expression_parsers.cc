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

#include "crud_parsers/expression_parser.h"
#include "crud_parsers/orderby_parser.h"
#include "crud_parsers/projection_parser.h"

#include <string>

/* {{{ xmysqlnd_crud_parse_collection_filter */
Mysqlx::Expr::Expr *
xmysqlnd_crud_parse_collection_filter(const std::string &source, std::vector<std::string>* placeholders = NULL)
{
  xmysqlnd::Expression_parser parser(source, true, false, placeholders);
  return parser.expr();
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_document_path */
void
xmysqlnd_crud_parse_document_path(const std::string& source, Mysqlx::Expr::ColumnIdentifier& colid)
{
  xmysqlnd::Expression_parser parser(source, true);
  return parser.document_path(colid);
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_document_path */
Mysqlx::Expr::Expr *
xmysqlnd_crud_parse_column_identifier(const std::string& source)
{
  xmysqlnd::Expression_parser parser(source, true);
  return parser.document_field();
}
/* }}} */


/* {{{ xmysqlnd_crud_parse_document_path */
Mysqlx::Expr::Expr*
xmysqlnd_crud_parse_table_filter(const std::string &source, std::vector<std::string>* placeholders = NULL)
{
  xmysqlnd::Expression_parser parser(source, false, false, placeholders);
  return parser.expr();
}

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