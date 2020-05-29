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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_STMT_H
#define XMYSQLND_STMT_H

#include "proto_gen/mysqlx_prepare.pb.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_crud_collection_commands.h"
#include "xmysqlnd_wireprotocol.h" /* struct st_xmysqlnd_msg__sql_stmt_execute */
#include "util/allocator.h"
#include "util/strings.h"
#include "util/exceptions.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_session;
typedef std::shared_ptr<xmysqlnd_session> XMYSQLND_SESSION;
struct st_xmysqlnd_stmt_result;
struct st_xmysqlnd_stmt_execution_state;
class xmysqlnd_warning_list;
struct st_xmysqlnd_rowset;

struct st_xmysqlnd_stmt_on_result_start_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt);
	void * ctx;
};

struct st_xmysqlnd_stmt_on_row_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const st_xmysqlnd_stmt_result_meta * const meta, const zval * const row, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_warning_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const enum xmysqlnd_stmt_warning_level level, const unsigned int code, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const unsigned int code, const util::string_view& sql_state, const util::string_view& message);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_result_end_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const zend_bool has_more);
	void * ctx;
};


struct st_xmysqlnd_stmt_on_statement_ok_bind
{
	const enum_hnd_func_status (*handler)(void * context, xmysqlnd_stmt * const stmt, const st_xmysqlnd_stmt_execution_state * const exec_state);
	void * ctx;
};

typedef struct st_xmysqlnd_rowset *                                    (*func_xmysqlnd_stmt__create_rowset)(void * ctx);

struct st_xmysqlnd_stmt_bind_ctx
{
	xmysqlnd_stmt* stmt;
	MYSQLND_STATS* stats;
	MYSQLND_ERROR_INFO* error_info;
	func_xmysqlnd_stmt__create_rowset create_rowset;
	size_t fwd_prefetch_count;
	size_t prefetch_counter;
	zval* current_row;
	st_xmysqlnd_rowset* rowset;
	st_xmysqlnd_stmt_result_meta* meta;
	st_xmysqlnd_stmt_result* result;
	xmysqlnd_warning_list* warnings;
	st_xmysqlnd_stmt_execution_state* exec_state;

	st_xmysqlnd_stmt_on_row_bind on_row;
	st_xmysqlnd_stmt_on_warning_bind on_warning;
	st_xmysqlnd_stmt_on_error_bind on_error;
	st_xmysqlnd_stmt_on_result_end_bind on_resultset_end;
	st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok;
};

class xmysqlnd_stmt : public util::custom_allocable
{
public:
	xmysqlnd_stmt() = default;
	xmysqlnd_stmt(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const obj_factory,
						 XMYSQLND_SESSION cur_session);
	enum_func_status	send_raw_message(xmysqlnd_stmt * const stmt,
										 const st_xmysqlnd_pb_message_shell message_shell,
										 MYSQLND_STATS * const stats,
										 MYSQLND_ERROR_INFO * const error_info);


	enum_func_status	read_one_result(xmysqlnd_stmt * const stmt,
										const st_xmysqlnd_stmt_on_row_bind on_row,
										const st_xmysqlnd_stmt_on_warning_bind on_warning,
										const st_xmysqlnd_stmt_on_error_bind on_error,
										const st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
										const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
										zend_bool * const has_more_results,
										MYSQLND_STATS * const stats,
										MYSQLND_ERROR_INFO * const error_info);

	enum_func_status	read_all_results(xmysqlnd_stmt * const stmt,
										 const st_xmysqlnd_stmt_on_row_bind on_row,
										const st_xmysqlnd_stmt_on_warning_bind on_warning,
										 const st_xmysqlnd_stmt_on_error_bind on_error,
										 const st_xmysqlnd_stmt_on_result_start_bind on_result_start,
										 const st_xmysqlnd_stmt_on_result_end_bind on_resultset_end,
										 const st_xmysqlnd_stmt_on_statement_ok_bind on_statement_ok,
										 MYSQLND_STATS * const stats,
										 MYSQLND_ERROR_INFO * const error_info);
	zend_bool           has_more_results(xmysqlnd_stmt * stmt);

	st_xmysqlnd_stmt_result *		get_buffered_result(xmysqlnd_stmt * const stmt,
													zend_bool * const has_more_results,
													const st_xmysqlnd_stmt_on_warning_bind on_warning,
													const st_xmysqlnd_stmt_on_error_bind on_error,
													MYSQLND_STATS * const stats,
													MYSQLND_ERROR_INFO * const error_info);

	st_xmysqlnd_stmt_result *		get_fwd_result(xmysqlnd_stmt * const stmt,
												const size_t rows,
												zend_bool * const has_more_rows_in_set,
												zend_bool * const has_more_results,
												const st_xmysqlnd_stmt_on_warning_bind on_warning,
												const st_xmysqlnd_stmt_on_error_bind on_error,
												MYSQLND_STATS * const stats,
												MYSQLND_ERROR_INFO * const error_info);

	enum_func_status				skip_one_result(xmysqlnd_stmt * const stmt, zend_bool * const has_more_results, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	enum_func_status				skip_all_results(xmysqlnd_stmt * const stmt, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
	st_xmysqlnd_stmt_result_meta *	create_meta(void * ctx);
	st_xmysqlnd_stmt_execution_state *	create_execution_state(void * ctx);
	xmysqlnd_warning_list *			create_warning_list(void * ctx);
	xmysqlnd_stmt *					get_reference(xmysqlnd_stmt * const stmt);
	enum_func_status				free_reference(xmysqlnd_stmt * const stmt);
	void							free_contents(xmysqlnd_stmt * const stmt);
	void							cleanup(xmysqlnd_stmt * const stmt);
	XMYSQLND_SESSION				get_session() {
		return session;
	}
	st_xmysqlnd_msg__sql_stmt_execute& get_msg_stmt_exec() {
		return msg_stmt_exec;
	}
	st_xmysqlnd_stmt_bind_ctx& get_read_ctx() {
		return read_ctx;
	}
	zend_bool get_persistent() {
		return persistent;
	}
private:
	XMYSQLND_SESSION session;
	st_xmysqlnd_msg__sql_stmt_execute msg_stmt_exec;
	st_xmysqlnd_stmt_bind_ctx read_ctx;
	zend_bool         partial_read_started;
	unsigned int	refcount;
	zend_bool		persistent;
public: //To be removed anyway
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;
};


xmysqlnd_stmt * xmysqlnd_stmt_create(XMYSQLND_SESSION session,
													  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
													  MYSQLND_STATS * const stats,
													  MYSQLND_ERROR_INFO * const error_info);

void xmysqlnd_stmt_free(xmysqlnd_stmt* const result, MYSQLND_STATS* stats, MYSQLND_ERROR_INFO* error_info);

struct Prepare_statement_entry
{
	std::string                             type_name;
	std::string                             serialized_message;
	uint32_t                                msg_id;
	Mysqlx::Prepare::Prepare                prepare_msg;
	bool                                    delivered_ps;
	std::vector<Mysqlx::Datatypes::Scalar*> bound_values;
	bool                                    is_bind_finalized;
	uint64_t                                row_count;
	bool                                    has_row_count;
	uint64_t                                offset;
	bool                                    has_offset;
	Prepare_statement_entry() :
		msg_id{ 0 },
		is_bind_finalized{ false },
		row_count{0},
		has_row_count{ false },
		offset{ 0 },
		has_offset{ false }
	{}
	bool operator< (const Prepare_statement_entry& entry ) const {
		return (type_name < entry.type_name) || (serialized_message < entry.serialized_message);
	}
};

const enum_hnd_func_status prepare_st_on_error_handler(void * context,
												 const unsigned int code,
												 const util::string_view& sql_state,
												 const util::string_view& message);

#define DEFAULT_PS_ID 1

class Prepare_stmt_data : public util::custom_allocable
{
public:
	Prepare_stmt_data();
	template< typename MSG_T >
	std::pair<bool,uint32_t>     add_message( MSG_T& msg, const uint32_t bound_values );
	template< typename MSG_T >
	st_xmysqlnd_pb_message_shell get_protobuf_msg( MSG_T*,uint32_t );
	bool                         send_prepare_msg( uint32_t message_id );
	xmysqlnd_stmt *              send_execute_msg( uint32_t message_id );
	bool                         bind_values( uint32_t message_id, std::vector<Mysqlx::Datatypes::Scalar*> bound_values);
	bool                         bind_values( uint32_t message_id, zval* params, unsigned int params_allocated);
	void                         assign_session( XMYSQLND_SESSION session_obj );
	bool                         prepare_msg_delivered( const uint32_t message_id );
	void                         set_supported_ps( bool supported );
	bool                         is_ps_supported() const;
	bool                         is_bind_finalized( const uint32_t message_id );
	void                         set_finalized_bind( const uint32_t message_id, const bool finalized );
    void                         set_ps_server_error( const uint32_t message_code );
private:
	template< typename MSG_T >
	Prepare_statement_entry      prepare_ps_entry( const MSG_T& msg);
	size_t                       get_ps_entry( const google::protobuf::Message& msg );
	size_t                       get_ps_entry( const uint32_t msg_id );
	bool                         get_prepare_resp( drv::xmysqlnd_stmt * stmt );
	template< typename MSG_T >
	void                         handle_limit_expr( Prepare_statement_entry& prepare, MSG_T* msg,uint32_t bound_values_count );
	template< typename MSG_T >
	void                         add_limit_expr( MSG_T * msg, const uint32_t position );
	void                         add_limit_expr_mutable_arg( Mysqlx::Prepare::Execute& execute_msg, const int32_t value );
private:
	uint32_t                          next_ps_id;
	bool                              ps_supported;
	XMYSQLND_SESSION                  session;
    uint32_t                          ps_deliver_message_code;
	std::vector< Prepare_statement_entry > ps_db;
	template< typename T >
	void set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg, T msg );
};

template< typename T >
void Prepare_stmt_data::set_allocated_type( Mysqlx::Prepare::Prepare_OneOfMessage* one_msg, T msg )
{
    throw util::xdevapi_exception(util::xdevapi_exception::Code::ps_unknown_message);
}

template<>
void Prepare_stmt_data::set_allocated_type(Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
	Mysqlx::Crud::Insert* msg);

template<>
void Prepare_stmt_data::set_allocated_type(Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
	Mysqlx::Crud::Find* msg);

template<>
void Prepare_stmt_data::set_allocated_type(Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
	Mysqlx::Crud::Update* msg);

template<>
void Prepare_stmt_data::set_allocated_type(Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
	Mysqlx::Crud::Delete* msg);

template<>
void Prepare_stmt_data::set_allocated_type(Mysqlx::Prepare::Prepare_OneOfMessage* one_msg,
	Mysqlx::Sql::StmtExecute* msg);


template< typename MSG_T >
Prepare_statement_entry
Prepare_stmt_data::prepare_ps_entry( const MSG_T &msg )
{
	Prepare_statement_entry new_entry;
	new_entry.type_name = msg.GetTypeName();
	return new_entry;
}

template< typename MSG_T >
void Prepare_stmt_data::add_limit_expr(
		MSG_T * msg,
		const uint32_t position
		)
{
	Mysqlx::Crud::LimitExpr * limit_expr;
	Mysqlx::Expr::Expr *      expr;

	limit_expr = new Mysqlx::Crud::LimitExpr;
	expr = new Mysqlx::Expr::Expr;

	expr->set_type( Mysqlx::Expr::Expr_Type::Expr_Type_PLACEHOLDER );
	expr->set_position( position );
	limit_expr->set_allocated_row_count( expr );
	msg->set_allocated_limit_expr( limit_expr );
}

template< typename MSG_T >
void Prepare_stmt_data::handle_limit_expr(
		Prepare_statement_entry& prepare,
		MSG_T* msg,
		const uint32_t bound_values_count
)
{
	//We should never be here.
    throw util::xdevapi_exception(util::xdevapi_exception::Code::ps_limit_not_supported);
}

template<>
void Prepare_stmt_data::handle_limit_expr(
	Prepare_statement_entry& prepare,
	Mysqlx::Crud::Update* msg,
	uint32_t bound_values_count);

template<>
void Prepare_stmt_data::handle_limit_expr(
	Prepare_statement_entry& prepare,
	Mysqlx::Crud::Find* msg,
	uint32_t bound_values_count);

template<>
void Prepare_stmt_data::handle_limit_expr(
	Prepare_statement_entry& prepare,
	Mysqlx::Crud::Delete* msg,
	uint32_t bound_values_count);

template<>
void Prepare_stmt_data::handle_limit_expr(
	Prepare_statement_entry& prepare,
	Mysqlx::Sql::StmtExecute* msg,
	uint32_t bound_values_count);


template< typename MSG_T >
std::pair<bool,uint32_t> Prepare_stmt_data::add_message(
		MSG_T& msg,
		const uint32_t bound_values
)
{
	if( ps_supported == false ) {
		return { false, 0 };
	}

	if( msg.mutable_args()->size() > 0 ) {
		msg.mutable_args()->Clear();
	}

	Prepare_statement_entry                new_entry = prepare_ps_entry( msg );
	MSG_T*                                 model_clone = nullptr;
	Mysqlx::Prepare::Prepare_OneOfMessage* one_message = nullptr;

	model_clone = new MSG_T;
	one_message = new Mysqlx::Prepare::Prepare_OneOfMessage;

	model_clone->CopyFrom( msg );
	handle_limit_expr( new_entry, model_clone, bound_values);
	set_allocated_type( one_message, model_clone );

	new_entry.prepare_msg.set_allocated_stmt( one_message );
	new_entry.prepare_msg.set_stmt_id( 0 );
	new_entry.msg_id = next_ps_id;
	new_entry.serialized_message = new_entry.prepare_msg.SerializeAsString();

	uint32_t db_idx = static_cast<uint32_t>(get_ps_entry( new_entry.prepare_msg ));
	if( db_idx > ps_db.size() ) {
		new_entry.prepare_msg.set_stmt_id( next_ps_id++ );
		ps_db.push_back( new_entry );
		return { true, new_entry.msg_id };
	} else {
		ps_db[ db_idx ].row_count = new_entry.row_count;
		ps_db[ db_idx ].offset = new_entry.offset;
	}
	return { false, ps_db[ db_idx ].msg_id };
}

template< typename MSG_T >
st_xmysqlnd_pb_message_shell
Prepare_stmt_data::get_protobuf_msg(
			MSG_T* prepare,
			uint32_t com_id
)
{
	st_xmysqlnd_pb_message_shell ret = { (void *) prepare, com_id };
	return ret;
}


} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STMT_H */
