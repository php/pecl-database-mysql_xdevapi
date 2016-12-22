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

#ifndef ORDERBY_PARSER_H
#define ORDERBY_PARSER_H

#include <boost/format.hpp>
#include "xmysqlnd/crud_parsers/compilerutils.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

#include <memory>

namespace xmysqlnd
{
  class Orderby_parser : public Expression_parser
  {
  public:
    Orderby_parser(const std::string& expr_str, const bool document_mode = false);

    template<typename Container>
    void parse(Container &result)
    {
      Mysqlx::Crud::Order *colid = result.Add();
      column_identifier(*colid);

      if (_tokenizer.tokens_available())
      {
        const xmysqlnd::Token& tok = _tokenizer.peek_token();
        throw Parser_error((boost::format("Orderby parser: Expected EOF, instead stopped at token '%s' at position %d") % tok.get_text()
          % tok.get_pos()).str());
      }
    }

    //const std::string& id();
    void column_identifier(Mysqlx::Crud::Order &orderby_expr);

    std::vector<Token>::const_iterator begin() const { return _tokenizer.begin(); }
    std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }
  };
};
#endif /* ORDERBY_PARSER_H */
