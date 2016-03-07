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
  | Authors: Oracle Corp                                                 |
  +----------------------------------------------------------------------+
*/

#ifndef XMYSQLND_CRUND_PARSERS_H
#define XMYSQLND_CRUND_PARSERS_H

#include "expression_parser.h"
#include "orderby_parser.h"
#include "projection_parser.h"

#include <string>

inline Mysqlx::Expr::Expr* xmysqlnd_crud_parse_collection_filter(const std::string &source, std::vector<std::string>* placeholders = NULL)
{
  xmysqlnd::Expression_parser parser(source, true, false, placeholders);
  return parser.expr();
}

inline void xmysqlnd_crud_parse_document_path(const std::string& source, Mysqlx::Expr::ColumnIdentifier& colid)
{
  xmysqlnd::Expression_parser parser(source, true);
  return parser.document_path(colid);
}

inline Mysqlx::Expr::Expr* xmysqlnd_crud_parse_column_identifier(const std::string& source)
{
  xmysqlnd::Expression_parser parser(source, true);
  return parser.document_field();
}

inline Mysqlx::Expr::Expr* xmysqlnd_crud_parse_table_filter(const std::string &source, std::vector<std::string>* placeholders = NULL)
{
  xmysqlnd::Expression_parser parser(source, false, false, placeholders);
  return parser.expr();
}

template<typename Container>
void xmysqlnd_crud_parse_collection_sort_column(Container &container, const std::string &source)
{
  xmysqlnd::Orderby_parser parser(source, true);
  return parser.parse(container);
}

template<typename Container>
void xmysqlnd_crud_parse_table_sort_column(Container &container, const std::string &source)
{
  xmysqlnd::Orderby_parser parser(source, false);
  return parser.parse(container);
}

template<typename Container>
void xmysqlnd_crud_parse_collection_column_list(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, true, false);
  parser.parse(container);
}

template<typename Container>
void xmysqlnd_crud_parse_collection_column_list_with_alias(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, true, true);
  parser.parse(container);
}

template<typename Container>
void xmysqlnd_crud_parse_table_column_list(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, false, false);
  parser.parse(container);
}

template<typename Container>
void xmysqlnd_crud_parse_table_column_list_with_alias(Container &container, const std::string &source)
{
  xmysqlnd::Projection_expression_parser parser(source, false, true);
  parser.parse(container);
}

#endif /* XMYSQLND_CRUND_PARSERS_H */
