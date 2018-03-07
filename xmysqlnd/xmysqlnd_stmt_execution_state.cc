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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_stmt_execution_state.h"
#include "util/string_utils.h"
#include "util/types.h"

namespace mysqlx {

namespace drv {

/* {{{ xmysqlnd_stmt_execution_state::init */
static enum_func_status
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, init)(XMYSQLND_STMT_EXECUTION_STATE * const state,
													 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
													 MYSQLND_STATS * const stats,
													 MYSQLND_ERROR_INFO * const error_info)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::init");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::get_affected_items_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_affected_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::get_affected_items_count");
	DBG_RETURN(state->items_affected);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::get_matched_items_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_matched_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::get_matched_items_count");
	DBG_RETURN(state->items_matched);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::get_found_items_count */
static size_t
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_found_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::get_found_items_count");
	DBG_RETURN(state->items_found);
}
/* }}} */



/* {{{ xmysqlnd_stmt_execution_state::get_last_insert_id */
static uint64_t
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_last_insert_id)(const XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::get_last_insert_id");
	DBG_RETURN(state->last_insert_id);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::get_generated_ids */
static const mysqlx::util::vector<mysqlx::util::string> *
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_generated_ids)(const XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::get_generated_ids");
	DBG_RETURN(&state->generated_doc_ids);
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::set_affected_items_count */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_affected_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::set_affected_items_count");
	DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, value);
	state->items_affected = value;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::set_matched_items_count */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_matched_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::set_matched_items_count");
	DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, value);
	state->items_matched = value;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::set_found_items_count */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_found_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::set_found_items_count");
	DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, value);
	state->items_found = value;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::add_generated_doc_id */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, add_generated_doc_id)(XMYSQLND_STMT_EXECUTION_STATE * const state,
																	 const MYSQLND_STRING id)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::add_generated_doc_id");
	state->generated_doc_ids.push_back( util::to_string(id) );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::set_last_insert_id */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_last_insert_id)(XMYSQLND_STMT_EXECUTION_STATE * const state, const uint64_t value)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::set_last_insert_id");
	DBG_INF_FMT("value=" MYSQLND_LLU_SPEC, value);
	state->last_insert_id = value;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::free_contents */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, free_contents)(XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::free_contents");
	if( state && !state->generated_doc_ids.empty() ) {
		state->generated_doc_ids.clear();
		state->generated_doc_ids.shrink_to_fit();
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ xmysqlnd_stmt_execution_state::dtor */
static void
XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, dtor)(XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state::dtor");
	if (state) {
		state->m->free_contents(state);
		mnd_pefree(state, state->persistent);
	}
	DBG_VOID_RETURN;
}
/* }}} */


static
MYSQLND_CLASS_METHODS_START(xmysqlnd_stmt_execution_state)
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, init),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_affected_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_matched_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_found_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_last_insert_id),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, get_generated_ids),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_affected_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_matched_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_found_items_count),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, set_last_insert_id),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, add_generated_doc_id),


	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, free_contents),
	XMYSQLND_METHOD(xmysqlnd_stmt_execution_state, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_stmt_execution_state);

/* {{{ xmysqlnd_stmt_execution_state_create */
PHP_MYSQL_XDEVAPI_API XMYSQLND_STMT_EXECUTION_STATE *
xmysqlnd_stmt_execution_state_create(const zend_bool persistent,
									 const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
									 MYSQLND_STATS * stats,
									 MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_STMT_EXECUTION_STATE* result{nullptr};
	DBG_ENTER("xmysqlnd_stmt_execution_state_create");
	result = object_factory->get_stmt_execution_state(object_factory, persistent, stats, error_info);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ xmysqlnd_node_stmt_result_free */
PHP_MYSQL_XDEVAPI_API void
xmysqlnd_stmt_execution_state_free(XMYSQLND_STMT_EXECUTION_STATE * const state)
{
	DBG_ENTER("xmysqlnd_stmt_execution_state_free");
	DBG_INF_FMT("result=%p", state);
	if (state) {
		state->m->dtor(state);
	}
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
