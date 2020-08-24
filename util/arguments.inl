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

namespace mysqlx::util {

inline arg_string::arg_string(const char* cstr)
	: str(cstr)
	, len(std::strlen(cstr))
{
}

inline bool arg_string::empty() const
{
	return (str == nullptr) || (*str == '\0');
}

inline string_view arg_string::to_view() const
{
	return string_view(str, len);
}

inline std::string_view arg_string::to_std_view() const
{
	return std::string_view(str, len);
}

inline string arg_string::to_string() const
{
	return string(str, len);
}

inline std::string arg_string::to_std_string() const
{
	return std::string(str, len);
}

inline const char* arg_string::c_str() const
{
	return str;
}

inline const char* arg_string::data() const
{
	return str;
}

inline size_t arg_string::length() const
{
	return len;
}

inline size_t arg_string::size() const
{
	return len;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

inline arg_zvals::arg_zvals(raw_zval* data, int counter)
	: data(data)
	, counter(counter)
{
}

inline bool arg_zvals::empty() const
{
	return (data == nullptr) || (counter == 0);
}

inline std::size_t arg_zvals::size() const
{
	return static_cast<std::size_t>(counter);
}

inline arg_zvals::iterator arg_zvals::begin() const
{
	return iterator(data);
}

inline arg_zvals::iterator arg_zvals::end() const
{
	return iterator(data + counter);
}

} // namespace mysqlx::util
