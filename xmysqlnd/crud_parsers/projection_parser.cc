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
#include "xmysqlnd/crud_parsers/projection_parser.h"
#include "proto_gen/mysqlx_crud.pb.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace xmysqlnd;

Projection_expression_parser::Projection_expression_parser(const std::string& expr_str, const bool document_mode, const bool allow_alias)
  : Expression_parser(expr_str, document_mode, allow_alias)
{
}

/*
 * id ::= IDENT | MUL
 */
const std::string& Projection_expression_parser::id()
{
  if (_tokenizer.cur_token_type_is(Token::IDENT))
    return _tokenizer.consume_token(Token::IDENT);
  else
    return _tokenizer.consume_token(Token::MUL);
}

/*
 * column_identifier ::= ( expr [ [AS] IDENT ] ) | ( DOLLAR [ IDENT ] document_path )
 */
void Projection_expression_parser::source_expression(Mysqlx::Crud::Projection &col)
{
  if ( _document_mode && _tokenizer.cur_token_type_is(Token::DOLLAR))
  {
    _tokenizer.consume_token(Token::DOLLAR);
    Mysqlx::Expr::ColumnIdentifier* colid = col.mutable_source()->mutable_identifier();
    col.mutable_source()->set_type(Mysqlx::Expr::Expr::IDENT);
    if (_tokenizer.cur_token_type_is(Token::IDENT))
    {
      const std::string& ident = _tokenizer.consume_token(Token::IDENT);
      colid->mutable_document_path()->Add()->set_value(ident.c_str(), ident.size());
    }
    document_path(*colid);
  }
  else
    col.set_allocated_source(my_expr());

  // Sets the alias token
  if (_allow_alias)
  {
    if (_tokenizer.cur_token_type_is(Token::AS))
    {
      _tokenizer.consume_token(Token::AS);
      const std::string& alias = _tokenizer.consume_token(Token::IDENT);
      col.set_alias(alias.c_str());
    }
    else if (_tokenizer.cur_token_type_is(Token::IDENT))
    {
      const std::string& alias = _tokenizer.consume_token(Token::IDENT);
      col.set_alias(alias.c_str());
    }
    else if (_document_mode)
      col.set_alias(_tokenizer.get_input());
  }
}