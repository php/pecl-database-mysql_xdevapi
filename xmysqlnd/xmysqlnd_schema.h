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
#ifndef XMYSQLND_SCHEMA_H
#define XMYSQLND_SCHEMA_H

#include "xmysqlnd_driver.h"
#include "util/allocator.h"
#include "util/strings.h"
#include "xmysqlnd/xmysqlnd_session.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_session;
struct xmysqlnd_collection;
struct xmysqlnd_table;
struct st_xmysqlnd_session_on_error_bind;

struct st_xmysqlnd_schema_on_database_object_bind
{
	void (*handler)(void * context, xmysqlnd_schema * const schema, const util::string_view& object_name, const util::string_view& object_type);
	void * ctx;
};

struct st_xmysqlnd_schema_on_error_bind
{
	const enum_hnd_func_status (*handler)(void * context, const xmysqlnd_schema * const schema, const unsigned int code, const util::string_view& sql_state, const util::string_view& message);
	void * ctx;
};

bool is_table_object_type(const util::string_view& object_type);
bool is_collection_object_type(const util::string_view& object_type);
bool is_view_object_type(const util::string_view& object_type);

enum class db_object_type_filter
{
	table_or_view,
	collection
};

class xmysqlnd_schema : public util::custom_allocable
{
public:
	xmysqlnd_schema() = default;
	xmysqlnd_schema(const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const obj_factory,
							XMYSQLND_SESSION session,
							const util::string_view& schema_name);
	~xmysqlnd_schema();
	void cleanup();

	enum_func_status        exists_in_database(struct st_xmysqlnd_session_on_error_bind on_error, zval* exists);
	xmysqlnd_collection* create_collection_object(const util::string_view& collection_name);
	xmysqlnd_collection* create_collection(
		const util::string_view& collection_name,
		const util::string_view& collection_options,
		const st_xmysqlnd_schema_on_error_bind on_error);
	bool modify_collection(
		const util::string_view& collection_name,
		const util::string_view& collection_options,
		const st_xmysqlnd_schema_on_error_bind on_error);
	enum_func_status        drop_collection(const util::string_view& collection_name,const st_xmysqlnd_schema_on_error_bind on_error);
	xmysqlnd_table*      create_table_object(const util::string_view& table_name);
	enum_func_status        drop_table(const util::string_view& table_name,const st_xmysqlnd_schema_on_error_bind on_error);
	enum_func_status        get_db_objects(const util::string_view& collection_name,const db_object_type_filter object_type_filter,const st_xmysqlnd_schema_on_database_object_bind on_object,const st_xmysqlnd_schema_on_error_bind on_error);

	xmysqlnd_schema *	    get_reference();
	enum_func_status	    free_reference(MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
	void				    free_contents();
	const util::string& get_name() const {
		return schema_name;
	}
	XMYSQLND_SESSION        get_session() {
		return session;
	}
	zend_bool				is_persistent() {
		return persistent;
	}
	unsigned int            get_counter() {
		return refcount;
	}
private:
	XMYSQLND_SESSION session;
	util::string schema_name;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;

	unsigned int	refcount;
	zend_bool		persistent;
};


xmysqlnd_schema* xmysqlnd_schema_create(XMYSQLND_SESSION session,
														  const util::string_view& schema_name,
														  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
														  MYSQLND_STATS * const stats,
														  MYSQLND_ERROR_INFO * const error_info);

void xmysqlnd_schema_free(xmysqlnd_schema * const schema, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_SCHEMA_H */
