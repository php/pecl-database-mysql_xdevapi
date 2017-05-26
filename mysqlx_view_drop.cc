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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <zend_exceptions.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_schema.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result.h"
#include "xmysqlnd/xmysqlnd_ddl_view_commands.h"
#include "xmysqlnd/xmysqlnd_view.h"
#include "xmysqlnd/xmysqlnd_utils.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_exception.h"
#include "mysqlx_executable.h"
#include "mysqlx_expression.h"
#include "mysqlx_node_result.h"
#include "mysqlx_node_sql_statement.h"
#include "mysqlx_object.h"
#include "mysqlx_view_drop.h"
#include "phputils/allocator.h"
#include "phputils/exceptions.h"
#include "phputils/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;


/* {{{ view_drop */
bool view_drop(
	drv::st_xmysqlnd_node_schema* schema,
	const phputils::string_input_param& view_name)
{
	DBG_ENTER("mysqlx::devapi::view_drop");

	const phputils::string_input_param schema_name(schema->data->schema_name);
	if (schema_name.empty() || view_name.empty()) {
		throw phputils::xdevapi_exception(phputils::xdevapi_exception::Code::view_drop_fail);
	}

	drv::Drop_view_cmd command;
	command.set_view_name(schema_name, view_name);
	command.set_if_exists(true);

	auto session = schema->data->session;
	const st_xmysqlnd_pb_message_shell pb_msg = command.get_message();
	st_xmysqlnd_node_stmt* stmt = drv::View::drop(session, pb_msg);
	zend_long flags = 0;
	zval drop_result;
	ZVAL_UNDEF(&drop_result);
	execute_new_statement_read_response(
		stmt,
		flags,
		MYSQLX_RESULT,
		&drop_result);

	if (Z_TYPE(drop_result) == IS_OBJECT) {
		auto& result_data = phputils::fetch_data_object<st_mysqlx_node_result>(&drop_result);
		return result_data.result->warnings->warning_count == 0;
	}

	return false;
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
