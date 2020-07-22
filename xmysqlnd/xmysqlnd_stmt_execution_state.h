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
#ifndef XMYSQLND_STMT_EXECUTION_STATE_H
#define XMYSQLND_STMT_EXECUTION_STATE_H

#include "xmysqlnd_driver.h"
#include "util/allocator.h"
#include "util/types.h"
#include "util/strings.h"

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_stmt_execution_state XMYSQLND_STMT_EXECUTION_STATE;

typedef enum_func_status	(*func_xmysqlnd_stmt_execution_state__init)(XMYSQLND_STMT_EXECUTION_STATE * const result, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef size_t		(*func_xmysqlnd_stmt_execution_state__get_affected_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state);
typedef size_t		(*func_xmysqlnd_stmt_execution_state__get_matched_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state);
typedef size_t		(*func_xmysqlnd_stmt_execution_state__get_found_items_count)(const XMYSQLND_STMT_EXECUTION_STATE * const state);
typedef uint64_t	(*func_xmysqlnd_stmt_execution_state__get_last_insert_id)(const XMYSQLND_STMT_EXECUTION_STATE * const state);
typedef const util::vector< util::string>* (*func_xmysqlnd_stmt_execution_state__get_generated_ids)(const XMYSQLND_STMT_EXECUTION_STATE * const state);

typedef void		(*func_xmysqlnd_stmt_execution_state__set_affected_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value);
typedef void		(*func_xmysqlnd_stmt_execution_state__set_matched_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value);
typedef void		(*func_xmysqlnd_stmt_execution_state__set_found_items_count)(XMYSQLND_STMT_EXECUTION_STATE * const state, const size_t value);
typedef void		(*func_xmysqlnd_stmt_execution_state__set_last_insert_id)(XMYSQLND_STMT_EXECUTION_STATE * const state, const uint64_t value);
typedef void		(*func_xmysqlnd_stmt_execution_state__set_add_generated_doc_id)(XMYSQLND_STMT_EXECUTION_STATE * const state, const util::string& id);


typedef void		(*func_xmysqlnd_stmt_execution_state__free_contents)(XMYSQLND_STMT_EXECUTION_STATE * const state);
typedef void		(*func_xmysqlnd_stmt_execution_state__dtor)(XMYSQLND_STMT_EXECUTION_STATE * const state);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state)
{
	func_xmysqlnd_stmt_execution_state__init init;
	func_xmysqlnd_stmt_execution_state__get_affected_items_count get_affected_items_count;
	func_xmysqlnd_stmt_execution_state__get_matched_items_count get_matched_items_count;
	func_xmysqlnd_stmt_execution_state__get_found_items_count get_found_items_count;
	func_xmysqlnd_stmt_execution_state__get_last_insert_id get_last_insert_id;
	func_xmysqlnd_stmt_execution_state__get_generated_ids get_generated_ids;

	func_xmysqlnd_stmt_execution_state__set_affected_items_count set_affected_items_count;
	func_xmysqlnd_stmt_execution_state__set_matched_items_count set_matched_items_count;
	func_xmysqlnd_stmt_execution_state__set_found_items_count set_found_items_count;
	func_xmysqlnd_stmt_execution_state__set_last_insert_id set_last_insert_id;
	func_xmysqlnd_stmt_execution_state__set_add_generated_doc_id add_generated_doc_id;


	func_xmysqlnd_stmt_execution_state__free_contents free_contents;
	func_xmysqlnd_stmt_execution_state__dtor dtor;
};

struct st_xmysqlnd_stmt_execution_state : public util::custom_allocable
{
	size_t items_affected;
	size_t items_matched;
	size_t items_found;
	uint64_t last_insert_id;
	util::vector< util::string> generated_doc_ids;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_execution_state) * m;
	zend_bool persistent;
};

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_stmt_execution_state);
PHP_MYSQL_XDEVAPI_API XMYSQLND_STMT_EXECUTION_STATE * xmysqlnd_stmt_execution_state_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_stmt_execution_state_free(XMYSQLND_STMT_EXECUTION_STATE * const state);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STMT_EXECUTION_STATE_H */
