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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "exceptions.h"

namespace mysqlx::util {

template<typename T>
zvalue::zvalue(std::initializer_list<T> values)
{
	ZVAL_UNDEF(&zv);
	push_back(values);
}

template<typename Iterator>
zvalue::zvalue(Iterator begin, Iterator end)
{
	ZVAL_UNDEF(&zv);
	reserve(std::distance(begin, end));
	for_each(
		begin,
		end,
		[this](const auto& value) { push_back(value); }
		);
}

template<typename T>
zvalue::zvalue(const vector<T>& values)
{
	ZVAL_UNDEF(&zv);
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
}

template<typename T>
zvalue::zvalue(const set<T>& values)
{
	ZVAL_UNDEF(&zv);
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
}

template<typename Key, typename Value>
zvalue::zvalue(const map<Key, Value>& values)
{
	ZVAL_UNDEF(&zv);
	reserve(values.size());
	for_each(
		values.begin(),
		values.end(),
		[this](const auto& key_value) { insert(key_value.first, key_value.second); }
		);
}

// -----------------------------------------------------------------------------

template<typename T>
zvalue& zvalue::operator=(std::initializer_list<T> values)
{
	push_back(values);
	return *this;
}

template<typename T>
zvalue& zvalue::operator=(const vector<T>& values)
{
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
	return *this;
}

template<typename T>
zvalue& zvalue::operator=(const set<T>& values)
{
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
	return *this;
}

template<typename Key, typename Value>
zvalue& zvalue::operator=(const map<Key, Value>& values)
{
	reserve(values.size());
	for (const auto& value : values) {
		insert(value.first, value.second);
	}
	return *this;
}

// -----------------------------------------------------------------------------

inline zvalue::Type zvalue::type() const
{
	return static_cast<Type>(Z_TYPE(zv));
}

inline bool zvalue::is_undef() const
{
	return type() == Type::Undefined;
}

inline bool zvalue::is_null() const
{
	return type() == Type::Null;
}

inline bool zvalue::is_false() const
{
	return type() == Type::False;
}

inline bool zvalue::is_true() const
{
	return type() == Type::True;
}

inline bool zvalue::is_bool() const
{
	const Type t{ type() };
	return (t == Type::False) || (t == Type::True);
}

inline bool zvalue::is_long() const
{
	return type() == Type::Long;
}

inline bool zvalue::is_double() const
{
	return type() == Type::Double;
}

inline bool zvalue::is_string() const
{
	return type() == Type::String;
}

inline bool zvalue::is_array() const
{
	return type() == Type::Array;
}

inline bool zvalue::is_object() const
{
	return type() == Type::Object;
}

inline bool zvalue::is_reference() const
{
	return type() == Type::Reference;
}

inline bool zvalue::has_value() const
{
	switch (type()) {
	case Type::Undefined:
	case Type::Null:
		return false;

	default:
		return true;
	}
}

// -----------------------------------------------------------------------------

template<>
inline bool zvalue::to_value<bool>() const
{
	return to_bool();
}

// ---------------------

template<>
inline int32_t zvalue::to_value<int32_t>() const
{
	return to_int32();
}

template<>
inline int64_t zvalue::to_value<int64_t>() const
{
	return to_int64();
}

// ---------------------

template<>
inline uint32_t zvalue::to_value<uint32_t>() const
{
	return to_uint32();
}

template<>
inline uint64_t zvalue::to_value<uint64_t>() const
{
	return to_uint64();
}

// ---------------------

template<>
inline double zvalue::to_value<double>() const
{
	return to_double();
}

// ---------------------

template<>
inline string zvalue::to_value<string>() const
{
	return to_string();
}

template<>
inline string_view zvalue::to_value<string_view>() const
{
	return to_string_view();
}

template<>
inline std::string zvalue::to_value<std::string>() const
{
	return to_std_string();
}

template<>
inline const char* zvalue::to_value<const char*>() const
{
	return c_str();
}

// -----------------------------------------------------------------------------

template<>
inline std::optional<bool> zvalue::to_optional_value<bool>() const
{
	return is_bool() ? std::make_optional(to_bool()) : std::nullopt;
}

// ---------------------

template<>
inline std::optional<int32_t> zvalue::to_optional_value<int32_t>() const
{
	return is_long() ? std::make_optional(to_int32()) : std::nullopt;
}

template<>
inline std::optional<int64_t> zvalue::to_optional_value<int64_t>() const
{
	return is_long() ? std::make_optional(to_int64()) : std::nullopt;
}

// ---------------------

template<>
inline std::optional<uint32_t> zvalue::to_optional_value<uint32_t>() const
{
	return is_long() ? std::make_optional(to_uint32()) : std::nullopt;
}

template<>
inline std::optional<uint64_t> zvalue::to_optional_value<uint64_t>() const
{
	return is_long() ? std::make_optional(to_uint64()) : std::nullopt;
}

// ---------------------

template<>
inline std::optional<double> zvalue::to_optional_value<double>() const
{
	return is_double() ? std::make_optional(to_double()) : std::nullopt;
}

// ---------------------

template<>
inline std::optional<string> zvalue::to_optional_value<string>() const
{
	return is_string() ? std::make_optional(to_string()) : std::nullopt;
}

template<>
inline std::optional<string_view> zvalue::to_optional_value<string_view>() const
{
	return is_string() ? std::make_optional(to_string_view()) : std::nullopt;
}

template<>
inline std::optional<std::string> zvalue::to_optional_value<std::string>() const
{
	return is_string() ? std::make_optional(to_std_string()) : std::nullopt;
}

template<>
inline std::optional<const char*> zvalue::to_optional_value<const char*>() const
{
	return is_string() ? std::make_optional(c_str()) : std::nullopt;
}

// -----------------------------------------------------------------------------

template<>
inline bool zvalue::to_obligatory_value<bool>() const
{
	if (!is_bool()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"boolean");
	}
	return to_bool();
}

// ---------------------

template<>
inline int32_t zvalue::to_obligatory_value<int32_t>() const
{
	if (!is_long()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"long");
	}
	return to_int32();
}

template<>
inline int64_t zvalue::to_obligatory_value<int64_t>() const
{
	if (!is_long()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"long");
	}
	return to_int64();
}

// ---------------------

template<>
inline uint32_t zvalue::to_obligatory_value<uint32_t>() const
{
	if (!is_long()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"long");
	}
	return to_uint32();
}

template<>
inline uint64_t zvalue::to_obligatory_value<uint64_t>() const
{
	if (!is_long()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"long");
	}
	return to_uint64();
}

// ---------------------

template<>
inline double zvalue::to_obligatory_value<double>() const
{
	if (!is_double()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"double");
	}
	return to_double();
}

// ---------------------

template<>
inline string zvalue::to_obligatory_value<string>() const
{
	if (!is_string()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"string");
	}
	return to_string();
}

template<>
inline string_view zvalue::to_obligatory_value<string_view>() const
{
	if (!is_string()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"string");
	}
	return to_string_view();
}

template<>
inline std::string zvalue::to_obligatory_value<std::string>() const
{
	if (!is_string()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"string");
	}
	return to_std_string();
}

template<>
inline const char* zvalue::to_obligatory_value<const char*>() const
{
	if (!is_string()) {
		throw util::xdevapi_exception(
			util::xdevapi_exception::Code::object_property_invalid_type,
			"string");
	}
	return c_str();
}

// -----------------------------------------------------------------------------

inline std::size_t zvalue::length() const
{
	return size();
}

inline bool zvalue::empty() const
{
	return size() == 0;
}

inline void zvalue::acquire(zval& src)
{
	acquire(&src);
}

inline void zvalue::copy_to(zval& dst)
{
	copy_to(&dst);
}

inline void zvalue::move_to(zval& dst)
{
	move_to(&dst);
}

inline void zvalue::inc_ref() const
{
	Z_TRY_ADDREF(ref());
}

inline void zvalue::dec_ref() const
{
	Z_TRY_DELREF(ref());
}

inline void zvalue::swap(zvalue& rhs) noexcept
{
	std::swap(zv, rhs.zv);
}

// -----------------------------------------------------------------------------

inline bool zvalue::has_property(const string& name) const
{
	return has_property(name.c_str(), name.length());
}

inline bool zvalue::has_property(const string_view& name) const
{
	return has_property(name.data(), name.length());
}

inline bool zvalue::has_property(const std::string& name) const
{
	return has_property(name.c_str(), name.length());
}

inline bool zvalue::has_property(const char* name) const
{
	return has_property(name, std::strlen(name));
}

// -----------------------------------------------------------------------------

inline zvalue zvalue::get_property(const string& name) const
{
	return get_property(name.c_str(), name.length());
}

inline zvalue zvalue::get_property(const string_view& name) const
{
	return get_property(name.data(), name.length());
}

inline zvalue zvalue::get_property(const std::string& name) const
{
	return get_property(name.c_str(), name.length());
}

inline zvalue zvalue::get_property(const char* name) const
{
	return get_property(name, std::strlen(name));
}

// -----------------------------------------------------------------------------

inline zvalue zvalue::require_property(const string& name) const
{
	return require_property(name.c_str(), name.length());
}

inline zvalue zvalue::require_property(const string_view& name) const
{
	return require_property(name.data(), name.length());
}

inline zvalue zvalue::require_property(const std::string& name) const
{
	return require_property(name.c_str(), name.length());
}

inline zvalue zvalue::require_property(const char* name) const
{
	return require_property(name, std::strlen(name));
}

// ---------------------

inline void zvalue::set_property(const string& name, const zvalue& value)
{
	set_property(name.c_str(), name.length(), value);
}

inline void zvalue::set_property(const string_view& name, const zvalue& value)
{
	set_property(name.data(), name.length(), value);
}

inline void zvalue::set_property(const std::string& name, const zvalue& value)
{
	set_property(name.c_str(), name.length(), value);
}

inline void zvalue::set_property(const char* name, const zvalue& value)
{
	set_property(name, std::strlen(name), value);
}

// ---------------------

inline void zvalue::unset_property(const string& name)
{
	return unset_property(name.c_str(), name.length());
}

inline void zvalue::unset_property(const string_view& name)
{
	return unset_property(name.data(), name.length());
}

inline void zvalue::unset_property(const std::string& name)
{
	return unset_property(name.c_str(), name.length());
}

inline void zvalue::unset_property(const char* name)
{
	return unset_property(name, std::strlen(name));
}

// -----------------------------------------------------------------------------

inline bool zvalue::contains(long index) const
{
	return contains(static_cast<std::size_t>(index));
}

inline bool zvalue::contains(const string& key) const
{
	return contains(key.c_str(), key.length());
}

inline bool zvalue::contains(const string_view& key) const
{
	return contains(key.data(), key.length());
}

inline bool zvalue::contains(const std::string& key) const
{
	return contains(key.c_str(), key.length());
}

inline bool zvalue::contains(const char* key) const
{
	return contains(key, std::strlen(key));
}

// ---------------------

inline zvalue zvalue::find(long index) const
{
	return find(static_cast<std::size_t>(index));
}

inline zvalue zvalue::find(const string& key) const
{
	return find(key.c_str(), key.length());
}

inline zvalue zvalue::find(const string_view& key) const
{
	return find(key.data(), key.length());
}

inline zvalue zvalue::find(const std::string& key) const
{
	return find(key.c_str(), key.length());
}

inline zvalue zvalue::find(const char* key) const
{
	return find(key, std::strlen(key));
}

// ---------------------

inline zvalue zvalue::operator[](std::size_t index) const
{
	return find(index);
}

inline zvalue zvalue::operator[](long index) const
{
	return find(index);
}

inline zvalue zvalue::operator[](const string_view& key) const
{
	return find(key);
}

inline zvalue zvalue::operator[](const string& key) const
{
	return find(key);
}

inline zvalue zvalue::operator[](const std::string& key) const
{
	return find(key);
}

inline zvalue zvalue::operator[](const char* key) const
{
	return find(key);
}

// ---------------------

inline zvalue zvalue::at(long index) const
{
	return at(static_cast<std::size_t>(index));
}

inline zvalue zvalue::at(const string& key) const
{
	return at(key.c_str(), key.length());
}

inline zvalue zvalue::at(const string_view& key) const
{
	return at(key.data(), key.length());
}

inline zvalue zvalue::at(const std::string& key) const
{
	return at(key.c_str(), key.length());
}

inline zvalue zvalue::at(const char* key) const
{
	return at(key, std::strlen(key));
}

// ---------------------

inline void zvalue::insert(long index, const zvalue& value)
{
	insert(static_cast<std::size_t>(index), value);
}

inline void zvalue::insert(const string& key, const zvalue& value)
{
	insert(key.c_str(), key.length(), value);
}

inline void zvalue::insert(const string_view& key, const zvalue& value)
{
	insert(key.data(), key.length(), value);
}

inline void zvalue::insert(const std::string& key, const zvalue& value)
{
	insert(key.c_str(), key.length(), value);
}

inline void zvalue::insert(const char* key, const zvalue& value)
{
	insert(key, std::strlen(key), value);
}

// ---------------------

inline void zvalue::insert(long index, zvalue&& value)
{
	insert(static_cast<std::size_t>(index), value);
}

inline void zvalue::insert(const string& key, zvalue&& value)
{
	insert(key.c_str(), key.length(), value);
}

inline void zvalue::insert(const string_view& key, zvalue&& value)
{
	insert(key.data(), key.length(), value);
}

inline void zvalue::insert(const std::string& key, zvalue&& value)
{
	insert(key.c_str(), key.length(), value);
}

inline void zvalue::insert(const char* key, zvalue&& value)
{
	insert(key, std::strlen(key), value);
}

// ---------------------

template<typename Key, typename Value>
void zvalue::insert(const std::pair<Key, Value>& key_value)
{
	insert(key_value.first, key_value.second);
}

inline void zvalue::insert(std::initializer_list<std::pair<const char*, zvalue>> values)
{
	append(values);
}

template<typename Key, typename Value>
void zvalue::append(std::initializer_list<std::pair<Key, Value>> values)
{
	reserve(values.size());
	for_each(
		values.begin(),
		values.end(),
		[this](const auto& key_value) { this->insert(key_value); }
		);
}

// ---------------------

template<typename T>
void zvalue::push_back(std::initializer_list<T> values)
{
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
}

// ---------------------

inline bool zvalue::erase(long index)
{
	return erase(static_cast<std::size_t>(index));
}

inline bool zvalue::erase(const string& key)
{
	return erase(key.c_str(), key.length());
}

inline bool zvalue::erase(const string_view& key)
{
	return erase(key.data(), key.length());
}

inline bool zvalue::erase(const std::string& key)
{
	return erase(key.c_str(), key.length());
}

inline bool zvalue::erase(const char* key)
{
	return erase(key, std::strlen(key));
}

// -----------------------------------------------------------------------------

inline zvalue::keys_range::keys_range(const zvalue& ref) : ref(ref)
{
}

inline zvalue::key_iterator zvalue::keys_range::begin() const
{
	return ref.kbegin();
}

inline zvalue::key_iterator zvalue::keys_range::end() const
{
	return ref.kend();
}

inline zvalue::keys_range zvalue::keys() const
{
	return keys_range(*this);
}

// -----------------------------------------------------------------------------

inline zvalue::values_range::values_range(const zvalue& ref) : ref(ref)
{
}

inline zvalue::value_iterator zvalue::values_range::begin() const
{
	return ref.vbegin();
}

inline zvalue::value_iterator zvalue::values_range::end() const
{
	return ref.vend();
}

inline zvalue::values_range zvalue::values() const
{
	return values_range(*this);
}

// -----------------------------------------------------------------------------

inline const zval& zvalue::c_ref() const
{
	return zv;
}

inline zval& zvalue::ref() const
{
	return const_cast<zval&>(zv);
}

inline const zval* zvalue::c_ptr() const
{
	return &zv;
}

inline zval* zvalue::ptr() const
{
	return const_cast<zval*>(&zv);
}

} // namespace mysqlx::util
