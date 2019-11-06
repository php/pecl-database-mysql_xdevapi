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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_zval2any.h"
#include "xmysqlnd_protocol_dumper.h"
#include "util/strings.h"

#include "proto_gen/mysqlx.pb.h"
#include "proto_gen/mysqlx_connection.pb.h"
#include "proto_gen/mysqlx_crud.pb.h"
#include "proto_gen/mysqlx_expect.pb.h"
#include "proto_gen/mysqlx_expr.pb.h"
#include "proto_gen/mysqlx_notice.pb.h"
#include "proto_gen/mysqlx_resultset.pb.h"
#include "proto_gen/mysqlx_session.pb.h"
#include "proto_gen/mysqlx_sql.pb.h"

namespace mysqlx {

namespace drv {

static char hexconvtab[] = "0123456789abcdef";

void
xmysqlnd_dump_string_to_log(const char * prefix, const char * s, const size_t len)
{
	util::string message_dump(len * 3, '\0');
	DBG_ENTER("dump_string_to_log");
	for (unsigned int i{0}; i < len; ++i) {
		message_dump[i*3+0] = hexconvtab[ s[i] >> 4];
		message_dump[i*3+1] = hexconvtab[ s[i] & 15];
		message_dump[i*3+2] = ' ';
	}
	DBG_INF_FMT("%s[%u]=[%*s]", prefix, static_cast<unsigned int>(len), len, message_dump.c_str());
	DBG_VOID_RETURN;
}

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

MYSQLX_SUPPRESS_MSVC_WARNINGS(4505)
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
	for (int i{0}; i < column.document_path_size(); ++i) {
		xmysqlnd_dump_expr_doc_path_item(column.document_path(i));
	}
	DBG_VOID_RETURN;
}
MYSQLX_RESTORE_WARNINGS()

static void xmysqlnd_dump_expr(const Mysqlx::Expr::Expr & expr);

static void
xmysqlnd_dump_function_call(const Mysqlx::Expr::FunctionCall & fc)
{
	DBG_ENTER("xmysqlnd_dump_function_call");

	const bool has_name = fc.has_name();
	DBG_INF_FMT("fc has name %s", has_name? "SET":"NOT SET");
	if (has_name) {
		const bool has_ident_name = fc.name().has_name();
		DBG_INF_FMT("identifier::name[%s] is %s", has_ident_name? "SET":"NOT SET",
												  has_ident_name? fc.name().name().c_str() : "n/a");

		const bool has_ident_schema = fc.name().has_schema_name();
		DBG_INF_FMT("identifier::schema[%s] is %s", has_ident_schema? "SET":"NOT SET",
													has_ident_schema? fc.name().schema_name().c_str() : "n/a");

	}

	DBG_INF_FMT("%d fc::params", fc.param_size());
	for (int i{0}; i < fc.param_size(); ++i) {
		xmysqlnd_dump_expr(fc.param(i));
	}

	DBG_VOID_RETURN;
}

static void
xmysqlnd_dump_operator(const Mysqlx::Expr::Operator & op)
{
	DBG_ENTER("xmysqlnd_dump_operator");

	const bool has_name = op.has_name();
	DBG_INF_FMT("name[%s] is %s", has_name? "SET":"NOT SET",
								  has_name? op.name().c_str() : "n/a");

	DBG_INF_FMT("%d params", op.param_size());
	for (int i{0}; i < op.param_size(); ++i) {
		xmysqlnd_dump_expr(op.param(i));
	}
	DBG_VOID_RETURN;
}

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
		for (int i{0}; i < expr.object().fld_size(); ++i) {
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
		for (int i{0}; i < expr.array().value_size(); ++i) {
			xmysqlnd_dump_expr(expr.array().value(i));
		}
	}

	DBG_VOID_RETURN;
}

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

void
xmysqlnd_dump_client_message(const zend_uchar packet_type, const void * payload, const int payload_size)
{
	DBG_ENTER("xmysqlnd_dump_client_message");
	const Mysqlx::ClientMessages_Type type = (Mysqlx::ClientMessages_Type)(packet_type);
	DBG_INF_FMT("packet is %s   payload_size=%u", Mysqlx::ClientMessages_Type_Name(type).c_str(), static_cast<unsigned int>(payload_size));
	{
		char * message_dump = new char[payload_size*3 + 1];
		message_dump[payload_size*3] = '\0';
		for (int i{0}; i < payload_size; i++) {
			message_dump[i*3+0] = hexconvtab[((const char*)payload)[i] >> 4];
			message_dump[i*3+1] = hexconvtab[((const char*)payload)[i] & 15];
			message_dump[i*3+2] = ' ';
		}
		DBG_INF_FMT("payload[%u]=[%*s]", static_cast<unsigned int>(payload_size), payload_size, message_dump);
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
				for (unsigned int i{0}; i < caps; ++i) {
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
				for (int i{0}; i < message.args_size(); ++i) {
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
				for (int i{0}; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]=" MYSQLX_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]=" MYSQLX_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (int i{0}; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");
					if (order.has_expr()) {
						xmysqlnd_dump_expr(order.expr());
					}

					const bool has_direction = order.has_direction();
					DBG_INF_FMT("direction[%s]=[%s]", has_direction? "SET":"NOT SET",
													  has_direction? Mysqlx::Crud::Order::Direction_Name(order.direction()).c_str() : "n/a");
				}
			}

			DBG_INF_FMT("grouping_size=%d", message.grouping_size());
			if (message.grouping_size()) {
				for (int i{0}; i < message.grouping_size(); ++i) {
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
				for (int i{0}; i < message.args_size(); ++i) {
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
				for (int i{0}; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]=" MYSQLX_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]=" MYSQLX_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (int i{0}; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");
					if (order.has_expr()) {
						xmysqlnd_dump_expr(order.expr());
					}

					const bool has_direction = order.has_direction();
					DBG_INF_FMT("direction[%s]=[%s]", has_direction? "SET":"NOT SET",
													  has_direction? Mysqlx::Crud::Order::Direction_Name(order.direction()).c_str() : "n/a");
				}
			}

			DBG_INF_FMT("%d operations", message.operation_size());
			if (message.operation_size()) {
				for (int i{0}; i < message.operation_size(); ++i) {
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
				for (int i{0}; i < message.args_size(); ++i) {
					scalar2log(message.args(i));
				}
			}

			const bool has_limit = message.has_limit();
			DBG_INF_FMT("limit is %s", has_collection? "SET":"NOT SET");
			if (has_limit) {
				const Mysqlx::Crud::Limit & limit = message.limit();
				DBG_INF_FMT("row_count[%s]=" MYSQLX_LLU_SPEC, limit.has_row_count()? "SET":"NOT SET", limit.has_row_count()? limit.row_count() :0);
				DBG_INF_FMT("offset   [%s]=" MYSQLX_LLU_SPEC, limit.has_offset()? "SET":"NOT SET", limit.has_offset()? limit.offset() :0);
			}

			DBG_INF_FMT("order_size=%d", message.order_size());
			if (message.order_size()) {
				for (int i{0}; i < message.order_size(); ++i) {
					const Mysqlx::Crud::Order & order = message.order(i);

					/* expression dump */
					DBG_INF_FMT("expr is %s", order.has_expr()? "SET":"NOT SET");
					if (order.has_expr()) {
						xmysqlnd_dump_expr(order.expr());
					}

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
	DBG_INF_FMT("collation[%s] is [" MYSQLX_LLU_SPEC "]", has_collation? "SET":"NOT SET",
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

	const bool has_content_type = meta.has_content_type();
	DBG_INF_FMT("content_type[%s] is [%u]", has_content_type? "SET":"NOT SET",
											has_content_type? meta.content_type() : 0);

	DBG_VOID_RETURN;
}

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

static void
xmysqlnd_dump_changed_state(const Mysqlx::Notice::SessionStateChanged & message)
{
	DBG_ENTER("xmysqlnd_dump_changed_state");

	const bool has_param = message.has_param();
	DBG_INF_FMT("param[%s] is %s", has_param? "SET":"NOT SET",
								   has_param? Mysqlx::Notice::SessionStateChanged::Parameter_Name(message.param()).c_str() : "n/a");


	const bool has_value = 0 < message.value_size();
	DBG_INF_FMT("value is %s", has_value? "SET":"NOT SET");
	if (has_value) {
		repeated2log(message.value());
	}

	DBG_VOID_RETURN;
}

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
		const char* frame_payload_str = frame.payload().c_str();
		const int frame_payload_size = static_cast<int>(frame.payload().size());
		switch (frame.type()) {
			case 1:{ /* Warning */
				Mysqlx::Notice::Warning message;
				DBG_INF("Warning");
				message.ParseFromArray(frame_payload_str, frame_payload_size);
				xmysqlnd_dump_warning(message);
				break;
			}
			case 2:{ /* SessionVariableChanged */
				Mysqlx::Notice::SessionVariableChanged message;
				DBG_INF("SessionVariableChanged");
				message.ParseFromArray(frame_payload_str, frame_payload_size);
				xmysqlnd_dump_changed_variable(message);
				break;
			}
			case 3:{ /* SessionStateChanged */
				Mysqlx::Notice::SessionStateChanged message;
				DBG_INF("SessionStateChanged");
				message.ParseFromArray(frame_payload_str, frame_payload_size);
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

static void
xmysqlnd_dump_capabilities_to_log(const Mysqlx::Connection::Capabilities & message)
{
	DBG_ENTER("xmysqlnd_dump_capabilities_to_log");
	for (int i{0}; i < message.capabilities_size(); ++i) {
		DBG_INF_FMT("Cap_name[i]=%s", message.capabilities(i).name().c_str());
		any2log(message.capabilities(i).value());
	}
	DBG_VOID_RETURN;
}

void
xmysqlnd_dump_server_message(const zend_uchar packet_type, const void * payload, const int payload_size)
{
	DBG_ENTER("xmysqlnd_dump_server_message");
	const Mysqlx::ServerMessages_Type type = (Mysqlx::ServerMessages_Type)(packet_type);
	DBG_INF_FMT("packet is %s   payload_size=%u", Mysqlx::ServerMessages_Type_Name(type).c_str(), static_cast<unsigned int>(payload_size));
	{
		char* message_dump = new char[payload_size*3 + 1];
		message_dump[payload_size*3] = '\0';
		for (int i{0}; i < payload_size; i++) {
			message_dump[i*3+0] = hexconvtab[((const char*)payload)[i] >> 4];
			message_dump[i*3+1] = hexconvtab[((const char*)payload)[i] & 15];
			message_dump[i*3+2] = ' ';
		}
		DBG_INF_FMT("payload[%u]=[%s]", static_cast<unsigned int>(payload_size), message_dump);
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
			uint32_t code{0};
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

} // namespace drv

} // namespace mysqlx
