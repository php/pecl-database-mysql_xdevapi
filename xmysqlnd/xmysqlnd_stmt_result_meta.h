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
#ifndef XMYSQLND_STMT_RESULT_META_H
#define XMYSQLND_STMT_RESULT_META_H

#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_driver.h"

namespace mysqlx {

namespace drv {

/*
 * After we refactor to C++ this enum shall
 * be updated automatically by the data from
 * the protobuf generated files.
 */
enum xmysqlnd_field_type
{
	XMYSQLND_TYPE_SIGNED_INT	= 1,
	XMYSQLND_TYPE_UNSIGNED_INT	= 2,
	XMYSQLND_TYPE_DOUBLE		= 5,
	XMYSQLND_TYPE_FLOAT         = 6,
	XMYSQLND_TYPE_BYTES         = 7,
	XMYSQLND_TYPE_TIME          = 10,
	XMYSQLND_TYPE_DATETIME		= 12,
	XMYSQLND_TYPE_SET           = 15,
	XMYSQLND_TYPE_ENUM          = 16,
	XMYSQLND_TYPE_BIT           = 17,
	XMYSQLND_TYPE_DECIMAL		= 18,
	XMYSQLND_TYPE_NONE          = 255,
};


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

struct st_xmysqlnd_result_field_meta : public util::custom_allocable
{
	enum xmysqlnd_field_type type;
	util::string name;
	util::string original_name;
	util::string table;
	util::string original_table;
	util::string schema;
	util::string catalog;
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

typedef struct st_xmysqlnd_stmt_result_meta XMYSQLND_STMT_RESULT_META;

typedef enum_func_status	(*func_xmysqlnd_stmt_result_meta__init)(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_stmt_result_meta__add_field)(XMYSQLND_STMT_RESULT_META * const meta, XMYSQLND_RESULT_FIELD_META * field, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef unsigned int		(*func_xmysqlnd_stmt_result_meta__get_field_count)(const XMYSQLND_STMT_RESULT_META * const meta);
typedef const XMYSQLND_RESULT_FIELD_META * (*func_xmysqlnd_stmt_result_meta__get_field)(const XMYSQLND_STMT_RESULT_META * const meta, unsigned int field);
typedef void				(*func_xmysqlnd_stmt_result_meta__free_contents)(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
typedef void				(*func_xmysqlnd_stmt_result_meta__dtor)(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result_meta)
{
	func_xmysqlnd_stmt_result_meta__init init;
	func_xmysqlnd_stmt_result_meta__add_field add_field;
	func_xmysqlnd_stmt_result_meta__get_field_count get_field_count;
	func_xmysqlnd_stmt_result_meta__get_field get_field;
	func_xmysqlnd_stmt_result_meta__free_contents free_contents;
	func_xmysqlnd_stmt_result_meta__dtor dtor;
};

struct st_xmysqlnd_stmt_result_meta : public util::custom_allocable
{
	unsigned int field_count;
	XMYSQLND_RESULT_FIELD_META ** fields;
	unsigned int fields_size;

	const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_stmt_result_meta) * m;
	zend_bool		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_stmt_result_meta);
PHP_MYSQL_XDEVAPI_API XMYSQLND_STMT_RESULT_META * xmysqlnd_stmt_result_meta_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_stmt_result_meta_free(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_STMT_RESULT_META_H */
