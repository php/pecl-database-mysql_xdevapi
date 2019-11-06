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

#ifndef CRUD_PARSERS_EXPRESSION_PARSER_H
#define CRUD_PARSERS_EXPRESSION_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>

#include <boost/function.hpp>

// Avoid warnings from includes of other project and protobuf
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4018 4996)
#endif

#include "legacy_tokenizer.h"
#include "xmysqlnd/proto_gen/mysqlx_datatypes.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_expr.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif


namespace mysqlx {

namespace old_parser_api {

class Expression_parser
{
  public:
    Expression_parser(const std::string& expr_str, bool document_mode = false, bool allow_alias = false, std::vector<std::string>* place_holders = nullptr);

    typedef boost::function<Mysqlx::Expr::Expr*(Expression_parser*)> inner_parser_t;

    Mysqlx::Expr::Identifier* identifier();
    void docpath_member(Mysqlx::Expr::DocumentPathItem& item);
    void docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item);
    void document_path(Mysqlx::Expr::ColumnIdentifier& colid);
    const std::string& id();
    Mysqlx::Expr::Expr* column_field();

    std::vector<Token>::const_iterator begin() const { return _tokenizer.begin(); }
    std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }

  protected:
    // placeholder
    std::vector<std::string> _place_holders;
    std::vector<std::string>* _place_holder_ref;
    Tokenizer _tokenizer;
    bool _document_mode;
    bool _allow_alias;
};

} // namespace parser

} // namespace mysqlx

#endif /* CRUD_PARSERS_EXPRESSION_PARSER_H */
