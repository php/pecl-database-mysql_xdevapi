/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  +----------------------------------------------------------------------+
*/
#include "php.h"
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_connection.h>
#include "xmysqlnd.h"
#include "xmysqlnd_wireprotocol.h"
#include "messages/mysqlx_message__capabilities.h"
#include "xmysqlnd_zval2any.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "proto_gen/mysqlx_notice.pb.h"
#include "proto_gen/mysqlx_resultset.pb.h"
#include "proto_gen/mysqlx_session.pb.h"
#include "proto_gen/mysqlx_sql.pb.h"

typedef enum xmysqlnd_handler_func_status
{
	HND_PASS = PASS,
	HND_FAIL = FAIL,
	HND_PASS_RETURN_FAIL,
	HND_AGAIN,
	HND_AGAIN_ASYNC,
} enum_hnd_func_status;


/* {{{ xmysqlnd_client_message_type_is_valid */
zend_bool
xmysqlnd_client_message_type_is_valid(enum xmysqlnd_client_message_type type)
{
	return Mysqlx::ClientMessages::Type_IsValid((Mysqlx::ClientMessages_Type) type);
}
/* }}} */


/* {{{ xmysqlnd_server_message_type_is_valid */
zend_bool
xmysqlnd_server_message_type_is_valid(zend_uchar type)
{
	DBG_ENTER("xmysqlnd_server_message_type_is_valid");
	zend_bool ret = Mysqlx::ServerMessages::Type_IsValid((Mysqlx::ServerMessages_Type) type);
	if (ret) {
		DBG_INF_FMT("TYPE=%s", Mysqlx::ServerMessages::Type_Name((Mysqlx::ServerMessages_Type) type).c_str());
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ xmysqlnd_dump_column_identifier */
static void
xmysqlnd_dump_column_identifier(const Mysqlx::Expr::ColumnIdentifier & column)
{
	DBG_ENTER("xmysqlnd_dump_column_identifier");

	const bool has_name = column.has_name();
	DBG_INF_FMT("name[%s] is %s", has_name? "SET":"NOT SET",
								  has_name? column.name().c_str() : "n/a");

	const bool has_table_name = column.has_table_name();
	DBG_INF_FMT("table_name[%s] is %s", has_table_name? "SET":"NOT SET",
										has_table_name? column.table_name().c_str() : "n/a");

	const bool has_schema_name = column.has_schema_name();
	DBG_INF_FMT("table_name[%s] is %s", has_schema_name? "SET":"NOT SET",
										has_schema_name? column.schema_name().c_str() : "n/a");
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_expr_doc_path_item */
static void
xmysqlnd_dump_expr_doc_path_item(const Mysqlx::Expr::DocumentPathItem & item)
{
	DBG_ENTER("xmysqlnd_dump_expr_doc_path_item");

	const bool has_type = item.has_type();
	DBG_INF_FMT("type [%s] is %s", has_type? "SET":"NOT SET",
								   has_type? Mysqlx::Expr::DocumentPathItem::Type_Name(item.type()).c_str() : "n/a");

	const bool has_value = item.has_value();
	DBG_INF_FMT("value[%s] is %s", has_value? "SET":"NOT SET",
								   has_value? item.value().c_str() : "n/a");

	const bool has_index = item.has_index();
	DBG_INF_FMT("index[%s] is %u", has_index? "SET":"NOT SET",
								   has_index? item.index() : 0);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_column */
static void
xmysqlnd_dump_column(const Mysqlx::Crud::Column & column)
{
	DBG_ENTER("xmysqlnd_dump_column");

	const bool has_name = column.has_name();
	DBG_INF_FMT("name[%s] is %s", has_name? "SET":"NOT SET",
								  has_name? column.name().c_str() : "n/a");
	
	const bool has_alias = column.has_alias();
	DBG_INF_FMT("alias[%s] is %s", has_alias? "SET":"NOT SET",
								   has_alias? column.alias().c_str() : "n/a");

	DBG_INF_FMT("%d doc_paths", column.document_path_size());
	for (unsigned int i = 0; i < column.document_path_size(); ++i) {
		xmysqlnd_dump_expr_doc_path_item(column.document_path(i));	
	}
	DBG_VOID_RETURN;
}
/* }}} */

static void xmysqlnd_dump_expr(const Mysqlx::Expr::Expr & expr);

/* {{{ xmysqlnd_dump_function_call */
static void
xmysqlnd_dump_function_call(const Mysqlx::Expr::FunctionCall & fc)
{
	DBG_ENTER("xmysqlnd_dump_function_call");

	const bool has_name = fc.has_name();
	DBG_INF_FMT("fc::name[%s] is %s", has_name? "SET":"NOT SET");
	if (has_name) {
		const bool has_ident_name = fc.name().has_name();
		DBG_INF_FMT("identifier::name[%s] is %s", has_ident_name? "SET":"NOT SET",
												  has_ident_name? fc.name().name().c_str() : "n/a");

		const bool has_ident_schema = fc.name().has_schema_name();
		DBG_INF_FMT("identifier::schema[%s] is %s", has_ident_schema? "SET":"NOT SET",
													has_ident_schema? fc.name().schema_name().c_str() : "n/a");
	
	}

	DBG_INF_FMT("%d fc::params", fc.param_size());
	for (unsigned int i = 0; i < fc.param_size(); ++i) {
		xmysqlnd_dump_expr(fc.param(i));
	}	

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_operator */
static void
xmysqlnd_dump_operator(const Mysqlx::Expr::Operator & op)
{
	DBG_ENTER("xmysqlnd_dump_operator");

	const bool has_name = op.has_name();
	DBG_INF_FMT("name[%s] is %s", has_name? "SET":"NOT SET",
								  has_name? op.name().c_str() : "n/a");

	DBG_INF_FMT("%d params", op.param_size());
	for (unsigned int i = 0; i < op.param_size(); ++i) {
		xmysqlnd_dump_expr(op.param(i));
	}	
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_expr */
static void
xmysqlnd_dump_expr(const Mysqlx::Expr::Expr & expr)
{
	DBG_ENTER("xmysqlnd_dump_expr");

	const bool has_type = expr.has_type();
	DBG_INF_FMT("type [%s] is %s", has_type? "SET":"NOT SET",
								   has_type? Mysqlx::Expr::Expr::Type_Name(expr.type()).c_str() : "n/a");

	const bool has_identifier = expr.has_identifier();
	DBG_INF_FMT("identifier is %s", has_identifier? "SET":"NOT SET");
	if (has_identifier) {
		xmysqlnd_dump_column_identifier(expr.identifier());
	}

	const bool has_variable = expr.has_variable();
	DBG_INF_FMT("variable [%s] is %s", has_variable? "SET":"NOT SET",
									   has_variable? expr.variable().c_str() : "n/a");

	const bool has_literal = expr.has_literal();
	DBG_INF_FMT("literal is %s", has_literal? "SET":"NOT SET");
	if (has_literal) {
		scalar2log(expr.literal());
	}

	const bool has_function_call = expr.has_function_call();
	DBG_INF_FMT("function_call is %s", has_literal? "SET":"NOT SET");
	if (has_function_call) {
		xmysqlnd_dump_function_call(expr.function_call());
	}

	const bool has_operator = expr.has_operator_();
	DBG_INF_FMT("operator is %s", has_operator? "SET":"NOT SET");
	if (has_operator) {
		xmysqlnd_dump_operator(expr.operator_());
	}

	const bool has_position = expr.has_position();
	DBG_INF_FMT("position[%s] is %u", has_position? "SET":"NOT SET",
									  has_position? expr.position() : 0);

	const bool has_object = expr.has_object();
	DBG_INF_FMT("object is %s", has_object? "SET":"NOT SET");
	if (has_object) {
		DBG_INF_FMT("%d fields", expr.object().fld_size());
		for (unsigned int i = 0; i < expr.object().fld_size(); ++i) {
			const Mysqlx::Expr::Object::ObjectField & field = expr.object().fld(i);

			const bool has_obj_key = field.has_key();
			DBG_INF_FMT("obj_key [%s] is %s", has_obj_key? "SET":"NOT SET",
											  has_obj_key? field.key().c_str() : "n/a");
		
			const bool has_obj_value = field.has_value();
			DBG_INF_FMT("obj_value is %s", has_obj_value? "SET":"NOT SET");
			if (has_obj_value) {
				xmysqlnd_dump_expr(field.value());
			}
		}
	}

	const bool has_array = expr.has_array();
	DBG_INF_FMT("array is %s", has_array? "SET":"NOT SET");
	if (has_array) {
		DBG_INF_FMT("%d array elements", expr.array().value_size());
		for (unsigned int i = 0; i < expr.array().value_size(); ++i) {
			xmysqlnd_dump_expr(expr.array().value(i));
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_update_operation */
static void
xmysqlnd_dump_update_operation(const Mysqlx::Crud::UpdateOperation & op)
{
	DBG_ENTER("xmysqlnd_dump_update_operation");

	const bool has_source = op.has_source();
	DBG_INF_FMT("source is %s", has_source? "SET":"NOT SET");
	if (has_source) {
		xmysqlnd_dump_column_identifier(op.source());
	}

	const bool has_operation = op.has_operation();
	DBG_INF_FMT("operation[%s] is %s", has_operation? "SET":"NOT SET",
									   has_operation? Mysqlx::Crud::UpdateOperation::UpdateType_Name(op.operation()).c_str() : "n/a");

	const bool has_value = op.has_value();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_value) {
		xmysqlnd_dump_expr(op.value());
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_client_message */
extern "C" void
xmysqlnd_dump_client_message(const zend_uchar packet_type, const void * payload, const size_t payload_size)
{
	DBG_ENTER("xmysqlnd_dump_client_message");
	const Mysqlx::ClientMessages_Type type = (Mysqlx::ClientMessages_Type)(packet_type);
	DBG_INF_FMT("packet is %s   payload_size=%u", Mysqlx::ClientMessages_Type_Name(type).c_str(), (uint) payload_size);
	{
		static char hexconvtab[] = "0123456789abcdef";
		char * message_dump = new char[payload_size*3 + 1];
		message_dump[payload_size*3] = '\0';
		for (unsigned int i = 0; i < payload_size; i++) {
			message_dump[i*3+0] = hexconvtab[((const char*)payload)[i] >> 4];
			message_dump[i*3+1] = hexconvtab[((const char*)payload)[i] & 15];
			message_dump[i*3+2] = ' ';
		}
		DBG_INF_FMT("payload[%u]=[%*s]", (uint) payload_size, payload_size, message_dump);
		delete [] message_dump;
	}
	switch (type) {
		case Mysqlx::ClientMessages_Type_CON_CAPABILITIES_GET:
			break;
		case Mysqlx::ClientMessages_Type_CON_CAPABILITIES_SET:{
			Mysqlx::Connection::CapabilitiesSet message;
			message.ParseFromArray(payload, payload_size);
			DBG_INF_FMT("capabilities is %s", message.has_capabilities()? "SET":"NOT SET");
			if (message.has_capabilities()) {
				unsigned int caps = message.capabilities().capabilities_size();
				DBG_INF_FMT("%d capabilit%s", caps, caps == 1? "y":"ies");
				for (unsigned int i = 0; i < caps; ++i) {
					const Mysqlx::Connection::Capability & capability =  message.capabilities().capabilities(i);

					DBG_INF_FMT("name is %s", capability.has_name()? "SET":"NOT SET");
					if (capability.has_name()) {
						DBG_INF_FMT("cap_name=[%s]", capability.name().c_str());
					}

					DBG_INF_FMT("value is %s", capability.has_value()? "SET":"NOT SET");
					if (capability.has_value()) {
						any2log(capability.value());
					}
				}
			}
			break;
		}
		case Mysqlx::ClientMessages_Type_CON_CLOSE: break; /* Empty */
		case Mysqlx::ClientMessages_Type_SESS_AUTHENTICATE_START:{
			Mysqlx::Session::AuthenticateStart message;
			message.ParseFromArray(payload, payload_size);
			const bool has_mech_name = message.has_mech_name();
			const bool has_auth_data = message.has_auth_data();
			const bool has_initial_response = message.has_initial_response();
			
			DBG_INF_FMT("mech_name[%s]=[%s]", has_mech_name? "SET":"NOT SET",
											  has_mech_name? message.mech_name().c_str() : "n/a");
			DBG_INF_FMT("auth_data[%s]=[%*s]", has_auth_data? "SET":"NOT SET",
											   has_auth_data? message.auth_data().size() : sizeof("n/a") - 1,
											   has_auth_data? message.auth_data().c_str() : "n/a");
			DBG_INF_FMT("initial_response[%s]=[%*s]", has_initial_response? "SET":"NOT SET",
													  has_initial_response? message.initial_response().size() : sizeof("n/a") - 1,
													  has_initial_response? message.initial_response().c_str() : "n/a");
			break;
		}
		case Mysqlx::ClientMessages_Type_SESS_AUTHENTICATE_CONTINUE:{
			Mysqlx::Session::AuthenticateContinue message;
			message.ParseFromArray(payload, payload_size);
			const bool has_auth_data = message.has_auth_data();
			message.ParseFromArray(payload, payload_size);
			DBG_INF_FMT("message[%s]=%*s ",	has_auth_data? "SET":"NOT SET",
											has_auth_data? message.auth_data().size() : sizeof("n/a") - 1,
											has_auth_data? message.auth_data().c_str() : "n/a");			
			break;
		}
		case Mysqlx::ClientMessages_Type_SESS_RESET: break; /* Empty */
		case Mysqlx::ClientMessages_Type_SESS_CLOSE: break; /* Empty */
		case Mysqlx::ClientMessages_Type_SQL_STMT_EXECUTE:{
			Mysqlx::Sql::StmtExecute message;
			message.ParseFromArray(payload, payload_size);

			const bool has_namespace = message.has_namespace_();
			const bool has_stmt = message.has_stmt();
			const bool has_metadata = message.has_compact_metadata();

			DBG_INF_FMT("namespace[%s]=[%s]", has_namespace? "SET":"NOT SET",
											  has_namespace? message.namespace_().c_str() : "n/a");
			DBG_INF_FMT("stmt     [%s]=[%*s]", has_stmt? "SET":"NOT SET",
											   has_stmt? message.stmt().size() : sizeof("n/a") - 1,
											   has_stmt? message.stmt().c_str() : "n/a");
			DBG_INF_FMT("comp_meta[%s]=[%s]", has_metadata? "SET":"NOT SET",
											  has_metadata? (message.compact_metadata()? "YES":"NO") : "n/a");
			DBG_INF_FMT("%d arguments", message.args_size());
			if (message.args_size()) {
				for (unsigned int i = 0; i < message.args_size(); ++i) {
					any2log(message.args(i));
				}
			}
			break;
		}
		case Mysqlx::ClientMessages_Type_CRUD_FIND:{
			Mysqlx::Crud::Find message;		
			message.ParseFromArray(payload, payload_size);

			const bool has_collection = message.has_collection();
			DBG_INF_FMT("collection is %s", has_collection? "SET":"NOT SET");
			if (has_collection) {
				const Mysqlx::Crud::Collection & collection = message.collection();
				DBG_INF_FMT("[%s].[%s]", collection.has_schema()? collection.schema().c_str() :"n/a",
										 collection.has_name()? collection.name().c_str() :"n/a");
			}
			
			const bool has_data_model = message.has_data_model();
			DBG_INF_FMT("data_model[%s]=[%s]", has_data_model? "SET":"NOT SET",
											   has_data_model? Mysqlx::Crud::DataModel_Name(message.data_model()).c_str() : "n/a");

			/* projection */

			const bool has_criteria = message.has_criteria();
			DBG_INF_FMT("criteria is %s", has_criteria? "SET":"NOT SET");
			if (has_criteria) {
				xmysqlnd_dump_expr(message.criteria());
			}

			DBG_INF_FMT("%d arguments", message.args_size());
			if (message.args_size()) {
				for (unsigned int i = 0; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]="MYSQLND_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]="MYSQLND_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (unsigned int i = 0; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");

					const bool has_direction = order.has_direction();
					DBG_INF_FMT("direction[%s]=[%s]", has_direction? "SET":"NOT SET",
													  has_direction? Mysqlx::Crud::Order::Direction_Name(order.direction()).c_str() : "n/a");
				}
			}

			DBG_INF_FMT("grouping_size=%d", message.grouping_size());
			if (message.grouping_size()) {
				for (unsigned int i = 0; i < message.grouping_size(); ++i) {
					xmysqlnd_dump_expr(message.grouping(i));
				}
			}

			const bool has_grouping_criteria = message.has_grouping_criteria();
			DBG_INF_FMT("grouping_criteria is %s", has_grouping_criteria? "SET":"NOT SET");
			if (has_grouping_criteria) {
				xmysqlnd_dump_expr(message.grouping_criteria());
			}

			break;
		}
		case Mysqlx::ClientMessages_Type_CRUD_INSERT:{
			Mysqlx::Crud::Insert message;
			message.ParseFromArray(payload, payload_size);

			const bool has_collection = message.has_collection();
			DBG_INF_FMT("collection %s", has_collection? "SET":"NOT SET");
			if (has_collection) {
				const Mysqlx::Crud::Collection & collection = message.collection();
				DBG_INF_FMT("[%s].[%s]", collection.has_schema()? collection.schema().c_str() :"n/a",
										 collection.has_name()? collection.name().c_str() :"n/a");
			}

			const bool has_data_model = message.has_data_model();
			DBG_INF_FMT("data_model[%s]=[%s]", has_data_model? "SET":"NOT SET",
											   has_data_model? Mysqlx::Crud::DataModel_Name(message.data_model()).c_str() : "n/a");

			/* projection */

			/* typedRow */

			/* row */

			DBG_INF_FMT("%d arguments", message.args_size());
			if (message.args_size()) {
				for (unsigned int i = 0; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			break;
		}
		case Mysqlx::ClientMessages_Type_CRUD_UPDATE:{
			Mysqlx::Crud::Update message;
			message.ParseFromArray(payload, payload_size);

			const bool has_collection = message.has_collection();
			DBG_INF_FMT("collection is %s", has_collection? "SET":"NOT SET");
			if (has_collection) {
				const Mysqlx::Crud::Collection & collection = message.collection();
				DBG_INF_FMT("[%s].[%s]", collection.has_schema()? collection.schema().c_str() :"n/a",
										 collection.has_name()? collection.name().c_str() :"n/a");
			}

			const bool has_data_model = message.has_data_model();
			DBG_INF_FMT("data_model[%s]=[%s]", has_data_model? "SET":"NOT SET",
											   has_data_model? Mysqlx::Crud::DataModel_Name(message.data_model()).c_str() : "n/a");

			const bool has_criteria = message.has_criteria();
			DBG_INF_FMT("criteria is %s", has_criteria? "SET":"NOT SET");
			if (has_criteria) {
				xmysqlnd_dump_expr(message.criteria());
			}

			DBG_INF_FMT("%d arguments", message.args_size());
			if (message.args_size()) {
				for (unsigned int i = 0; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]="MYSQLND_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]="MYSQLND_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (unsigned int i = 0; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");

					const bool has_direction = order.has_direction();
					DBG_INF_FMT("direction[%s]=[%s]", has_direction? "SET":"NOT SET",
													  has_direction? Mysqlx::Crud::Order::Direction_Name(order.direction()).c_str() : "n/a");
				}
			}

			DBG_INF_FMT("%d operations", message.operation_size());
			if (message.operation_size()) {
				for (unsigned int i = 0; i < message.operation_size(); ++i) {
					xmysqlnd_dump_update_operation(message.operation(i));
				}
			}
			break;
		}
		case Mysqlx::ClientMessages_Type_CRUD_DELETE:{
			Mysqlx::Crud::Delete message;
			message.ParseFromArray(payload, payload_size);

			const bool has_collection = message.has_collection();
			DBG_INF_FMT("collection %s", has_collection? "SET":"NOT SET");
			if (has_collection) {
				const Mysqlx::Crud::Collection & collection = message.collection();
				DBG_INF_FMT("[%s].[%s]", collection.has_schema()? collection.schema().c_str() :"n/a",
										 collection.has_name()? collection.name().c_str() :"n/a");
			}

			const bool has_data_model = message.has_data_model();
			DBG_INF_FMT("data_model[%s]=[%s]", has_data_model? "SET":"NOT SET",
											   has_data_model? Mysqlx::Crud::DataModel_Name(message.data_model()).c_str() : "n/a");

			const bool has_criteria = message.has_criteria();
			DBG_INF_FMT("criteria is %s", has_criteria? "SET":"NOT SET");
			if (has_criteria) {
				xmysqlnd_dump_expr(message.criteria());
			}

			DBG_INF_FMT("%d arguments", message.args_size());
			if (message.args_size()) {
				for (unsigned int i = 0; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]="MYSQLND_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]="MYSQLND_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (unsigned int i = 0; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");

					const bool has_direction = order.has_direction();
					DBG_INF_FMT("direction[%s]=[%s]", has_direction? "SET":"NOT SET",
													  has_direction? Mysqlx::Crud::Order::Direction_Name(order.direction()).c_str() : "n/a");
				}
			}
			break;
		}
		case Mysqlx::ClientMessages_Type_EXPECT_OPEN:{
			Mysqlx::Expect::Open message;
			message.ParseFromArray(payload, payload_size);
			break;
		}
		case Mysqlx::ClientMessages_Type_EXPECT_CLOSE: break; /* Empty */
		default:
			DBG_ERR_FMT("Unknown type %d", (int) packet_type);
			break;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_column_meta */
static void
xmysqlnd_dump_column_meta(const Mysqlx::Resultset::ColumnMetaData & meta)
{
	DBG_ENTER("xmysqlnd_dump_column_meta");

	const bool has_type = meta.has_type();
	DBG_INF_FMT("type [%s] is [%s]", has_type? "SET":"NOT SET",
									 has_type? Mysqlx::Resultset::ColumnMetaData::FieldType_Name(meta.type()).c_str() : "n/a");

	const bool has_name = meta.has_name();
	DBG_INF_FMT("name[%s] is [%s]", has_name? "SET":"NOT SET",
									has_name? meta.name().c_str() : "n/a");

	const bool has_orig_name = meta.has_original_name();
	DBG_INF_FMT("orig_name[%s] is [%s]", has_orig_name? "SET":"NOT SET",
										 has_orig_name? meta.original_name().c_str() : "n/a");

	const bool has_table = meta.has_table();
	DBG_INF_FMT("table[%s] is [%s]", has_table? "SET":"NOT SET",
								   has_table? meta.table().c_str() : "n/a");

	const bool has_orig_table = meta.has_original_table();
	DBG_INF_FMT("orig_table[%s] is [%s]", has_orig_table? "SET":"NOT SET",
										  has_orig_table? meta.original_table().c_str() : "n/a");

	const bool has_schema = meta.has_schema();
	DBG_INF_FMT("schema[%s] is [%s]", has_schema? "SET":"NOT SET",
									  has_schema? meta.schema().c_str() : "n/a");

	const bool has_catalog = meta.has_catalog();
	DBG_INF_FMT("catalog[%s] is [%s]", has_catalog? "SET":"NOT SET",
									   has_catalog? meta.catalog().c_str() : "n/a");

	const bool has_collation = meta.has_collation();
	DBG_INF_FMT("collation[%s] is ["MYSQLND_LLU_SPEC"]", has_collation? "SET":"NOT SET",
														 has_collation? meta.collation() : 0);

	const bool has_frac_digits = meta.has_fractional_digits();
	DBG_INF_FMT("frac_digits[%s] is [%u]", has_frac_digits? "SET":"NOT SET",
										   has_frac_digits? meta.fractional_digits() : 0);

	const bool has_length = meta.has_length();
	DBG_INF_FMT("length[%s] is [%u]", has_length? "SET":"NOT SET",
									  has_length? meta.length() : 0);

	const bool has_flags = meta.has_flags();
	DBG_INF_FMT("flags[%s] is [%u]", has_flags? "SET":"NOT SET",
									 has_flags? meta.flags() : 0);

	const bool has_content_type = meta.has_flags();
	DBG_INF_FMT("content_type[%s] is [%u]", has_content_type? "SET":"NOT SET",
											has_content_type? meta.content_type() : 0);

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ xmysqlnd_dump_warning */
static void
xmysqlnd_dump_warning(const Mysqlx::Notice::Warning & warning)
{
	DBG_ENTER("xmysqlnd_dump_warning");

	const bool has_level = warning.has_level();
	DBG_INF_FMT("level[%s] is %s", has_level? "SET":"NOT SET",
								   has_level? Mysqlx::Notice::Warning::Level_Name(warning.level()).c_str() : "n/a");

	const bool has_code = warning.has_code();
	DBG_INF_FMT("code[%s] is %u", has_code? "SET":"NOT SET",
								  has_code? warning.code() : 0);

	const bool has_msg = warning.has_msg();
	DBG_INF_FMT("messsage[%s] is %s", has_msg? "SET":"NOT SET",
									  has_msg? warning.msg().c_str() : "n/a");

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_changed_variable */
static void
xmysqlnd_dump_changed_variable(const Mysqlx::Notice::SessionVariableChanged & message)
{
	DBG_ENTER("xmysqlnd_dump_changed_variable");

	const bool has_param = message.has_param();
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? message.param().c_str() : "n/a");

	const bool has_value = message.has_value();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_value) {
		scalar2log(message.value());
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_changed_state */
static void
xmysqlnd_dump_changed_state(const Mysqlx::Notice::SessionStateChanged & message)
{
	DBG_ENTER("xmysqlnd_dump_changed_state");

	const bool has_param = message.has_param();
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? Mysqlx::Notice::SessionStateChanged::Parameter_Name(message.param()).c_str() : "n/a");


	const bool has_value = message.has_value();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_value) {
		scalar2log(message.value());
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_notice_frame */
static void
xmysqlnd_dump_notice_frame(const Mysqlx::Notice::Frame & frame)
{
	DBG_ENTER("xmysqlnd_dump_notice_frame");

	const bool has_scope = frame.has_scope();
	DBG_INF_FMT("scope[%s] is %s", has_scope? "SET":"NOT SET",
								   has_scope? Mysqlx::Notice::Frame::Scope_Name(frame.scope()).c_str() : "n/a");

	const bool has_payload = frame.has_payload();
	DBG_INF_FMT("payload is %s", has_payload? "SET":"NOT SET");

	const bool has_type = frame.has_type();
	
	DBG_INF_FMT("type is %s", has_type? "SET":"NOT SET");
	if (has_type && has_payload) {
		switch (frame.type()) {
			case 1:{ /* Warning */
				Mysqlx::Notice::Warning message;
				message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
				xmysqlnd_dump_warning(message);
				break;
			}
			case 2:{ /* SessionVariableChanged */
				Mysqlx::Notice::SessionVariableChanged message;
				message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
				xmysqlnd_dump_changed_variable(message);
				break;
			}
			case 3:{ /* SessionStateChanged */
				Mysqlx::Notice::SessionStateChanged message;
				message.ParseFromArray(frame.payload().c_str(), frame.payload().size());
				xmysqlnd_dump_changed_state(message);
				break;
			}
			default:
				DBG_ERR_FMT("Unknown type %d", frame.type());
				break;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_capabilities_to_log */
static void
xmysqlnd_dump_capabilities_to_log(const Mysqlx::Connection::Capabilities & message)
{
	DBG_ENTER("xmysqlnd_dump_capabilities_to_log");
	for (unsigned int i = 0; i < message.capabilities_size(); ++i) {
		DBG_INF_FMT("Cap_name[i]=%s", message.capabilities(i).name().c_str());
		any2log(message.capabilities(i).value());
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_dump_server_message */
extern "C" void
xmysqlnd_dump_server_message(const zend_uchar packet_type, const void * payload, const size_t payload_size)
{
	DBG_ENTER("xmysqlnd_dump_server_message");
	const Mysqlx::ServerMessages_Type type = (Mysqlx::ServerMessages_Type)(packet_type);
	DBG_INF_FMT("packet is %s   payload_size=%u", Mysqlx::ServerMessages_Type_Name(type).c_str(), (uint) payload_size);
	{
		static char hexconvtab[] = "0123456789abcdef";
		char * message_dump = new char[payload_size*3 + 1];
		message_dump[payload_size*3] = '\0';
		for (unsigned int i = 0; i < payload_size; i++) {
			message_dump[i*3+0] = hexconvtab[((const char*)payload)[i] >> 4];
			message_dump[i*3+1] = hexconvtab[((const char*)payload)[i] & 15];
			message_dump[i*3+2] = ' ';
		}
		DBG_INF_FMT("payload[%u]=[%s]", (uint) payload_size, message_dump);	
		delete [] message_dump;
	}
	switch (type) {
		case Mysqlx::ServerMessages_Type_OK:{
			Mysqlx::Ok message;
			message.ParseFromArray(payload, payload_size);
			const bool has_msg = message.has_msg();

			DBG_INF_FMT("message[%s]=%s ", has_msg? "SET":"NOT SET", has_msg? message.msg().c_str() : "n/a");
			break;
		}
		case Mysqlx::ServerMessages_Type_ERROR:{
			Mysqlx::Error message;
			message.ParseFromArray(payload, payload_size);

			const char * error_severity = "Uknown Severity";
			uint32_t code = 0;
			const char * sql_state = "00000";
			const char * error_message = "";
			if (message.has_severity()) {
				if (message.severity() == Mysqlx::Error_Severity_ERROR) {
					error_severity = "ERROR";
				} else if (message.severity() == Mysqlx::Error_Severity_FATAL) {
					error_severity = "FATAL";
				}
			}
			if (message.has_code()) {
				code = message.code();
			}
			if (message.has_sql_state()) {
				sql_state = message.sql_state().c_str();
			}
			if (message.has_msg()) {
				error_message = message.msg().c_str();
			}
			DBG_INF_FMT("severity=%s", error_severity);
			DBG_INF_FMT("error   =%u", code);
			DBG_INF_FMT("sqlstate=%s", sql_state);
			DBG_INF_FMT("message =%s", error_message);
			break;
		}
		case Mysqlx::ServerMessages_Type_CONN_CAPABILITIES:{
			Mysqlx::Connection::Capabilities message;
			message.ParseFromArray(payload, payload_size);
			xmysqlnd_dump_capabilities_to_log(message);
			break;
		}
		case Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_CONTINUE:{
			Mysqlx::Session::AuthenticateContinue message;
			message.ParseFromArray(payload, payload_size);
			const bool has_auth_data = message.has_auth_data();
			DBG_INF_FMT("message[%s][len=%d]=%s ", has_auth_data? "SET":"NOT SET", has_auth_data? message.auth_data().size():0, has_auth_data? message.auth_data().c_str() : "n/a");			
			break;
		}
		case Mysqlx::ServerMessages_Type_SESS_AUTHENTICATE_OK:{
			Mysqlx::Session::AuthenticateOk message;
			message.ParseFromArray(payload, payload_size);
			const bool has_auth_data = message.has_auth_data();
			DBG_INF_FMT("auth_data[%s][len=%d]=%s ", has_auth_data? "SET":"NOT SET", has_auth_data? message.auth_data().size():0, has_auth_data? message.auth_data().c_str() : "n/a");			
			break;
		}
		case Mysqlx::ServerMessages_Type_NOTICE:{
			/* payload of the Frame is a Mysqlx::Notice::Warning */
			Mysqlx::Notice::Frame message;
			message.ParseFromArray(payload, payload_size);
			xmysqlnd_dump_notice_frame(message);
			break;
		}
		case Mysqlx::ServerMessages_Type_RESULTSET_COLUMN_META_DATA:{
			Mysqlx::Resultset::ColumnMetaData message;
			message.ParseFromArray(payload, payload_size);
			xmysqlnd_dump_column_meta(message);
			break;
		}
		case Mysqlx::ServerMessages_Type_RESULTSET_ROW: break; /* NEEDS Metadata for decoding */
		case Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE: break; /* Empty */
		case Mysqlx::ServerMessages_Type_RESULTSET_FETCH_SUSPENDED:
			// WHAAAAT ??
			break;
		case Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_RESULTSETS: break; /* Empty */
		case Mysqlx::ServerMessages_Type_SQL_STMT_EXECUTE_OK: break; /* Empty */
		case Mysqlx::ServerMessages_Type_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS: break; /* Empty */
		default:
			DBG_ERR_FMT("Unknown type %d", (int) packet_type);
			break;
	}
	DBG_VOID_RETURN;
}
/* }}} */

#include "mysqlx_node_connection.h"
#include "mysqlx_node_pfc.h"

/* {{{ xmysqlnd_send_protobuf_message */
static size_t
xmysqlnd_send_protobuf_message(struct st_mysqlx_node_connection * connection, struct st_mysqlx_node_pfc * codec,
							   enum xmysqlnd_client_message_type packet_type, ::google::protobuf::Message & message)
{
	size_t ret;
	DBG_ENTER("xmysqlnd_send_protobuf_message");

	const size_t payload_size = message.ByteSize();
	size_t bytes_sent;
	void * payload = payload_size? mnd_emalloc(payload_size) : NULL;
	if (payload_size && !payload) {
		php_error_docref(NULL, E_WARNING, "Memory allocation problem");
		
		DBG_RETURN(0);
	}
	message.SerializeToArray(payload, payload_size);
	ret = codec->pfc->data->m.send(codec->pfc, connection->vio,
								   packet_type,
								   (zend_uchar *) payload, payload_size,
								   &bytes_sent,
								   connection->stats,
								   connection->error_info);
	mnd_efree(payload);
	return bytes_sent;
}
/* }}} */

#define SIZE_OF_STACK_BUFFER 200

/* {{{ xmysqlnd_send_message */
static enum_func_status
xmysqlnd_send_message(enum xmysqlnd_client_message_type packet_type, ::google::protobuf::Message & message,
					  MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info,
					  size_t * bytes_sent)
{
	enum_func_status ret;
	DBG_ENTER("xmysqlnd_send_protobuf_message");
#ifdef PHP_DEBUG
	if (!xmysqlnd_client_message_type_is_valid(packet_type)) {
		SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The client wants to send invalid packet type");
		DBG_ERR_FMT("The client wants to send invalid packet type %d", (int) packet_type);
		DBG_RETURN(FAIL);	
	}
#endif
	char stack_buffer[SIZE_OF_STACK_BUFFER];
	void * payload = stack_buffer;

	const size_t payload_size = message.ByteSize();
	if (payload_size > sizeof(stack_buffer)) {
		payload = payload_size? mnd_emalloc(payload_size) : NULL;
		if (payload_size && !payload) {
			php_error_docref(NULL, E_WARNING, "Memory allocation problem");
			SET_OOM_ERROR(error_info);
			DBG_RETURN(FAIL);
		}
	}

	message.SerializeToArray(payload, payload_size);
	ret = pfc->data->m.send(pfc, vio, packet_type, (zend_uchar *) payload, payload_size, bytes_sent, stats, error_info);
	if (payload != stack_buffer) {
		mnd_efree(payload);
	}
	DBG_RETURN(ret);
}
/* }}} */


struct st_xmysqlnd_server_messages_handlers
{
	enum_hnd_func_status (*on_OK)(const Mysqlx::Ok & message, void * context);
	enum_hnd_func_status (*on_ERROR)(const Mysqlx::Error & message, void * context);
	enum_hnd_func_status (*on_CAPABILITIES)(const Mysqlx::Connection::Capabilities & message, void * context);
	enum_hnd_func_status (*on_AUTHENTICATE_CONTINUE)(const Mysqlx::Session::AuthenticateContinue & message, void * context);
	enum_hnd_func_status (*on_AUTHENTICATE_OK)(const Mysqlx::Session::AuthenticateOk & message, void * context);
	enum_hnd_func_status (*on_NOTICE)(const Mysqlx::Notice::Frame & message, void * context);
	enum_hnd_func_status (*on_COLUMN_META)(const Mysqlx::Resultset::ColumnMetaData & message, void * context);
	enum_hnd_func_status (*on_RSET_ROW)(const Mysqlx::Resultset::Row & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_DONE)(const Mysqlx::Resultset::FetchDone & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_SUSPENDED)(void * context); /*  there is no Mysqlx::Resultset::FetchSuspended*/
	enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_RSETS)(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context);
	enum_hnd_func_status (*on_STMT_EXECUTE_OK)(const Mysqlx::Sql::StmtExecuteOk & message, void * context);
	enum_hnd_func_status (*on_RSET_FETCH_DONE_MORE_OUT_PARAMS)(const Mysqlx::Resultset::FetchDoneMoreOutParams & message, void * context);
	enum_hnd_func_status (*on_UNEXPECTED)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
	enum_hnd_func_status (*on_UNKNOWN)(const zend_uchar packet_type, const zend_uchar * const payload, const size_t payload_size, void * context);
};


/* {{{ xmysqlnd_receive_message */
enum_func_status
xmysqlnd_receive_message(struct st_xmysqlnd_server_messages_handlers * handlers, void * handler_ctx,
						 MYSQLND_VIO * vio, XMYSQLND_PFC * pfc, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	zend_uchar stack_buffer[SIZE_OF_STACK_BUFFER];
	enum_func_status ret = FAIL;
	enum_hnd_func_status hnd_ret;
	size_t payload_size;
	zend_uchar * payload;
	zend_uchar type;

	DBG_ENTER("xmysqlnd_receive_message");

	do {
		ret = pfc->data->m.receive(pfc, vio, stack_buffer, sizeof(stack_buffer), &type, &payload, &payload_size, stats, error_info);
		if (FAIL == ret) {
			DBG_RETURN(FAIL);
		}
		if (!xmysqlnd_server_message_type_is_valid(type)) {
			SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "The server sent invalid packet type");
			DBG_ERR_FMT("Invalid packet type %u from the server", (uint) type);
			DBG_RETURN(FAIL);	
		}
		enum xmysqlnd_server_message_type packet_type = (enum xmysqlnd_server_message_type) type;
		hnd_ret = HND_PASS;
		bool handled = false;
		switch (packet_type) {
			case XMSG_OK:
				if (handlers->on_OK) {
					Mysqlx::Ok message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_OK(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_ERROR:
				if (handlers->on_ERROR) {
					Mysqlx::Error message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_ERROR(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_CAPABILITIES:
				if (handlers->on_CAPABILITIES) {
					Mysqlx::Connection::Capabilities message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_CAPABILITIES(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_AUTH_CONTINUE:
				if (handlers->on_AUTHENTICATE_CONTINUE) {
					Mysqlx::Session::AuthenticateContinue message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_AUTHENTICATE_CONTINUE(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_AUTH_OK:
				if (handlers->on_AUTHENTICATE_OK) {
					Mysqlx::Session::AuthenticateOk message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_AUTHENTICATE_OK(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_NOTICE:
				if (handlers->on_NOTICE) {
					Mysqlx::Notice::Frame message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_NOTICE(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_COLUMN_METADATA:
				if (handlers->on_COLUMN_META) {
					Mysqlx::Resultset::ColumnMetaData message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_COLUMN_META(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_RSET_ROW:
				if (handlers->on_RSET_ROW) {
					Mysqlx::Resultset::Row message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_RSET_ROW(message, handler_ctx);
					handled = true;
				}
				break;
			case XMSG_RSET_FETCH_DONE:
				if (handlers->on_RSET_FETCH_DONE) {
					Mysqlx::Resultset::FetchDone message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_RSET_FETCH_DONE(message, handler_ctx);
					handled = true;
				}
				break;
			case XMGS_RSET_FETCH_SUSPENDED:
				if (handlers->on_RSET_FETCH_SUSPENDED) {
					hnd_ret = handlers->on_RSET_FETCH_SUSPENDED(handler_ctx);
					handled = true;
				}
				break;
			case XMSG_RSET_FETCH_DONE_MORE_RSETS:
				if (handlers->on_RSET_FETCH_DONE_MORE_RSETS) {
					Mysqlx::Resultset::FetchDoneMoreResultsets message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_RSET_FETCH_DONE_MORE_RSETS(message, handler_ctx);
					handled = true;
				}
				break;
			case XMGS_STMT_EXECUTE_OK:
				if (handlers->on_STMT_EXECUTE_OK) {
					Mysqlx::Sql::StmtExecuteOk message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_STMT_EXECUTE_OK(message, handler_ctx);
					handled = true;
				}
				break;
			case XMGS_RSET_FETCH_DONE_MORE_OUT:
				if (handlers->on_RSET_FETCH_DONE_MORE_OUT_PARAMS) {
					Mysqlx::Resultset::FetchDoneMoreOutParams message;
					message.ParseFromArray(payload, payload_size);
					hnd_ret = handlers->on_RSET_FETCH_DONE_MORE_OUT_PARAMS(message, handler_ctx);
					handled = true;
				}
				break;
			default:
				handled = true;
				if (handlers->on_UNKNOWN) {
					hnd_ret = handlers->on_UNKNOWN(type, payload, payload_size, handler_ctx);
				}
				SET_CLIENT_ERROR(error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Unknown type");
				DBG_ERR_FMT("Unknown type %d", (int) packet_type);
				break;
		}
		if (!handled) {
			DBG_INF_FMT("Unhandled message %d", packet_type);
			if (handlers->on_UNEXPECTED) {
				hnd_ret = handlers->on_UNEXPECTED(type, payload, payload_size, handler_ctx);
			}
		}
		if (payload != stack_buffer) {
			mnd_efree(payload);
		}
		if (hnd_ret == HND_AGAIN) {
			DBG_INF("HND_AGAIN. Reading new packet from the network");
		}
	} while (hnd_ret == HND_AGAIN);
	ret = (HND_PASS || HND_AGAIN_ASYNC)? PASS:FAIL;
	DBG_RETURN(ret);
}
/* }}} */


/************************************** CAPABILITIES GET **************************************************/

/* {{{ xmysqlnd_send__capabilities_get */
extern "C" enum_func_status
xmysqlnd_send__capabilities_get(SEND_READ_CTX_DEF)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesGet message;
	return xmysqlnd_send_message(COM_CAPABILITIES_GET, message, vio, pfc, stats, error_info, &bytes_sent);
}
/* }}} */


/* {{{ proto capabilities_to_zv */
static void
capabilities_to_zval(const Mysqlx::Connection::Capabilities & message, zval * return_value)
{
	DBG_ENTER("capabilities_to_zv");
	array_init_size(return_value, message.capabilities_size());
	for (unsigned int i = 0; i < message.capabilities_size(); ++i) {
		zval zv = {0};
		any2zval(message.capabilities(i).value(), &zv);
		if (Z_REFCOUNTED(zv)) {
			Z_ADDREF(zv);
		}
		add_assoc_zval_ex(return_value, message.capabilities(i).name().c_str(), message.capabilities(i).name().size(), &zv);
		zval_ptr_dtor(&zv);
	}
	DBG_VOID_RETURN;
}
/* }}} */


struct st_capabilities_get_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * capabilities;
};


/* {{{ capabilities_get_on_ERROR */
static enum_hnd_func_status
capabilities_get_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_capabilities_get_ctx * ctx = static_cast<struct st_capabilities_get_ctx *>(context);
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ capabilities_get_on_CAPABILITIES */
static enum_hnd_func_status
capabilities_get_on_CAPABILITIES(const Mysqlx::Connection::Capabilities & message, void * context)
{
	struct st_capabilities_get_ctx * ctx = static_cast<struct st_capabilities_get_ctx *>(context);
	capabilities_to_zval(message, ctx->capabilities);
	return HND_PASS;
}
/* }}} */


/* {{{ capabilities_get_on_NOTICE */
static enum_hnd_func_status
capabilities_get_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers capabilities_get_handlers =
{
	NULL, 							// on_OK
	capabilities_get_on_ERROR,		// on_ERROR
	capabilities_get_on_CAPABILITIES,// on_CAPABILITIES
	NULL,							// on_AUTHENTICATE_CONTINUE
	NULL,							// on_AUTHENTICATE_OK
	capabilities_get_on_NOTICE,		// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};


/* {{{ xmysqlnd_read__capabilities_get */
extern "C" enum_func_status
xmysqlnd_read__capabilities_get(zval * capabilities, SEND_READ_CTX_DEF)
{
	enum_func_status ret;
	struct st_capabilities_get_ctx ctx = { stats, error_info, capabilities };
	DBG_ENTER("xmysqlnd_read__capabilities_get");
	ret = xmysqlnd_receive_message(&capabilities_get_handlers, &ctx, vio, pfc, stats, error_info);
	DBG_RETURN(ret);
}
/* }}} */


/************************************** CAPABILITIES SET **************************************************/

/* {{{ xmysqlnd_send__capabilities_set */
extern "C" enum_func_status
xmysqlnd_send__capabilities_set(const size_t cap_count, zval ** capabilities_names, zval ** capabilities_values, SEND_READ_CTX_DEF)
{
	size_t bytes_sent;
	Mysqlx::Connection::CapabilitiesSet message;
	for (unsigned i = 0; i < cap_count; ++i) {
		Mysqlx::Connection::Capability * capability = message.mutable_capabilities()->add_capabilities();
		capability->set_name(Z_STRVAL_P(capabilities_names[i]), Z_STRLEN_P(capabilities_names[i]));
		Mysqlx::Datatypes::Any any_entry;
		zval2any(capabilities_values[i], any_entry);
		capability->mutable_value()->CopyFrom(any_entry);
	}

	return xmysqlnd_send_message(COM_CAPABILITIES_SET, message, vio, pfc, stats, error_info, &bytes_sent);
}
/* }}} */

struct st_capabilities_set_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * return_value;
};

#include "messages/mysqlx_message__ok.h"

/* {{{ capabilities_set_on_OK */
static enum_hnd_func_status
capabilities_set_on_OK(const Mysqlx::Ok & message, void * context)
{
	struct st_capabilities_set_ctx * ctx = static_cast<struct st_capabilities_set_ctx *>(context);
	mysqlx_new_message__ok(ctx->return_value, message);
}
/* }}} */


/* {{{ capabilities_set_on_ERROR */
static enum_hnd_func_status
capabilities_set_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_capabilities_set_ctx * ctx = static_cast<struct st_capabilities_set_ctx *>(context);
	DBG_ENTER("capabilities_set_on_ERROR");
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	DBG_RETURN(HND_PASS_RETURN_FAIL);
}
/* }}} */


/* {{{ capabilities_on_NOTICE */
static enum_hnd_func_status
capabilities_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers capabilities_set_handlers =
{
	capabilities_set_on_OK, 		// on_OK
	capabilities_set_on_ERROR,		// on_ERROR
	NULL,							// on_CAPABILITIES
	NULL,							// on_AUTHENTICATE_CONTINUE
	NULL,							// on_AUTHENTICATE_OK
	capabilities_on_NOTICE,
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_read__capabilities_set */
extern "C" enum_func_status
xmysqlnd_read__capabilities_set(zval * return_value, SEND_READ_CTX_DEF)
{
	enum_func_status ret;
	struct st_capabilities_set_ctx ctx = { stats, error_info, return_value};
	DBG_ENTER("xmysqlnd_read__capabilities_set");
	ret = xmysqlnd_receive_message(&capabilities_set_handlers, &ctx, vio, pfc, stats, error_info);
	DBG_RETURN(ret);
}
/* }}} */

/************************************** AUTH_START **************************************************/

extern "C" enum_func_status
xmysqlnd_send__authentication_start(const MYSQLND_CSTRING auth_mech_name, const MYSQLND_CSTRING auth_data, SEND_READ_CTX_DEF)
{
	size_t bytes_sent;
	Mysqlx::Session::AuthenticateStart message;
	message.set_mech_name(auth_mech_name.s, auth_mech_name.l);
	message.set_auth_data(auth_data.s, auth_data.l);
	return xmysqlnd_send_message(COM_AUTH_START, message, SEND_READ_CTX_PASSTHRU, &bytes_sent);
}


struct st_auth_start_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * auth_start_response;
};


/* {{{ auth_start_on_ERROR */
static enum_hnd_func_status
auth_start_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */

/* {{{ auth_start_on_NOTICE */
static enum_hnd_func_status
auth_start_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */

#include "messages/mysqlx_message__auth_continue.h"
#include "messages/mysqlx_message__auth_ok.h"

/* {{{ auth_start_on_AUTHENTICATE_CONTINUE */
static enum_hnd_func_status
auth_start_on_AUTHENTICATE_CONTINUE(const Mysqlx::Session::AuthenticateContinue & message, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_CONTINUE");
	mysqlx_new_message__auth_continue(ctx->auth_start_response, message);
	DBG_RETURN(HND_PASS);
}
/* }}} */


/* {{{ auth_start_on_AUTHENTICATE_OK */
static enum_hnd_func_status
auth_start_on_AUTHENTICATE_OK(const Mysqlx::Session::AuthenticateOk & message, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	DBG_ENTER("auth_start_on_AUTHENTICATE_OK");
	mysqlx_new_message__auth_ok(ctx->auth_start_response, message);
	DBG_RETURN(HND_PASS);
}
/* }}} */



static struct st_xmysqlnd_server_messages_handlers auth_start_handlers =
{
	NULL, 							// on_OK
	auth_start_on_ERROR,			// on_ERROR
	NULL,							// on_CAPABILITIES
	auth_start_on_AUTHENTICATE_CONTINUE,// on_AUTHENTICATE_CONTINUE
	auth_start_on_AUTHENTICATE_OK,	// on_AUTHENTICATE_OK
	auth_start_on_NOTICE,			// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_read__authentication_start */
extern "C" enum_func_status
xmysqlnd_read__authentication_start(zval * auth_start_response, SEND_READ_CTX_DEF)
{
	enum_func_status ret;
	struct st_auth_start_ctx ctx = { stats, error_info, auth_start_response };
	DBG_ENTER("xmysqlnd_read__authentication_start");
	ret = xmysqlnd_receive_message(&auth_start_handlers, &ctx, vio, pfc, stats, error_info);
	DBG_RETURN(ret);
}
/* }}} */


/************************************** AUTH_CONTINUE **************************************************/
extern "C"
{
#include "ext/mysqlnd/mysqlnd_auth.h" /* php_mysqlnd_scramble */
}

static const char hexconvtab[] = "0123456789abcdef";

/* {{{ xmysqlnd_send__authentication_continue */
extern "C" enum_func_status
xmysqlnd_send__authentication_continue(const MYSQLND_CSTRING schema,
									   const MYSQLND_CSTRING user,
									   const MYSQLND_CSTRING password,
									   const MYSQLND_CSTRING salt,
									   SEND_READ_CTX_DEF)
{
	size_t bytes_sent;
	Mysqlx::Session::AuthenticateContinue message;
	DBG_ENTER("xmysqlnd_send__authentication_continue");

	std::string response(schema.s, schema.l);
	response.append(1, '\0');
	response.append(user.s, user.l);
	response.append(1, '\0'); 
	if (password.s && password.l) {
		zend_uchar hash[SCRAMBLE_LENGTH];

		php_mysqlnd_scramble(hash, (zend_uchar*) salt.s, (const zend_uchar*) password.s, password.l);
		char hexed_hash[SCRAMBLE_LENGTH*2];
		for (unsigned int i = 0; i < SCRAMBLE_LENGTH; i++) {
			hexed_hash[i*2] = hexconvtab[hash[i] >> 4];
			hexed_hash[i*2 + 1] = hexconvtab[hash[i] & 15];
		}
		DBG_INF_FMT("hexed_hash=%s", hexed_hash);
		response.append(1, '*');
		response.append(hexed_hash, SCRAMBLE_LENGTH*2);
	}
	response.append(1, '\0');
	DBG_INF_FMT("response_size=%u", (uint) response.size());
	message.set_auth_data(response.c_str(), response.size());

	return xmysqlnd_send_message(COM_AUTH_CONTINUE, message, SEND_READ_CTX_PASSTHRU, &bytes_sent);
}
/* }}} */


struct st_auth_continue_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * auth_continue_response;
};


/* {{{ auth_continue_on_ERROR */
static enum_hnd_func_status
auth_continue_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ auth_continue_on_NOTICE */
static enum_hnd_func_status
auth_continue_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */

#include "messages/mysqlx_message__auth_continue.h"
#include "messages/mysqlx_message__auth_ok.h"

/* {{{ auth_continue_on_AUTHENTICATE_CONTINUE */
static enum_hnd_func_status
auth_continue_on_AUTHENTICATE_CONTINUE(const Mysqlx::Session::AuthenticateContinue & message, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	DBG_ENTER("auth_continue_on_AUTHENTICATE_CONTINUE");
	mysqlx_new_message__auth_continue(ctx->auth_start_response, message);
	DBG_RETURN(HND_PASS);
}
/* }}} */


/* {{{ auth_continue_on_AUTHENTICATE_OK */
static enum_hnd_func_status
auth_continue_on_AUTHENTICATE_OK(const Mysqlx::Session::AuthenticateOk & message, void * context)
{
	struct st_auth_start_ctx * ctx = static_cast<struct st_auth_start_ctx *>(context);
	DBG_ENTER("auth_continue_on_AUTHENTICATE_OK");
	mysqlx_new_message__auth_ok(ctx->auth_start_response, message);
	DBG_RETURN(HND_PASS);
}
/* }}} */



static struct st_xmysqlnd_server_messages_handlers auth_continue_handlers =
{
	NULL, 							// on_OK
	auth_continue_on_ERROR,			// on_ERROR
	NULL,							// on_CAPABILITIES
	auth_continue_on_AUTHENTICATE_CONTINUE,	// on_AUTHENTICATE_CONTINUE
	auth_continue_on_AUTHENTICATE_OK,		// on_AUTHENTICATE_OK
	auth_continue_on_NOTICE,		// on_NOTICE
	NULL,							// on_RSET_COLUMN_META
	NULL,							// on_RSET_ROW
	NULL,							// on_RSET_FETCH_DONE
	NULL,							// on_RESULTSET_FETCH_SUSPENDED
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	NULL,							// on_SQL_STMT_EXECUTE_OK
	NULL,							// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,							// on_UNEXPECTED
	NULL,							// on_UNKNOWN
};

/* {{{ xmysqlnd_read__authentication_continue */
extern "C" enum_func_status
xmysqlnd_read__authentication_continue(zval * auth_continue_response, SEND_READ_CTX_DEF)
{
	enum_func_status ret;
	struct st_auth_continue_ctx ctx = { stats, error_info, auth_continue_response };
	DBG_ENTER("xmysqlnd_read__authentication_continue");
	ret = xmysqlnd_receive_message(&auth_start_handlers, &ctx, vio, pfc, stats, error_info);
	DBG_RETURN(ret);
}
/* }}} */


/**************************************  STMT_EXECUTE **************************************************/

/* {{{ xmysqlnd_send__sql_stmt_execute */
extern "C" enum_func_status
xmysqlnd_send__sql_stmt_execute(const MYSQLND_CSTRING namespace_, const MYSQLND_CSTRING stmt, const zend_bool compact_meta, SEND_READ_CTX_DEF)
{
	size_t bytes_sent;
	Mysqlx::Sql::StmtExecute message;

	message.set_namespace_(namespace_.s, namespace_.l);
	message.set_stmt(stmt.s, stmt.l);
	message.set_compact_metadata(compact_meta? true:false);
	return xmysqlnd_send_message(COM_SQL_STMT_EXECUTE, message, SEND_READ_CTX_PASSTHRU, &bytes_sent);
}
/* }}} */


struct st_sql_stmt_execute_ctx
{
	MYSQLND_STATS * stats;
	MYSQLND_ERROR_INFO * error_info;
	zval * response;
};

#include "messages/mysqlx_message__ok.h"


/* {{{ stmt_execute_on_ERROR */
static enum_hnd_func_status
stmt_execute_on_ERROR(const Mysqlx::Error & error, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	SET_CLIENT_ERROR(ctx->error_info,
					 error.has_code()? error.code() : CR_UNKNOWN_ERROR,
					 error.has_sql_state()? error.sql_state().c_str() : UNKNOWN_SQLSTATE,
					 error.has_msg()? error.msg().c_str() : "Unknown server error");
	return HND_PASS_RETURN_FAIL;
}
/* }}} */


/* {{{ auth_start_on_NOTICE */
static enum_hnd_func_status
stmt_execute_on_NOTICE(const Mysqlx::Notice::Frame & message, void * context)
{
	return HND_AGAIN;
}
/* }}} */


#include "mysqlx_resultset__column_metadata.h"

/* {{{ stmt_execute_on_COLUMN_META */
static enum_hnd_func_status
stmt_execute_on_COLUMN_META(const Mysqlx::Resultset::ColumnMetaData & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	mysqlx_new_column_metadata(ctx->response, message);
	return HND_PASS; /* typically this should be HND_AGAIN */
}
/* }}} */

#include "mysqlx_resultset__data_row.h"

/* {{{ stmt_execute_on_RSET_ROW */
static enum_hnd_func_status
stmt_execute_on_RSET_ROW(const Mysqlx::Resultset::Row & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	mysqlx_new_data_row(ctx->response, message);
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE(const Mysqlx::Resultset::FetchDone & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_SUSPENDED */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_SUSPENDED(void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS(const Mysqlx::Resultset::FetchDoneMoreResultsets & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	return HND_PASS;
}
/* }}} */

#include "messages/mysqlx_message__stmt_execute_ok.h"

/* {{{ stmt_execute_on_STMT_EXECUTE_OK */
static enum_hnd_func_status
stmt_execute_on_STMT_EXECUTE_OK(const Mysqlx::Sql::StmtExecuteOk & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	mysqlx_new_stmt_execute_ok(ctx->response, message);
	return HND_PASS;
}
/* }}} */


/* {{{ stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS */
static enum_hnd_func_status
stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS(const Mysqlx::Resultset::FetchDoneMoreOutParams & message, void * context)
{
	struct st_sql_stmt_execute_ctx * ctx = static_cast<struct st_sql_stmt_execute_ctx *>(context);
	return HND_PASS;
}
/* }}} */


static struct st_xmysqlnd_server_messages_handlers stmt_execute_handlers =
{
	NULL,				 				// on_OK
	stmt_execute_on_ERROR,				// on_ERROR
	NULL,								// on_CAPABILITIES
	NULL,								// on_AUTHENTICATE_CONTINUE
	NULL,								// on_AUTHENTICATE_OK
	stmt_execute_on_NOTICE,				// on_NOTICE
	stmt_execute_on_COLUMN_META,		// on_RSET_COLUMN_META
	stmt_execute_on_RSET_ROW,			// on_RSET_ROW
	stmt_execute_on_RSET_FETCH_DONE,	// on_RSET_FETCH_DONE
	stmt_execute_on_RSET_FETCH_SUSPENDED,			// on_RESULTSET_FETCH_SUSPENDED
	stmt_execute_on_RSET_FETCH_DONE_MORE_RSETS,		// on_RESULTSET_FETCH_DONE_MORE_RESULTSETS
	stmt_execute_on_STMT_EXECUTE_OK,				// on_SQL_STMT_EXECUTE_OK
	stmt_execute_on_RSET_FETCH_DONE_MORE_OUT_PARAMS,// on_RESULTSET_FETCH_DONE_MORE_OUT_PARAMS)
	NULL,								// on_UNEXPECTED
	NULL,								// on_UNKNOWN
};

/* {{{ xmysqlnd_read__stmt_execute */
extern "C" enum_func_status
xmysqlnd_read__stmt_execute(zval * response, SEND_READ_CTX_DEF)
{
	enum_func_status ret;
	struct st_sql_stmt_execute_ctx ctx = { stats, error_info, response };
	DBG_ENTER("xmysqlnd_read__stmt_execute");
	ret = xmysqlnd_receive_message(&stmt_execute_handlers, &ctx, vio, pfc, stats, error_info);
	DBG_RETURN(ret);
}
/* }}} */


/*
 * Local variables:{
 * tab-width: 4
 * c-basic-offset: 4
 * End:{
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
