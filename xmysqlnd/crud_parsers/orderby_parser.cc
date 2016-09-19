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
extern "C"
{
#include <php.h>
#undef ERROR
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}


#include "xmysqlnd/crud_parsers/orderby_parser.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>


using namespace xmysqlnd;

Orderby_parser::Orderby_parser(const std::string& expr_str, const bool document_mode)
: Expression_parser(expr_str, document_mode)
{
}

/*
* document_mode = false:
*   column_identifier ::= expr ( ASC | DESC )?
*/
void Orderby_parser::column_identifier(Mysqlx::Crud::Order &orderby_expr)
{
  DBG_ENTER("Orderby_parser::column_identifier");
  orderby_expr.set_allocated_expr(my_expr());

  if (_tokenizer.cur_token_type_is(Token::ASC))
  {
    DBG_INF("ASC");
    orderby_expr.set_direction(Mysqlx::Crud::Order_Direction_ASC);
    _tokenizer.consume_token(Token::ASC);
  }
  else if (_tokenizer.cur_token_type_is(Token::DESC))
  {
    DBG_INF("DESC");
    orderby_expr.set_direction(Mysqlx::Crud::Order_Direction_DESC);
    _tokenizer.consume_token(Token::DESC);
  }
  DBG_VOID_RETURN;
}
