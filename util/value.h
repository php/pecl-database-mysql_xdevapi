/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2019 The PHP Group                                |
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
#ifndef MYSQL_XDEVAPI_UTIL_VALUE_H
#define MYSQL_XDEVAPI_UTIL_VALUE_H

#include "strings.h"

namespace mysqlx {

namespace util {

/* {{{ zvalue */
class zvalue
{
	public:
		zvalue();
		zvalue(const zvalue& rhs);
		zvalue(zvalue&& rhs);

		zvalue(std::nullptr_t value);
		zvalue(bool value);
		zvalue(int16_t value);
		zvalue(int32_t value);
		zvalue(int64_t value);
		zvalue(double value);

		zvalue(char value);
		zvalue(const string& value);
		zvalue(const string_view& value);
		zvalue(const std::string& value);
		zvalue(const char* value, std::size_t length = 0);

		~zvalue();

	public:
		zvalue& operator=(const zvalue& rhs);
		zvalue& operator=(zvalue&& rhs);

		zvalue& operator=(std::nullptr_t value);
		zvalue& operator=(bool value);
		zvalue& operator=(int16_t value);
		zvalue& operator=(int32_t value);
		zvalue& operator=(int64_t value);
		zvalue& operator=(double value);

		zvalue& operator=(char value);
		zvalue& operator=(const string& value);
		zvalue& operator=(const string_view& value);
		zvalue& operator=(const std::string& value);
		zvalue& operator=(const char* value);
		void assign(const char* value, std::size_t length = 0);

	public:
		zvalue clone() const;
		void undef();

		zval& ref();
		zval* ptr();

		zval* release();

	public:
		enum Type {
			Undefined = IS_UNDEF,
			Null = IS_NULL,
			False = IS_FALSE,
			True = IS_TRUE,
			Long = IS_LONG,
			Double = IS_DOUBLE,
			String = IS_STRING,
			Array = IS_ARRAY,
			Object = IS_OBJECT,
			Resource = IS_RESOURCE,
			Reference = IS_REFERENCE,
			Constant_ast = IS_CONSTANT_AST,
			Bool = _IS_BOOL,
			Callable = IS_CALLABLE,
			Void = IS_VOID
		};

		Type type() const;

		bool is_undef() const;
		bool is_null() const;
		bool is_false() const;
		bool is_true() const;
		bool is_bool() const;
		bool is_long() const;
		bool is_double() const;
		bool is_string() const;
		bool is_array() const;
		bool is_object() const;

	public:
		bool to_bool() const;
		int64_t to_long() const;
		double to_double() const;
		string to_string() const;
		const char* get_str_ptr() const;
		std::size_t get_str_length() const;

	private:
		bool owner{true};
		zval zv;

};
/* }}} */


/* {{{ zvalue_ptr */
class zvalue_ptr
{
	public:
		zvalue_ptr(zval* zv);

	public:
		enum Type {
			Undefined = IS_UNDEF,
			Null = IS_NULL,
			False = IS_FALSE,
			True = IS_TRUE,
			Long = IS_LONG,
			Double = IS_DOUBLE,
			String = IS_STRING,
			Array = IS_ARRAY,
			Object = IS_OBJECT,
			Resource = IS_RESOURCE,
			Reference = IS_REFERENCE,
			Constant_ast = IS_CONSTANT_AST,
			Bool = _IS_BOOL,
			Callable = IS_CALLABLE,
			Void = IS_VOID
		};

		Type type() const;

		bool is_null() const;
		bool is_false() const;
		bool is_true() const;
		bool is_bool() const;
		bool is_long() const;
		bool is_double() const;
		bool is_string() const;
		bool is_array() const;
		bool is_object() const;

	private:
		zval* zv{nullptr};
};
/* }}} */

} // namespace util

} // namespace mysqlx

#endif // MYSQL_XDEVAPI_UTIL_VALUE_H
