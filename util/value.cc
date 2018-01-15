/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "value.h"

namespace mysqlx {

namespace util {

zvalue::zvalue()
{
	ZVAL_UNDEF(&zv);
}

zvalue::zvalue(const zvalue& rhs)
{
	ZVAL_COPY(&zv, &rhs.zv);
}

zvalue::zvalue(zvalue&& rhs)
{
	ZVAL_UNDEF(&zv);
	std::swap(zv, rhs.zv);
}

zvalue::zvalue(std::nullptr_t value)
{
	ZVAL_NULL(&zv);
}

zvalue::zvalue(bool value)
{
	ZVAL_BOOL(&zv, value);
}

zvalue::zvalue(int16_t value)
{
	ZVAL_LONG(&zv, value);
}

zvalue::zvalue(int32_t value)
{
	ZVAL_LONG(&zv, value);
}

zvalue::zvalue(int64_t value)
{
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
}

zvalue::zvalue(double value)
{
	ZVAL_DOUBLE(&zv, value);
}

zvalue::zvalue(char value)
{
	ZVAL_STRINGL(&zv, &value, 1);
}

zvalue::zvalue(const string& value)
	: zvalue(value.c_str(), value.length())
{
}

zvalue::zvalue(const string_view& value)
	: zvalue(value.c_str(), value.length())
{
}

zvalue::zvalue(const std::string& value)
	: zvalue(value.c_str(), value.length())
{
}

zvalue::zvalue(const char* value, std::size_t length)
{
	ZVAL_UNDEF(&zv);
	assign(value, length);
}

zvalue::~zvalue()
{
	if (owner) {
		zval_ptr_dtor(&zv);
	}
}

// -----------------------------------------------------------------------------

zvalue& zvalue::operator=(const zvalue& value)
{
	if (this == &value) return *this;

	zval_ptr_dtor(&zv);
	zv = value.zv;
	Z_TRY_ADDREF(zv);

	return *this;
}

zvalue& zvalue::operator=(zvalue&& rhs)
{
	if (this == &rhs) return *this;
	std::swap(zv, rhs.zv);
	return *this;
}

zvalue& zvalue::operator=(std::nullptr_t value)
{
	zval_dtor(&zv);
	ZVAL_NULL(&zv);
	return *this;
}

zvalue& zvalue::operator=(bool value)
{
	zval_dtor(&zv);
	ZVAL_BOOL(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(int16_t value)
{
	zval_dtor(&zv);
	ZVAL_LONG(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(int32_t value)
{
	zval_dtor(&zv);
	ZVAL_LONG(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(int64_t value)
{
	// TODO fix needed for 32bit
	zval_dtor(&zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
	return *this;
}

zvalue& zvalue::operator=(double value)
{
	zval_dtor(&zv);
	ZVAL_DOUBLE(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(char value)
{
	zval_dtor(&zv);
	ZVAL_STRINGL(&zv, &value, 1);
	return *this;
}

zvalue& zvalue::operator=(const string& value)
{
	assign(value.c_str(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const string_view& value)
{
	assign(value.c_str(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const std::string& value)
{
	assign(value.c_str(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const char* value)
{
	assign(value);
	return *this;
}

void zvalue::assign(const char* value, std::size_t length)
{
	zval_dtor(&zv);
	if (value)
	{
		ZVAL_STRINGL(&zv, value, length < 0 ? std::strlen(value) : length);
	}
	else
	{
		ZVAL_NULL(&zv);
	}
}

// -----------------------------------------------------------------------------

zvalue zvalue::clone() const
{
	zvalue result;
	ZVAL_DUP(&result.zv, &zv);
	return result;
}

void zvalue::undef()
{
	if (Z_TYPE(zv) == IS_UNDEF) return;
	zval_ptr_dtor(&zv);
	ZVAL_UNDEF(&zv);
}

zval& zvalue::ref()
{
	return zv;
}

zval* zvalue::ptr()
{
	return &zv;
}

zval* zvalue::release()
{
	// TODO: needs better zval management
	owner = false;
	return &zv;
}

// -----------------------------------------------------------------------------

zvalue::Type zvalue::type() const
{
	return static_cast<Type>(Z_TYPE(zv));
}

bool zvalue::is_undef() const
{
	return type() == Type::Undefined;
}

bool zvalue::is_null() const
{
	return type() == Type::Null;
}

bool zvalue::is_false() const
{
	return type() == Type::False;
}

bool zvalue::is_true() const
{
	return type() == Type::True;
}

bool zvalue::is_bool() const
{
	const Type t = type();
	return (t == Type::False) || (t == Type::True);
}

bool zvalue::is_long() const
{
	return type() == Type::Long;
}

bool zvalue::is_double() const
{
	return type() == Type::Double;
}

bool zvalue::is_string() const
{
	return type() == Type::String;
}

bool zvalue::is_array() const
{
	return type() == Type::Array;
}

bool zvalue::is_object() const
{
	return type() == Type::Object;
}

// -----------------------------------------------------------------------------

bool zvalue::to_bool() const
{
	switch (type())
	{
		case Type::False:
			return false;

		case Type::True:
			return true;

		default:
			assert(!"non-bool zvalue");
			return false;
	}
}

int64_t zvalue::to_long() const
{
	assert(is_long());
	return Z_LVAL(zv);
}

double zvalue::to_double() const
{
	assert(is_double());
	return Z_DVAL(zv);
}

string zvalue::to_string() const
{
	assert(is_string());
	return string { Z_STRVAL(zv), Z_STRLEN(zv) };
}

const char* zvalue::get_str_ptr() const
{
	assert(is_string());
	return Z_STRVAL(zv);
}

std::size_t zvalue::get_str_length() const
{
	assert(is_string());
	return Z_STRLEN(zv);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

zvalue_ptr::zvalue_ptr(zval* zv_ptr) : zv(zv_ptr)
{
}

// -----------------------------------------------------------------------------

zvalue_ptr::Type zvalue_ptr::type() const
{
	return static_cast<Type>(Z_TYPE_P(zv));
}

bool zvalue_ptr::is_null() const
{
	return type() == Type::Null;
}

bool zvalue_ptr::is_false() const
{
	return type() == Type::False;
}

bool zvalue_ptr::is_true() const
{
	return type() == Type::True;
}

bool zvalue_ptr::is_bool() const
{
	const Type t = type();
	return (t == Type::False) || (t == Type::True);
}

bool zvalue_ptr::is_long() const
{
	return type() == Type::Long;
}

bool zvalue_ptr::is_double() const
{
	return type() == Type::Double;
}

bool zvalue_ptr::is_string() const
{
	return type() == Type::String;
}

bool zvalue_ptr::is_array() const
{
	return type() == Type::Array;
}

bool zvalue_ptr::is_object() const
{
	return type() == Type::Object;
}

} // namespace util

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
