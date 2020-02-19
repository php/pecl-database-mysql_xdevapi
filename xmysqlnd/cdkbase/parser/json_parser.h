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
*/

#ifndef _JSON_PARSER_H_
#define _JSON_PARSER_H_

#include <mysql/cdk/common.h>
#include "parser.h"

namespace parser {

using cdk::JSON;

class JSON_parser
  : public JSON
{

  class Error;

  std::string m_json;
public:

  JSON_parser(const std::string &json)
    : m_json(json.begin(), json.end())
  {
    m_json.push_back('\0');
  }

  JSON_parser(std::string &&json)
    : m_json(std::move(json))
  {
    m_json.push_back('\0');
  }

  void process(Processor &prc) const;
};



/*
  Error class for JSON_parse

  It is a specialization of the generic parser::Error_base which defines
  convenience constructors.
*/

class JSON_parser::Error
    : public parser::Error_base
{
public:
  Error(const std::string& parsed_text,
    size_t pos,
    const std::string& desc = string())
    : parser::Error_base(desc, parsed_text, pos)
  {}
};

}  // parser

#endif
