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

#include "mysqlx_crud_parser.h"

namespace mysqlx {
namespace devapi {
namespace parser {

Args_conv::Args_conv( std::vector<std::string>& placeholders ) :
	placeholders{ placeholders }
{}

unsigned Args_conv::conv_placeholder( const cdk::protocol::mysqlx::string& parm )
{
	const unsigned int next = static_cast<unsigned int>(placeholders.size());
	placeholders.push_back( parm );
	return next;
}

Mysqlx::Expr::Expr* parse( const std::string& expression,
						   const bool doc_datamodel,
						   std::vector< std::string >& placeholders )
{
	Args_conv args_conv( placeholders );
	::parser::Expression_parser expr( doc_datamodel ? ::parser::Parser_mode::DOCUMENT :
									  ::parser::Parser_mode::TABLE,
									  expression.c_str() );

	Mysqlx::Expr::Expr* pb_expr = new Mysqlx::Expr::Expr;
	cdk::protocol::mysqlx::Expr_builder eb( *pb_expr, &args_conv );

	cdk::mysqlx::Expr_converter conv;

	conv.reset( expr );

	conv.process( eb );

	return pb_expr;
}

Mysqlx::Expr::Expr* parse( const std::string& expression,
						   const bool doc_datamodel )
{
	std::vector< std::string > placeholders;
	Mysqlx::Expr::Expr* expr = parse( expression, doc_datamodel, placeholders );
	if ( false == placeholders.empty() ) {
		/*
		 * The expression shouldn't contain placeholders, something
		 * must be wrong better return nullptr
		 */
		delete expr;
		expr = nullptr;
	}
	return expr;
}

Expr_builder::Expr_builder( Mysqlx::Expr::Expr& msg,
							cdk::protocol::mysqlx::Args_conv* conv )
{
	reset( msg, conv );
}

void Order_builder::reset( Builder_base::Message& msg,
						   cdk::protocol::mysqlx::Args_conv* conv )
{
	Builder_base::reset( msg, conv );
	expr_builder.reset( *msg.mutable_expr(), conv );
}

Order_builder::Expr_prc* Order_builder::sort_key( cdk::api::Sort_direction::value dir )
{
	m_msg->set_direction( dir == cdk::api::Sort_direction::ASC ?
						  Message::ASC : Message::DESC );
	return &expr_builder;
}

Order_by_item::Order_by_item( const char* expr,
							  cdk::Sort_direction::value sort_direction,
							  ::parser::Parser_mode::value mode ) :
	parser_mode( mode ),
	expression( expr ),
	sort_direction( sort_direction )
{}

void Order_by_item::process( cdk::Expression::Processor& prc ) const
{
	::parser::Expression_parser parser( parser_mode, expression );
	parser.process( prc );
}

cdk::Sort_direction::value Order_by_item::direction() const
{
	return sort_direction;
}

Order_by::Order_by( ::parser::Parser_mode::value mode ) : parser_mode( mode )
{}

void Order_by::add_item( const char* expr,
						 cdk::Sort_direction::value sort_direction )
{
	item_list.push_back( Order_by_item( expr, sort_direction, parser_mode ) );
}

void Order_by::clear()
{
	item_list.clear();
}

uint32_t Order_by::count() const
{
	return static_cast<uint32_t>(item_list.size());
}

void Order_by::process( Processor& prc ) const
{
	prc.list_begin();
	for ( Order_item_list::const_iterator it = item_list.begin();
			it != item_list.end(); ++it ) {
		Processor* pprc = &prc;
		if ( pprc ) {
			Processor::Element_prc* list_el_ptr = prc.list_el();
			if ( list_el_ptr ) {
				it->process_if( list_el_ptr->sort_key( it->direction() ) );
			}
		}
	}
	prc.list_end();
}

cdk::Sort_direction::value Order_by::get_direction( uint32_t pos ) const
{
	return item_list[pos].direction();
}

Expr_to_doc_prc_converter::Doc_prc* Expr_to_doc_prc_converter::doc()
{
	return m_proc;
}

Expr_to_doc_prc_converter::Scalar_prc* Expr_to_doc_prc_converter::scalar()
{
	throw std::runtime_error( "Document expected" );
}

Expr_to_doc_prc_converter::List_prc* Expr_to_doc_prc_converter::arr()
{
	throw std::runtime_error( "Document expected" );
}

void Projection_builder::reset( Projection_builder::Message& msg,
								cdk::protocol::mysqlx::Args_conv* conv )
{
	Builder_base::reset( msg, conv );
	expression_builder.reset( *msg.mutable_source(), conv );
}

Projection_builder::Expr_prc* Projection_builder::expr()
{
	return &expression_builder;
}

void Projection_builder::alias( const cdk::foundation::string& a )
{
	m_msg->set_alias( a );
}

Projection_list::Projection_list( bool doc_datamodel )
{
	if ( doc_datamodel ) {
		parser_mode = ::parser::Parser_mode::DOCUMENT;
	} else {
		parser_mode = ::parser::Parser_mode::TABLE;
	}
}

void Projection_list::process( cdk::Projection::Processor& prc ) const
{
	prc.list_begin();
	for ( Proj_vec::const_iterator it = values.begin();
			it != values.end(); ++it ) {
		::parser::Projection_parser parser( parser_mode, *it );
		cdk::Projection::Processor::Element_prc* eprc = prc.list_el();
		if ( eprc ) {
			parser.process( *eprc );
		}
	}
	prc.list_end();
}

void Projection_list::add_value( const char* val )
{
	values.push_back( val );
}

void Projection_list::clear()
{
	values.clear();
}

uint32_t Projection_list::count() const
{
	return static_cast<uint32_t>(values.size());
}

void Projection_list::process( cdk::Expression::Document::Processor& prc ) const
{
	::parser::Expression_parser parser( ::parser::Parser_mode::DOCUMENT, values[0] );
	cdk::Expr_conv_base<Expr_to_doc_prc_converter,
		cdk::Expression,
		cdk::Expression::Document> spec;
	spec.reset( parser );
	spec.process( prc );
}

} //mysqlx::devapi::parser
} //mysqlx::devapi
} //mysqlx

