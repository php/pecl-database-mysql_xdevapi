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
namespace mysqlx {

namespace util {

template<typename T>
zvalue::zvalue(std::initializer_list<T> values)
{
	ZVAL_UNDEF(&zv);
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
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
	reserve(values.size());
	for (const auto& value : values) {
		push_back(value);
	}
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

// -----------------------------------------------------------------------------

template<>
inline bool zvalue::to_value<bool>() const
{
	return to_bool();
}

// ---------------------

template<>
inline long zvalue::to_value<long>() const
{
	return to_long();
}

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
inline unsigned long zvalue::to_value<unsigned long>() const
{
	return to_ulong();
}

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

inline std::size_t zvalue::length() const
{
	return size();
}

inline bool zvalue::empty() const
{
	return size() == 0;
}

inline void zvalue::acquire(zval& zv)
{
	acquire(&zv);
}

inline void zvalue::copy_to(zval& zv)
{
	copy_to(&zv);
}

inline void zvalue::move_to(zval& zv)
{
	move_to(&zv);
}

inline void zvalue::swap(zvalue& rhs) noexcept
{
	std::swap(zv, rhs.zv);
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
	return contains(key.c_str(), key.length());
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

inline const zvalue zvalue::find(long index) const
{
	return find(static_cast<std::size_t>(index));
}

inline const zvalue zvalue::find(const string& key) const
{
	return find(key.c_str(), key.length());
}

inline const zvalue zvalue::find(const string_view& key) const
{
	return find(key.c_str(), key.length());
}

inline const zvalue zvalue::find(const std::string& key) const
{
	return find(key.c_str(), key.length());
}

inline const zvalue zvalue::find(const char* key) const
{
	return find(key, std::strlen(key));
}

// ---------------------

inline const zvalue zvalue::operator[](std::size_t index) const
{
	return find(index);
}

inline const zvalue zvalue::operator[](long index) const
{
	return find(index);
}

inline const zvalue zvalue::operator[](const string_view& key) const
{
	return find(key);
}

inline const zvalue zvalue::operator[](const string& key) const
{
	return find(key);
}

inline const zvalue zvalue::operator[](const std::string& key) const
{
	return find(key);
}

inline const zvalue zvalue::operator[](const char* key) const
{
	return find(key);
}

// ---------------------

inline const zvalue zvalue::at(long index) const
{
	return at(static_cast<std::size_t>(index));
}

inline const zvalue zvalue::at(const string& key) const
{
	return at(key.c_str(), key.length());
}

inline const zvalue zvalue::at(const string_view& key) const
{
	return at(key.c_str(), key.length());
}

inline const zvalue zvalue::at(const std::string& key) const
{
	return at(key.c_str(), key.length());
}

inline const zvalue zvalue::at(const char* key) const
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
	insert(key.c_str(), key.length(), value);
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
	insert(key.c_str(), key.length(), value);
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
	return erase(key.c_str(), key.length());
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

inline zval& zvalue::ref() const
{
	return const_cast<zval&>(zv);
}

inline zval* zvalue::ptr() const
{
	return const_cast<zval*>(&zv);
}

} // namespace util

} // namespace mysqlx
