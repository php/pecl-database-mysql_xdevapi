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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_NODE_STMT_RESULT_META_H
#define XMYSQLND_NODE_STMT_RESULT_META_H

#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_driver.h"

#ifdef __cplusplus
#include "proto_gen/mysqlx_resultset.pb.h"
#endif
enum xmysqlnd_field_type
{
#ifdef __cplusplus
	XMYSQLND_TYPE_SIGNED_INT	= Mysqlx::Resultset::ColumnMetaData_FieldType_SINT,
	XMYSQLND_TYPE_UNSIGNED_INT	= Mysqlx::Resultset::ColumnMetaData_FieldType_UINT,
	XMYSQLND_TYPE_DOUBLE		= Mysqlx::Resultset::ColumnMetaData_FieldType_DOUBLE,
	XMYSQLND_TYPE_FLOAT			= Mysqlx::Resultset::ColumnMetaData_FieldType_FLOAT,
	XMYSQLND_TYPE_BYTES			= Mysqlx::Resultset::ColumnMetaData_FieldType_BYTES,
	XMYSQLND_TYPE_TIME			= Mysqlx::Resultset::ColumnMetaData_FieldType_TIME,
	XMYSQLND_TYPE_DATETIME		= Mysqlx::Resultset::ColumnMetaData_FieldType_DATETIME,
	XMYSQLND_TYPE_SET			= Mysqlx::Resultset::ColumnMetaData_FieldType_SET,
	XMYSQLND_TYPE_ENUM			= Mysqlx::Resultset::ColumnMetaData_FieldType_ENUM,
	XMYSQLND_TYPE_BIT			= Mysqlx::Resultset::ColumnMetaData_FieldType_BIT,
	XMYSQLND_TYPE_DECIMAL		= Mysqlx::Resultset::ColumnMetaData_FieldType_DECIMAL,
#endif
	XMYSQLND_TYPE_NONE = 255,
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_xmysqlnd_result_field_meta XMYSQLND_RESULT_FIELD_META;
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__init)(XMYSQLND_RESULT_FIELD_META * const field, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_type)(XMYSQLND_RESULT_FIELD_META * const field, enum xmysqlnd_field_type type);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_original_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_original_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_schema)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_catalog)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_collation)(XMYSQLND_RESULT_FIELD_META * const field, const uint64_t collation);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_fractional_digits)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t digits);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_length)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t length);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_flags)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t flags);
typedef enum_func_status	(*func_xmysqlnd_result_field_meta__set_content_type)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t content_type);
typedef XMYSQLND_RESULT_FIELD_META * (*func_xmysqlnd_result_field_meta__clone)(const XMYSQLND_RESULT_FIELD_META * const field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void				(*func_xmysqlnd_result_field_meta__free_contents)(XMYSQLND_RESULT_FIELD_META * const field);
typedef void				(*func_xmysqlnd_result_field_meta__dtor)(XMYSQLND_RESULT_FIELD_META * const field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta)
{
	func_xmysqlnd_result_field_meta__init init;
	func_xmysqlnd_result_field_meta__set_type set_type;
	func_xmysqlnd_result_field_meta__set_name set_name;
	func_xmysqlnd_result_field_meta__set_original_name set_original_name;
	func_xmysqlnd_result_field_meta__set_table set_table;
	func_xmysqlnd_result_field_meta__set_original_table set_original_table;
	func_xmysqlnd_result_field_meta__set_schema set_schema;
	func_xmysqlnd_result_field_meta__set_catalog set_catalog;
	func_xmysqlnd_result_field_meta__set_collation set_collation;
	func_xmysqlnd_result_field_meta__set_fractional_digits set_fractional_digits;
	func_xmysqlnd_result_field_meta__set_length set_length;
	func_xmysqlnd_result_field_meta__set_flags set_flags;
	func_xmysqlnd_result_field_meta__set_content_type set_content_type;
	func_xmysqlnd_result_field_meta__clone clone;
	func_xmysqlnd_result_field_meta__free_contents free_contents;
	func_xmysqlnd_result_field_meta__dtor dtor;
};

struct st_xmysqlnd_result_field_meta
{
	enum xmysqlnd_field_type type;
	MYSQLND_STRING name;
	MYSQLND_STRING original_name;
	MYSQLND_STRING table;
	MYSQLND_STRING original_table;
	MYSQLND_STRING schema;
	MYSQLND_STRING catalog;
	uint64_t collation;
	uint32_t fractional_digits;
	uint32_t length;
	uint32_t flags;
	uint32_t content_type;
	struct
	{
		zend_string * sname;
		zend_bool	is_numeric;
		zend_ulong	key;
	} zend_hash_key;

	zend_bool type_set:1;
	zend_bool collation_set:1;
	zend_bool fractional_digits_set:1;
	zend_bool length_set:1;
	zend_bool flags_set:1;
	zend_bool content_type_set:1;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * object_factory;
	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_result_field_meta) * m;
	zend_bool persistent;
};

PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_result_field_meta);
PHP_MYSQL_XDEVAPI_API XMYSQLND_RESULT_FIELD_META * xmysqlnd_result_field_meta_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_result_field_meta_free(XMYSQLND_RESULT_FIELD_META * const field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);


/*******************************************************************************************************************************************/

typedef struct st_xmysqlnd_node_stmt_result_meta XMYSQLND_NODE_STMT_RESULT_META;

typedef enum_func_status	(*func_xmysqlnd_node_stmt_result_meta__init)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_node_stmt_result_meta__add_field)(XMYSQLND_NODE_STMT_RESULT_META * const meta, XMYSQLND_RESULT_FIELD_META * field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef unsigned int		(*func_xmysqlnd_node_stmt_result_meta__get_field_count)(const XMYSQLND_NODE_STMT_RESULT_META * const meta);
typedef const XMYSQLND_RESULT_FIELD_META * (*func_xmysqlnd_node_stmt_result_meta__get_field)(const XMYSQLND_NODE_STMT_RESULT_META * const meta, unsigned int field);
typedef void				(*func_xmysqlnd_node_stmt_result_meta__free_contents)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void				(*func_xmysqlnd_node_stmt_result_meta__dtor)(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta)
{
	func_xmysqlnd_node_stmt_result_meta__init init;
	func_xmysqlnd_node_stmt_result_meta__add_field add_field;
	func_xmysqlnd_node_stmt_result_meta__get_field_count get_field_count;
	func_xmysqlnd_node_stmt_result_meta__get_field get_field;
	func_xmysqlnd_node_stmt_result_meta__free_contents free_contents;
	func_xmysqlnd_node_stmt_result_meta__dtor dtor;
};

struct st_xmysqlnd_node_stmt_result_meta
{
	unsigned int field_count;
	XMYSQLND_RESULT_FIELD_META ** fields;
	unsigned int fields_size;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_node_stmt_result_meta) * m;
	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_node_stmt_result_meta);
PHP_MYSQL_XDEVAPI_API XMYSQLND_NODE_STMT_RESULT_META * xmysqlnd_node_stmt_result_meta_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_node_stmt_result_meta_free(XMYSQLND_NODE_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XMYSQLND_NODE_STMT_RESULT_META_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
