/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#ifndef _JSON_PARSER_H_
#define _JSON_PARSER_H_

#include <mysql/cdk/common.h>
#include "parser.h"

namespace parser {

using cdk::JSON;
class JSON_parser;

/*
  Specialization of Token_base, which can clasify base tokens as JSON tokens.
*/

class JSON_token_base
  : public Token_base
{
protected:

  using Token_base::Error;

public:

  enum Token_type {
    OTHER,
    STRING, NUMBER, INTEGER,
    PLUS, MINUS, T_NULL, T_TRUE, T_FALSE,
  };

  static Token_type get_jtype(const Token&);

  friend JSON_parser;
};



/*
  JSON_parser is build using generic Doc_parser<> template and base
  JSON_sclar_parser which parses scalar values (numbers, strings, Booleans).
  Parsing of arrays and sub-documents is handled by Doc_parser<> logic.
*/


class JSON_scalar_parser
  : public Expr_parser<cdk::JSON_processor, JSON_token_base>
{
public:

  JSON_scalar_parser(It &first, const It &last)
    : Expr_parser<cdk::JSON_processor, JSON_token_base>(first, last)
  {}

  static Processor *get_base_prc(JSON::Processor::Any_prc *prc)
  { return prc->scalar(); }

private:

  bool do_parse(Processor*);
};



class JSON_parser
  : public JSON
{
  Tokenizer m_toks;

public:

  JSON_parser(const cdk::string &json)
    : m_toks(json)
  {}

  void process(Processor &prc) const
  {
    It first = m_toks.begin();
    It last  = m_toks.end();

    if (m_toks.empty())
      throw JSON_token_base::Error(first, L"Expectiong JSON document string");

    /*
      Note: passing m_toks.end() directly as constructor argument results
      in "incompatible iterators" exception when comparing iterators (at
      least on win, VS2010). Problem with passing temporary object?
    */

    Doc_parser<JSON_scalar_parser> parser(first, last);
    if (!parser.parse(prc) || first != last)
      throw JSON_token_base::Error(
              first,
              L"Unexpected characters after parsing JSON string"
            );
  }

};

}  // parser

#endif
