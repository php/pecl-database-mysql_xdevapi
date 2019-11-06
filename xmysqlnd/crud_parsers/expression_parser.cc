/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "php_api.h"
#include "mysqlnd_api.h"

#include "expression_parser.h"
#include "legacy_tokenizer.h"
#include <stdexcept>
#include <memory>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cstdlib>

#ifndef PHP_WIN32
#include <strings.h>
#  define _stricmp strcasecmp
#endif

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace mysqlx {

namespace old_parser_api {

Expression_parser::Expression_parser(const std::string& expr_str, bool document_mode, bool allow_alias, std::vector<std::string>* place_holders) : _tokenizer(expr_str), _document_mode(document_mode), _allow_alias(allow_alias)
{
  // If provided uses external placeholder information, if not uses the internal
  if (place_holders)
    _place_holder_ref = place_holders;
  else
    _place_holder_ref = &_place_holders;

  _tokenizer.get_tokens();
}

/*
 * identifier ::= IDENT [ DOT IDENT ]
 */
Mysqlx::Expr::Identifier* Expression_parser::identifier()
{
  DBG_ENTER("Expression_parser::identifier");
  _tokenizer.assert_cur_token(Token::IDENT);
  std::unique_ptr<Mysqlx::Expr::Identifier> id(new Mysqlx::Expr::Identifier());
  if (_tokenizer.next_token_type(Token::DOT))
  {
    const std::string& schema_name = _tokenizer.consume_token(Token::IDENT);
    id->set_schema_name(schema_name.c_str(), schema_name.size());
    _tokenizer.consume_token(Token::DOT);
  }
  const std::string& name = _tokenizer.consume_token(Token::IDENT);
  id->set_name(name.c_str(), name.size());
  return id.release();
}


/*
 * docpath_member ::= DOT ( IDENT | LSTRING | MUL )
 */
void Expression_parser::docpath_member(Mysqlx::Expr::DocumentPathItem& item)
{
  DBG_ENTER("Expression_parser::docpath_member");
  _tokenizer.consume_token(Token::DOT);
  item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
  if (_tokenizer.cur_token_type_is(Token::IDENT))
  {
    const std::string& ident = _tokenizer.consume_token(Token::IDENT);
    item.set_value(ident.c_str(), ident.size());
  }
  else if (_tokenizer.cur_token_type_is(Token::LSTRING))
  {
    const std::string& lstring = _tokenizer.consume_token(Token::LSTRING);
    item.set_value(lstring.c_str(), lstring.size());
  }
  else if (_tokenizer.cur_token_type_is(Token::MUL))
  {
    const std::string& mul = _tokenizer.consume_token(Token::MUL);
    item.set_value(mul.c_str(), mul.size());
    item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER_ASTERISK);
  }
  else
  {
    const Token& tok = _tokenizer.peek_token();
    throw Parser_error((boost::format("Expected token type IDENT or LSTRING in JSON path at position %d (%s)") % tok.get_pos() % tok.get_text()).str());
  }
  DBG_VOID_RETURN;
}

/*
 * docpath_array_loc ::= LSQBRACKET ( MUL | LINTEGER ) RSQBRACKET
 */
void Expression_parser::docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item)
{
  DBG_ENTER("Expression_parser::docpath_array_loc");
  _tokenizer.consume_token(Token::LSQBRACKET);
  const Token& tok = _tokenizer.peek_token();
  if (_tokenizer.cur_token_type_is(Token::MUL))
  {
    _tokenizer.consume_token(Token::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX_ASTERISK);
  }
  else if (_tokenizer.cur_token_type_is(Token::LINTEGER))
  {
    const std::string& value = _tokenizer.consume_token(Token::LINTEGER);
    int v = boost::lexical_cast<int>(value.c_str(), value.size());
    if (v < 0)
      throw Parser_error((boost::format("Array index cannot be negative at position %d") % tok.get_pos()).str());
    _tokenizer.consume_token(Token::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX);
    item.set_index(v);
  }
  else
  {
    throw Parser_error((boost::format("Exception token type MUL or LINTEGER in JSON path array index at token position %d (%s)") % tok.get_pos() % tok.get_text()).str());
  }
  DBG_VOID_RETURN;
}

/*
 * document_path ::= ( docpath_member | docpath_array_loc | ( DOUBLESTAR ))+
 */
void Expression_parser::document_path(Mysqlx::Expr::ColumnIdentifier& colid)
{
  DBG_ENTER("Expression_parser::document_path");
  // Parse a JSON-style document path, like WL#7909, prefixing with $
  while (true)
  {
    if (_tokenizer.cur_token_type_is(Token::DOT))
    {
      docpath_member(*colid.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::LSQBRACKET))
    {
      docpath_array_loc(*colid.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::DOUBLESTAR))
    {
      _tokenizer.consume_token(Token::DOUBLESTAR);
      Mysqlx::Expr::DocumentPathItem* item = colid.mutable_document_path()->Add();
      item->set_type(Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK);
    }
    else
    {
      break;
    }
  }
  size_t size = colid.document_path_size();
  if (size > 0 && (colid.document_path(static_cast<int>(size) - 1).type() == Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK))
  {
    const Token& tok = _tokenizer.peek_token();
    throw Parser_error((boost::format("JSON path may not end in '**' at position %d (%s)") % tok.get_pos() % tok.get_text()).str());
  }
  DBG_VOID_RETURN;
}

/*
 * id ::= IDENT | MUL
 */
const std::string& Expression_parser::id()
{
  if (_tokenizer.cur_token_type_is(Token::IDENT))
    return _tokenizer.consume_token(Token::IDENT);
  else
    return _tokenizer.consume_token(Token::MUL);
}

/*
 * column_field ::= [ id DOT ][ id DOT ] id [ ARROW QUOTE DOLLAR docpath QUOTE ]
 */
Mysqlx::Expr::Expr* Expression_parser::column_field()
{
  std::unique_ptr<Mysqlx::Expr::Expr> e(new Mysqlx::Expr::Expr());
  std::vector<std::string> parts;
  const std::string& part = id();

  if (part == "*")
  {
    e->set_type(Mysqlx::Expr::Expr::OPERATOR);
    e->mutable_operator_()->set_name("*");
    return e.release();
  }

  parts.push_back(part);

  while (_tokenizer.cur_token_type_is(Token::DOT))
  {
    _tokenizer.consume_token(Token::DOT);
    parts.push_back(id());
  }
  if (parts.size() > 3)
  {
    const Token& tok = _tokenizer.peek_token();
    throw Parser_error((boost::format("Too many parts to identifier at position %d (%s)") % tok.get_pos() % tok.get_text()).str());
  }
  Mysqlx::Expr::ColumnIdentifier* colid = e->mutable_identifier();
  std::vector<std::string>::reverse_iterator myend = parts.rend();
  int i{0};
  for (std::vector<std::string>::reverse_iterator it = parts.rbegin(); it != myend; ++it, ++i)
  {
    std::string& s = *it;
    if (i == 0)
      colid->set_name(s.c_str(), s.size());
    else if (i == 1)
      colid->set_table_name(s.c_str(), s.size());
    else if (i == 2)
      colid->set_schema_name(s.c_str(), s.size());
  }
  // Arrow & docpath
  if (_tokenizer.cur_token_type_is(Token::ARROW))
  {
    _tokenizer.consume_token(Token::ARROW);
    _tokenizer.consume_token(Token::QUOTE);
    _tokenizer.consume_token(Token::DOLLAR);
    document_path(*colid);
    _tokenizer.consume_token(Token::QUOTE);
  }
  e->set_type(Mysqlx::Expr::Expr::IDENT);
  return e.release();
}

} // namespace parser

} // namespace mysqlx
