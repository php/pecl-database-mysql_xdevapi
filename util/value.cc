/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
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
#include "util/exceptions.h"

/*
	useful links:
	https://medium.com/@davidtstrauss/copy-and-move-semantics-of-zvals-in-php-7-41427223d784
	https://wiki.php.net/phpng-upgrading
	http://blog.jpauli.tech/2016-04-08-hashtables-html/
	http://blog.jpauli.tech/2015-09-18-php-string-management-html/
	http://blog.jpauli.tech/2016-01-14-php-7-objects-html/
	http://nikic.github.io/2014/12/22/PHPs-new-hashtable-implementation.html
	https://nikic.github.io/2015/05/05/Internal-value-representation-in-PHP-7-part-1.html
	https://nikic.github.io/2015/06/19/Internal-value-representation-in-PHP-7-part-2.html
	https://github.com/phpinternalsbook/PHP-Internals-Book/tree/master/Book/php7/internal_types/zvals
	http://www.phpinternalsbook.com/index.html

	also related to PHP5:
	http://www.phpinternalsbook.com/php5/hashtables/hashtable_api.html
	http://www.phpinternalsbook.com/php5/hashtables/array_api.html
	http://www.phpinternalsbook.com/php5/zvals/memory_management.html
*/

namespace mysqlx::util {

zvalue::zvalue()
{
	ZVAL_UNDEF(&zv);
}

zvalue::zvalue(Type type)
{
	switch (type) {
	case Type::Undefined:
		ZVAL_UNDEF(&zv);
		break;

	case Type::Null:
		ZVAL_NULL(&zv);
		break;

	case Type::False:
		ZVAL_FALSE(&zv);
		break;

	case Type::True:
		ZVAL_TRUE(&zv);
		break;

	case Type::Long:
		ZVAL_LONG(&zv, 0);
		break;

	case Type::Double:
		ZVAL_DOUBLE(&zv, 0.0);
		break;

	case Type::String:
		ZVAL_EMPTY_STRING(&zv);
		break;

	case Type::Array:
		array_init(&zv);
		break;

	case Type::Object:
		object_init(&zv);
		break;

	default:
		assert(!"default initialization of that type is not supported");
		ZVAL_UNDEF(&zv);
	}
}

zvalue::zvalue(const zvalue& rhs)
{
	ZVAL_ZVAL(&zv, rhs.ptr(), 1, 0);
}

zvalue::zvalue(zvalue&& rhs)
{
	ZVAL_ZVAL(&zv, &rhs.zv, 1, 1);
	ZVAL_UNDEF(&rhs.zv);
}

zvalue::zvalue(const zval& rhs)
{
	ZVAL_ZVAL(&zv, const_cast<zval*>(&rhs), 1, 0);
}

zvalue::zvalue(zval&& rhs)
{
	ZVAL_ZVAL(&zv, &rhs, 1, 1);
	ZVAL_UNDEF(&rhs);
}

zvalue::zvalue(const zval* rhs)
{
	if (rhs) {
		ZVAL_ZVAL(&zv, const_cast<zval*>(rhs), 1, 0);
	} else {
		ZVAL_UNDEF(&zv);
	}
}

zvalue::zvalue(std::nullptr_t /*value*/)
{
	ZVAL_NULL(&zv);
}

zvalue::zvalue(bool value)
{
	ZVAL_BOOL(&zv, value);
}

zvalue::zvalue(int32_t value)
{
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
}

zvalue::zvalue(int64_t value)
{
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
}

zvalue::zvalue(uint32_t value)
{
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
}

zvalue::zvalue(uint64_t value)
{
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
}

zvalue::zvalue(double value)
{
	ZVAL_DOUBLE(&zv, value);
}

zvalue::zvalue(char value)
	: zvalue(&value, 1)
{
}

zvalue::zvalue(const string& value)
	: zvalue(value.c_str(), value.length())
{
}

zvalue::zvalue(const string_view& value)
	: zvalue(value.data(), value.length())
{
}

zvalue::zvalue(const std::string& value)
	: zvalue(value.c_str(), value.length())
{
}

zvalue::zvalue(const char* value)
	: zvalue(value, std::strlen(value))
{
}

zvalue::zvalue(const char* value, std::size_t length)
{
	ZVAL_UNDEF(&zv);
	assign(value, length);
}

zvalue::zvalue(std::initializer_list<std::pair<const char*, zvalue>> values)
{
	ZVAL_UNDEF(&zv);
	insert(values);
}

zvalue zvalue::create_array(std::size_t size)
{
	zvalue arr;
	arr.reserve(size);
	return arr;
}

zvalue zvalue::create_object()
{
	return zvalue(Type::Object);
}

zvalue::~zvalue()
{
	zval_ptr_dtor(&zv);
}

// -----------------------------------------------------------------------------

zvalue& zvalue::operator=(const zvalue& rhs)
{
	if (this == &rhs) return *this;

	zval_ptr_dtor(&zv);
	ZVAL_ZVAL(&zv, rhs.ptr(), 1, 0);
	return *this;
}

zvalue& zvalue::operator=(zvalue&& rhs)
{
	if (this == &rhs) return *this;

	zval_ptr_dtor(&zv);
	ZVAL_ZVAL(&zv, &rhs.zv, 1, 1);
	ZVAL_UNDEF(&rhs.zv);
	return *this;
}

zvalue& zvalue::operator=(const zval& rhs)
{
	if (&zv == &rhs) return *this;

	zval_ptr_dtor(&zv);
	ZVAL_ZVAL(&zv, const_cast<zval*>(&rhs), 1, 0);
	return *this;
}

zvalue& zvalue::operator=(zval&& rhs)
{
	if (&zv == &rhs) return *this;

	zval_ptr_dtor(&zv);
	ZVAL_ZVAL(&zv, &rhs, 1, 1);
	ZVAL_UNDEF(&rhs);
	return *this;
}

zvalue& zvalue::operator=(const zval* rhs)
{
	if (&zv == rhs) return *this;

	zval_ptr_dtor(&zv);
	if (rhs) {
		ZVAL_ZVAL(&zv, const_cast<zval*>(rhs), 1, 0);
	} else {
		ZVAL_UNDEF(&zv);
	}
	return *this;
}

zvalue& zvalue::operator=(std::nullptr_t /*value*/)
{
	zval_ptr_dtor(&zv);
	ZVAL_NULL(&zv);
	return *this;
}

zvalue& zvalue::operator=(bool value)
{
	zval_ptr_dtor(&zv);
	ZVAL_BOOL(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(int32_t value)
{
	zval_ptr_dtor(&zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
	return *this;
}

zvalue& zvalue::operator=(int64_t value)
{
	zval_ptr_dtor(&zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
	return *this;
}

zvalue& zvalue::operator=(uint32_t value)
{
	zval_ptr_dtor(&zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
	return *this;
}

zvalue& zvalue::operator=(uint64_t value)
{
	zval_ptr_dtor(&zv);
	ZVAL_LONG(&zv, static_cast<zend_long>(value));
	return *this;
}

zvalue& zvalue::operator=(double value)
{
	zval_ptr_dtor(&zv);
	ZVAL_DOUBLE(&zv, value);
	return *this;
}

zvalue& zvalue::operator=(char value)
{
	assign(&value, 1);
	return *this;
}

zvalue& zvalue::operator=(const string& value)
{
	assign(value.c_str(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const string_view& value)
{
	assign(value.data(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const std::string& value)
{
	assign(value.c_str(), value.length());
	return *this;
}

zvalue& zvalue::operator=(const char* value)
{
	assign(value, std::strlen(value));
	return *this;
}

void zvalue::assign(const char* value, std::size_t length)
{
	assert(value && length);
	zval_ptr_dtor(&zv);
	ZVAL_STRINGL(&zv, value, length);
}

zvalue& zvalue::operator=(std::initializer_list<std::pair<const char*, zvalue>> values)
{
	insert(values);
	return *this;
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

// ---------------------

int zvalue::to_int() const
{
	assert(is_long());
	return static_cast<int>(Z_LVAL(zv));
}

long zvalue::to_long() const
{
	assert(is_long());
	return static_cast<long>(Z_LVAL(zv));
}

zend_long zvalue::to_zlong() const
{
	assert(is_long());
	return Z_LVAL(zv);
}

int32_t zvalue::to_int32() const
{
	assert(is_long());
	return static_cast<int32_t>(Z_LVAL(zv));
}

int64_t zvalue::to_int64() const
{
	assert(is_long());
	return static_cast<int64_t>(Z_LVAL(zv));
}

// ---------------------

unsigned int zvalue::to_uint() const
{
	assert(is_long());
	return static_cast<unsigned int>(Z_LVAL(zv));
}

unsigned long zvalue::to_ulong() const
{
	assert(is_long());
	return static_cast<unsigned long>(Z_LVAL(zv));
}

zend_ulong zvalue::to_zulong() const
{
	assert(is_long());
	return static_cast<zend_ulong>(Z_LVAL(zv));
}

std::size_t zvalue::to_size_t() const
{
	assert(is_long());
	return static_cast<std::size_t>(Z_LVAL(zv));
}

uint32_t zvalue::to_uint32() const
{
	assert(is_long());
	return static_cast<uint32_t>(Z_LVAL(zv));
}

uint64_t zvalue::to_uint64() const
{
	assert(is_long());
	return static_cast<uint64_t>(Z_LVAL(zv));
}

// ---------------------

double zvalue::to_double() const
{
	assert(is_double());
	return Z_DVAL(zv);
}

// ---------------------

string zvalue::to_string() const
{
	assert(is_string());
	return string{ Z_STRVAL(zv), Z_STRLEN(zv) };
}

string_view zvalue::to_string_view() const
{
	assert(is_string());
	return string_view{ Z_STRVAL(zv), Z_STRLEN(zv) };
}

std::string zvalue::to_std_string() const
{
	assert(is_string());
	return std::string{ Z_STRVAL(zv), Z_STRLEN(zv) };
}

const char* zvalue::c_str() const
{
	assert(is_string());
	return Z_STRVAL(zv);
}

// -----------------------------------------------------------------------------

std::size_t zvalue::size() const
{
	switch (type()) {
	case Type::String:
		return Z_STRLEN(zv);

	case Type::Array:
		return zend_array_count(Z_ARRVAL(zv));

	case Type::Object:
		return properties_count();

	default:
		assert(!"size unsupported for that type");
		return 0;
	}
}

void zvalue::clear()
{
	switch (type()) {
	case Type::String:
		zval_ptr_dtor(&zv);
		ZVAL_EMPTY_STRING(&zv);
		break;

	case Type::Array:
		zend_hash_clean(Z_ARRVAL(zv));
		break;

	default:
		assert(!"clear unsupported for that type");
	}
}

void zvalue::reserve(std::size_t size)
{
	if (is_array()) {
		zend_hash_extend(
			Z_ARRVAL(zv),
			static_cast<uint32_t>(size),
			(Z_ARRVAL(zv)->u.flags & HASH_FLAG_PACKED));
	} else {
		zval_ptr_dtor(&zv);
		array_init_size(&zv, static_cast<uint32_t>(size));
	}
}

void zvalue::reset()
{
	if (is_undef()) return;
	zval_ptr_dtor(&zv);
	ZVAL_UNDEF(&zv);
}

zvalue zvalue::clone() const
{
	zvalue result;
	ZVAL_DUP(&result.zv, &zv);
	return result;
}

zvalue zvalue::clone_from(zval* src)
{
	zvalue result;
	ZVAL_DUP(&result.zv, src);
	return result;
}

void zvalue::acquire(zval* src)
{
	assert(src);
	zval_ptr_dtor(&zv);
	ZVAL_ZVAL(&zv, src, 1, 1);
	ZVAL_UNDEF(src);
}

void zvalue::copy_to(zval* dst)
{
	assert(dst);
	ZVAL_ZVAL(dst, &zv, 1, 0);
}

void zvalue::copy_to(zval* src, zval* dst)
{
	assert(src && dst);
	ZVAL_ZVAL(dst, src, 1, 0);
}

void zvalue::move_to(zval* dst)
{
	assert(dst);
	ZVAL_ZVAL(dst, &zv, 1, 1);
	ZVAL_UNDEF(&zv);
}

void zvalue::move_to(zval* src, zval* dst)
{
	assert(src && dst);
	ZVAL_ZVAL(dst, src, 1, 1);
	ZVAL_UNDEF(src);
}

zval zvalue::release()
{
	zval other;
	ZVAL_ZVAL(&other, &zv, 1, 1);
	ZVAL_UNDEF(&zv);
	return other;
}

void zvalue::invalidate()
{
	ZVAL_UNDEF(&zv);
}

// -----------------------------------------------------------------------------

std::size_t zvalue::properties_count() const
{
	assert(is_object());
	HashTable* ht = HASH_OF(ptr());
	return ht ? zend_hash_num_elements(ht) : 0;
}

bool zvalue::has_property(const char* name, std::size_t name_length) const
{
	util::zvalue property{ get_property(name, name_length) };
	return !property.is_undef();
}

zvalue zvalue::get_property(const char* name, std::size_t name_length) const
{
	assert(is_object());
	zval rv;
	ZVAL_UNDEF(&rv);
	return zvalue(zend_read_property(Z_OBJCE(zv), ptr(), name, name_length, true, &rv));
}

zvalue zvalue::require_property(const char* name, std::size_t name_length) const
{
	zvalue property = get_property(name, name_length);
	if (!property.has_value()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_not_exist,
			name);
	}
	return property;
}

void zvalue::set_property(const char* name, std::size_t name_length, const zvalue& value)
{
	assert(is_object());
	zend_update_property(Z_OBJCE(zv), ptr(), name, name_length, value.ptr());
}

void zvalue::unset_property(const char* name, std::size_t name_length)
{
	assert(is_object());
	zend_unset_property(Z_OBJCE(zv), ptr(), name, name_length);
}

// -----------------------------------------------------------------------------

bool zvalue::contains(std::size_t index) const
{
	assert(is_array());
	return zend_hash_index_exists(Z_ARRVAL(zv), index) ? true : false;
}

bool zvalue::contains(const char* key, std::size_t key_length) const
{
	assert(is_array());
	return zend_hash_str_exists(Z_ARRVAL(zv), key, key_length) ? true : false;
}

// ---------------------

zvalue zvalue::find(std::size_t index) const
{
	assert(is_array());
	zval* elem{ zend_hash_index_find(Z_ARRVAL(ref()), index) };
	return elem ? zvalue(*elem) : zvalue();
}

zvalue zvalue::find(const char* key, std::size_t key_length) const
{
	assert(is_array());
	zval* elem{ zend_hash_str_find(Z_ARRVAL(ref()), key, key_length) };
	return elem ? zvalue(*elem) : zvalue();
}

zvalue zvalue::at(std::size_t index) const
{
	assert(is_array());
	zval* elem{ zend_hash_index_find(Z_ARRVAL(ref()), index) };
	if (!elem) {
		util::ostringstream os;
		os << "index " << index <<  " not found";
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::out_of_range,
			os.str());
	}
	return zvalue(*elem);
}

zvalue zvalue::at(const char* key, std::size_t key_length) const
{
	assert(is_array());
	zval* elem{ zend_hash_str_find(Z_ARRVAL(ref()), key, key_length) };
	if (!elem) {
		util::ostringstream os;
		os << "key " << key <<  " not found";
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::out_of_range,
			os.str());
	}
	return zvalue(*elem);
}

// -----------------------------------------------------------------------------

void zvalue::insert(std::size_t index, const zvalue& value)
{
	assert(is_array());
	if (zend_hash_index_update(Z_ARRVAL(zv), static_cast<zend_ulong>(index), value.ptr())) {
		value.inc_ref();
	}
}

void zvalue::insert(std::size_t index, zvalue&& value)
{
	assert(is_array());
	if (zend_hash_index_update(Z_ARRVAL(zv), static_cast<zend_ulong>(index), value.ptr())) {
		value.invalidate();
	}
}

void zvalue::insert(const char* key, std::size_t key_length, const zvalue& value)
{
	assert(is_array());
	if (zend_hash_str_update(Z_ARRVAL(zv), key, key_length, value.ptr())) {
		value.inc_ref();
	}
}

void zvalue::insert(const char* key, std::size_t key_length, zvalue&& value)
{
	assert(is_array());
	if (zend_hash_str_update(Z_ARRVAL(zv), key, key_length, value.ptr())) {
		value.invalidate();
	}
}

// ---------------------

void zvalue::push_back(const zvalue& value)
{
	assert(is_array());
	if (zend_hash_next_index_insert(Z_ARRVAL(zv), value.ptr())) {
		value.inc_ref();
	}
}

void zvalue::push_back(zvalue&& value)
{
	assert(is_array());
	if (zend_hash_next_index_insert(Z_ARRVAL(zv), value.ptr())) {
		value.invalidate();
	}
}

// ---------------------

bool zvalue::erase(std::size_t index)
{
	assert(is_array());
	return zend_hash_index_del(Z_ARRVAL(zv), static_cast<zend_ulong>(index)) == SUCCESS;
}

bool zvalue::erase(const char* key, std::size_t key_length)
{
	assert(is_array());
	return zend_hash_str_del(Z_ARRVAL(zv), key, key_length) == SUCCESS;
}

// -----------------------------------------------------------------------------

zvalue::iterator::iterator(HashTable* ht, HashPosition size, HashPosition pos)
	: ht(ht)
	, size(size)
	, pos(pos)
{
}

zvalue::iterator zvalue::iterator::operator++(int)
{
	iterator it(ht, size, pos);
	++(*this);
	return it;
}

zvalue::iterator& zvalue::iterator::operator++()
{
	if ((zend_hash_move_forward_ex(ht, &pos) == FAILURE) || (pos >= size)) {
		pos = HT_INVALID_IDX;
	}
	return *this;
}

zvalue::iterator::value_type zvalue::iterator::operator*() const
{
	zvalue key;
	zend_hash_get_current_key_zval_ex(ht, key.ptr(), &pos);
	assert(!key.is_null());
	zvalue value(zend_hash_get_current_data_ex(ht, &pos));
	return std::make_pair(key, value);
}

bool zvalue::iterator::operator==(const iterator& rhs) const
{
	return pos == rhs.pos;
}

bool zvalue::iterator::operator!=(const iterator& rhs) const
{
	return pos != rhs.pos;
}

// ---------------------

zvalue::iterator zvalue::begin() const
{
	assert(is_array() || is_object());
	HashTable* ht{ HASH_OF(ptr()) };
	HashPosition pos{ HT_INVALID_IDX };
	std::size_t count = size();
	if (count) {
		zend_hash_internal_pointer_reset_ex(ht, &pos);
	}
	return iterator(ht, static_cast<HashPosition>(count), pos);
}

zvalue::iterator zvalue::end() const
{
	assert(is_array() || is_object());
	return iterator(HASH_OF(ptr()), static_cast<HashPosition>(size()), HT_INVALID_IDX);
}

// -----------------------------------------------------------------------------

zvalue::key_iterator::key_iterator(HashTable* ht, HashPosition size, HashPosition pos)
	: ht(ht)
	, size(size)
	, pos(pos)
{
}

zvalue::key_iterator zvalue::key_iterator::operator++(int)
{
	key_iterator it(ht, size, pos);
	++(*this);
	return it;
}

zvalue::key_iterator& zvalue::key_iterator::operator++()
{
	if ((zend_hash_move_forward_ex(ht, &pos) == FAILURE) || (pos >= size)) {
		pos = HT_INVALID_IDX;
	}
	return *this;
}

zvalue::key_iterator::key_type zvalue::key_iterator::operator*() const
{
	zvalue key;
	zend_hash_get_current_key_zval_ex(ht, key.ptr(), &pos);
	assert(!key.is_null());
	return key;
}

bool zvalue::key_iterator::operator==(const key_iterator& rhs) const
{
	return pos == rhs.pos;
}

bool zvalue::key_iterator::operator!=(const key_iterator& rhs) const
{
	return pos != rhs.pos;
}

// ---------------------

zvalue::key_iterator zvalue::kbegin() const
{
	assert(is_array() || is_object());
	HashTable* ht{ HASH_OF(ptr()) };
	HashPosition pos{ HT_INVALID_IDX };
	std::size_t count = size();
	if (count) {
		zend_hash_internal_pointer_reset_ex(ht, &pos);
	}
	return key_iterator(ht, static_cast<HashPosition>(count), pos);
}

zvalue::key_iterator zvalue::kend() const
{
	assert(is_array() || is_object());
	return key_iterator(HASH_OF(ptr()), static_cast<HashPosition>(size()), HT_INVALID_IDX);
}

// -----------------------------------------------------------------------------

zvalue::value_iterator::value_iterator(HashTable* ht, HashPosition size, HashPosition pos)
	: ht(ht)
	, size(size)
	, pos(pos)
{
}

zvalue::value_iterator zvalue::value_iterator::operator++(int)
{
	value_iterator it(ht, size, pos);
	++(*this);
	return it;
}

zvalue::value_iterator& zvalue::value_iterator::operator++()
{
	if ((zend_hash_move_forward_ex(ht, &pos) == FAILURE) || (pos >= size)) {
		pos = HT_INVALID_IDX;
	}
	return *this;
}

zvalue::value_iterator::value_type zvalue::value_iterator::operator*() const
{
	return zend_hash_get_current_data_ex(ht, &pos);
}

bool zvalue::value_iterator::operator==(const value_iterator& rhs) const
{
	return pos == rhs.pos;
}

bool zvalue::value_iterator::operator!=(const value_iterator& rhs) const
{
	return pos != rhs.pos;
}

// ---------------------

zvalue::value_iterator zvalue::vbegin() const
{
	assert(is_array() || is_object());
	HashTable* ht{ HASH_OF(ptr()) };
	HashPosition pos{ HT_INVALID_IDX };
	std::size_t count = size();
	if (count) {
		zend_hash_internal_pointer_reset_ex(ht, &pos);
	}
	return value_iterator(ht, static_cast<HashPosition>(count), pos);
}

zvalue::value_iterator zvalue::vend() const
{
	assert(is_array() || is_object());
	return value_iterator(HASH_OF(ptr()), static_cast<HashPosition>(size()), HT_INVALID_IDX);
}

// -----------------------------------------------------------------------------

namespace {

class Serializer
{
public:
	Serializer(util::ostringstream& os);

public:
	void store(const zvalue& zv);

private:
	void store_composite(const zvalue& zv, const char* opening_tag, const char* closing_tag);

private:
	util::ostringstream& os;
};

Serializer::Serializer(util::ostringstream& os)
	: os(os)
{
}

void Serializer::store(const zvalue& zv)
{
	switch (zv.type()) {
	case zvalue::Type::Undefined:
		os << "undefined";
		break;

	case zvalue::Type::Null:
		os << "null";
		break;

	case zvalue::Type::False:
		os << "false";
		break;

	case zvalue::Type::True:
		os << "true";
		break;

	case zvalue::Type::Long:
		os << zv.to_long();
		break;

	case zvalue::Type::Double:
		os << zv.to_double();
		break;

	case zvalue::Type::String:
		os << "'" << zv.to_string() << "'";
		break;

	case zvalue::Type::Array:
		store_composite(zv, "[", "]");
		break;

	case zvalue::Type::Object:
		store_composite(zv, "{", "}");
		break;

	default:
		assert(!"unknown type!");
	}
}

void Serializer::store_composite(const zvalue& zv, const char* opening_tag, const char* closing_tag)
{
	assert(zv.is_array() || zv.is_object());
	os << opening_tag;
	bool first_elem = true;
	for (const auto& elem : zv) {
		if (first_elem) {
			first_elem = false;
		} else {
			os << ", ";
		}
		store(elem.first);
		os << ": ";
		store(elem.second);
	}
	os << closing_tag;
}

} // anonymous namespace

util::string zvalue::serialize() const
{
	util::ostringstream os;
	Serializer serializer(os);
	serializer.store(*this);
	return os.str();
}

} // namespace mysqlx::util
