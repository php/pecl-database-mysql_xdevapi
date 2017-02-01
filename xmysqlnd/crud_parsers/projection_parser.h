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
  | Authors: Oracle Corp                                                 |
  +----------------------------------------------------------------------+
*/

#ifndef CRUD_PARSERS_PROJECTION_PARSER_H
#define CRUD_PARSERS_PROJECTION_PARSER_H

#include <boost/format.hpp>
#include "xmysqlnd/crud_parsers/compilerutils.h"
#include "xmysqlnd/crud_parsers/expression_parser.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

#include <memory>

namespace mysqlx {

namespace parser {

class Projection_parser : public Expression_parser
{
  public:
    Projection_parser(const std::string& expr_str, bool document_mode = false, bool allow_alias = true);

    template<typename Container>
    void parse(Container &result)
    {
      Mysqlx::Crud::Projection *colid = result.Add();
      source_expression(*colid);

      if (_tokenizer.tokens_available())
      {
        const Token& tok = _tokenizer.peek_token();
        throw Parser_error((boost::format("Projection parser: Expression '%s' has unexpected token '%s' at position %d") % _tokenizer.get_input() % tok.get_text() %
          tok.get_pos()).str());
      }
    }

    const std::string& id();
    void source_expression(Mysqlx::Crud::Projection &column);
  };

} // namespace parser

} // namespace mysqlx

#endif /* CRUD_PARSERS_PROJECTION_PARSER_H */
