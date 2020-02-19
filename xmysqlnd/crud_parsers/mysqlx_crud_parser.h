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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  +----------------------------------------------------------------------+
*/

#ifndef MYSQLX_CRUD_PARSER_H
#define MYSQLX_CRUD_PARSER_H


#include <string>
#include "util/string_utils.h"
#include "xmysqlnd/proto_gen/mysqlx_expr.pb.h"
#include "xmysqlnd/cdkbase/parser/expr_parser.h"
#include "xmysqlnd/cdkbase/parser/tokenizer.h"
#include "xmysqlnd/cdkbase/protocol/mysqlx/builders.h"
#include "xmysqlnd/cdkbase/mysqlx/converters.h"
#include "xmysqlnd/cdkbase/include/mysql/cdk/api/query.h"
#include "xmysqlnd/cdkbase/include/mysql/cdk/converters.h"
#include "xmysqlnd/cdkbase/mysqlx/delayed_op.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

namespace mysqlx {
namespace devapi {
namespace parser {

class Args_conv : public cdk::protocol::mysqlx::Args_conv
{
public:
	Args_conv( std::vector< std::string >& placeholders );
	unsigned conv_placeholder( const cdk::protocol::mysqlx::string& parm ) override;
private:
	std::vector< std::string >& placeholders;
};

Mysqlx::Expr::Expr* parse(const std::string& expression,
						  const bool doc_datamodel,
						  std::vector<std::string>& placeholders);

Mysqlx::Expr::Expr* parse(const std::string& expression , const bool doc_datamodel);

class Expr_builder
		: public cdk::protocol::mysqlx::Any_builder_base<
		cdk::protocol::mysqlx::Expr_builder_base,
		Mysqlx::Expr::Expr>
{

public:
	Expr_builder() = default;
	Expr_builder(Mysqlx::Expr::Expr& msg,
				 cdk::protocol::mysqlx::Args_conv* conv = nullptr);
};

struct Order_builder
		: public cdk::protocol::mysqlx::Builder_base<
		Mysqlx::Crud::Order,
		cdk::protocol::mysqlx::api::Order_expr::Processor
		>
{
	void reset(Message& msg, cdk::protocol::mysqlx::Args_conv* conv = nullptr);
	Expr_prc* sort_key(cdk::api::Sort_direction::value dir);

	Expr_builder expr_builder;
};

template<class MSG>
struct Ord_msg_traits
{
	typedef MSG                 Array;
	typedef Mysqlx::Crud::Order Msg;

	static Msg& add_element(Array& arr)
	{
		return *arr.add_order();
	}
};

class Order_by_item : public cdk::Expression
{
public:
	Order_by_item(const char* expr,
				  cdk::Sort_direction::value sort_direction,
				  ::parser::Parser_mode::value mode);

	void process(cdk::Expression::Processor& prc) const;
	cdk::Sort_direction::value direction() const;
private:
	::parser::Parser_mode::value parser_mode;
	const char* expression;
	cdk::Sort_direction::value sort_direction;
};

class Order_by : public cdk::Order_by
{
	typedef std::vector<Order_by_item> Order_item_list;
public:
	Order_by(::parser::Parser_mode::value mode);
	void add_item(const char* expr,
				  cdk::Sort_direction::value sort_direction);
	void clear();
	uint32_t count() const;
	void process(Processor& prc) const;
	cdk::Sort_direction::value get_direction(uint32_t pos) const;
private:
	::parser::Parser_mode::value parser_mode;
	Order_item_list item_list;
};

class Expr_to_doc_prc_converter
  : public cdk::Converter<
	  Expr_to_doc_prc_converter,
	  cdk::Expression::Processor,
	  cdk::Expression::Document::Processor
	>
{
  Doc_prc* doc();
  Scalar_prc* scalar();
  List_prc* arr();
};

struct Projection_builder
  : public cdk::protocol::mysqlx::Builder_base<
		Mysqlx::Crud::Projection,
		cdk::protocol::mysqlx::api::Projection::Processor::Element_prc>
{
  void reset(Message& msg, cdk::protocol::mysqlx::Args_conv* conv = nullptr);
  Expr_prc* expr();
  void alias(const cdk::foundation::string& a);

  Expr_builder expression_builder;
};

struct Proj_msg_traits
{
  typedef Mysqlx::Crud::Find       Array;
  typedef Mysqlx::Crud::Projection Msg;

  static Msg& add_element(Array& arr)
  {
	return *arr.add_projection();
  }
};

class Projection_list : public cdk::Projection, public cdk::Expression::Document
{
public:
  Projection_list(bool doc_datamodel);
  void add_value(const char* val);
  void clear();
  uint32_t count() const;
  void process(cdk::Projection::Processor& prc) const;
  void process(cdk::Expression::Document::Processor& prc) const;

  ~Projection_list() {}
private:
  typedef std::vector<cdk::bytes> Proj_vec;
  ::parser::Parser_mode::value parser_mode;
  Proj_vec values;
};

template<typename MSG>
bool orderby(
		const std::string& expression,
		const bool doc_datamodel,
		MSG* message
		)
{
	const std::string parser_asc_symbol = "ASC";
	const std::string parser_desc_symbol = "DESC";
	Order_by orderby(doc_datamodel ? ::parser::Parser_mode::DOCUMENT :
									 ::parser::Parser_mode::TABLE);

	cdk::Sort_direction::value sort_dir = cdk::Sort_direction::value::ASC;
	::parser::Tokenizer tokens( expression );
	std::string expr;
	auto it = tokens.begin();
	while( it != tokens.end() ) {
		std::string criteria = it->get_text();
		std::transform( criteria.begin(),
						criteria.end(),
						criteria.begin(),
						[](char c) { return static_cast<char>(::toupper(c)); });
		if( criteria == parser_asc_symbol ) {
			break;
		}
		else if( criteria == parser_desc_symbol ) {
			sort_dir = cdk::Sort_direction::value::DESC;
			break;
		}
		expr += it->get_text();
		++it;
	}

	orderby.add_item( expr.c_str(), sort_dir);

	std::vector<std::string> ph;
	Args_conv parm_conv(ph);

	cdk::protocol::mysqlx::Array_builder<Order_builder,
			MSG,
			Ord_msg_traits<MSG> > ord_builder;

	ord_builder.reset(*message, &parm_conv);

	cdk::Expr_conv_base<
			cdk::List_prc_converter<cdk::mysqlx::Order_prc_converter>,
			cdk::api::Order_by<cdk::api::Any<cdk::Expr_processor>>,
			cdk::api::Order_by<cdk::api::Any<cdk::protocol::mysqlx::api::Expr_processor>>> conv( orderby );

	conv.process( ord_builder );

	return true;
}

template<typename MSG>
bool projection(
		const std::string& expression,
		const bool doc_datamodel,
		MSG* message
		)
{
	const std::string parser_as_symbol = "AS";
	//Make sure that the expression contains an alias
	::parser::Tokenizer tokens( expression );
	std::string ident;
	std::string target_expr = expression;
	auto it = tokens.begin();
	while( it != tokens.end() ) {
		std::string item = it->get_text();
		std::transform( item.begin(),
						item.end(),
						item.begin(),
						[](char c) { return static_cast<char>(::toupper(c)); });
		if( item == parser_as_symbol ) {
			ident.clear();
			break;
		} else {
			switch( it->get_type() )
			{
			case ::parser::Token::Type::WORD:
			case ::parser::Token::Type::QWORD:
			case ::parser::Token::Type::QSTRING:
			case ::parser::Token::Type::QQSTRING:
				ident = it->get_text();
				break;
			default:
				break;
			}
		}
		++it;
	}

	if( false == ident.empty() ) {
		target_expr += ' ';
		target_expr += parser_as_symbol;
		target_expr += ' ';

		const bool need_backticks{ !util::is_alnum_identifier(ident) };
		if (need_backticks) target_expr += '`';
		target_expr += ident;
		if (need_backticks) target_expr += '`';
	}

	std::vector<std::string> ph;
	Args_conv parm_conv(ph);

	cdk::protocol::mysqlx::Array_builder<
			Projection_builder,
			MSG,
			Proj_msg_traits> proj_builder;

	proj_builder.reset( *message, &parm_conv );

	Projection_list proj_list( doc_datamodel );

	proj_list.add_value( target_expr.c_str() );

	cdk::Expr_conv_base<
			cdk::List_prc_converter<cdk::mysqlx::Table_proj_prc_converter>,
			cdk::api::Projection<cdk::api::Any<cdk::Expr_processor>>,
			cdk::api::Projection<cdk::api::Any<cdk::protocol::mysqlx::api::Expr_processor>>
			> conv( proj_list );

	conv.process( proj_builder );
	return true;
}

} //mysqlx::devapi::parser
} //mysqlx::devapi
} //mysqlx

#endif //MYSQLX_CRUD_PARSER_H
