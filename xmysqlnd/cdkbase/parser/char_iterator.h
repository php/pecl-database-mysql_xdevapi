/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#ifndef _CHAR_ITERATOR_H_
#define _CHAR_ITERATOR_H_

#include <mysql/cdk.h>

#undef WORD

namespace parser {

  using cdk::byte;
  using cdk::bytes;
  using cdk::char_t;
  //using cdk::throw_error;

  class char_iterator;

  using char_iterator_base
    = cdk::foundation::char_iterator_base<
        cdk::foundation::String_encoding::UTF8
      >;


  /*
    Iterate over characters of an utf8 string examining their properties.
  */

  class char_iterator
    : private char_iterator_base
  {
  public:

    using char_type = char;
    using pos_type = const char*;

    char_iterator(bytes input, pos_type pos)
      : char_iterator_base((const char*)input.begin(), (const char*)input.end())
      , m_ctx_beg((const char*)input.begin())
    {
      set_pos(pos);
    }

    char_iterator(bytes input)
      : char_iterator(input, (const char*)input.begin())
    {}

    char_iterator() = default;
    char_iterator(const char_iterator&) = default;


    // -- Examine current character ------------------------------------------


    char_t cur_char() const noexcept
    {
      return operator*();
    }

    bool cur_char_is(char c) const noexcept
    {
      return c == *(char*)cur_pos();
    }

    bool cur_char_in(const char *set) const noexcept
    {
      assert(!at_end());
      return nullptr != strchr(set, *(char*)cur_pos());
    }

    static std::locale m_cloc;

    bool cur_char_is_space() const noexcept
    {
      static const std::ctype<char> &ctf = std::use_facet<std::ctype<char>>(m_cloc);
      assert(!at_end());
      return ctf.is(ctf.space, *(char*)cur_pos());
    }

    // Return true if current character can be part of a WORD token.

    bool cur_char_is_word() const noexcept
    {
      static const std::ctype<char> &ctf = std::use_facet<std::ctype<char>>(m_cloc);
      assert(!at_end());
      char c = *(char*)cur_pos();
      if ('_' == c)
        return true;
      return ctf.is(ctf.alnum, c);
    }

    /*
      Assuming that characters [m_pos, m_pos+off) are ASCII, return true
      if character at m_pos + off is c.
    */

    bool next_char_is(char c, size_t off = 1) const noexcept
    {
      assert(cur_pos() && cur_pos() + off < get_end());
      return c == *(char*)(cur_pos() + off);
    }

    bool next_char_in(const char *set, size_t off = 1) const noexcept
    {
      assert(cur_pos() && cur_pos() + off < get_end());
      char c = (char)*(cur_pos() + off);

      if (!c)
        return false;

      return nullptr != strchr(set, c);
    }


    // -- Examine current position ------------------------------------------


    using parser::char_iterator_base::cur_pos;
    using parser::char_iterator_base::at_end;

    bool at_end(size_t off) const noexcept
    {
      if (m_at_end)
        return true;
      if (0 == off)
        return false;

      if ((nullptr == cur_pos()) || (cur_pos() + off >= get_end()))
        return true;

      return false;
    }


    // -- Change iterator position -------------------------------------------


    using parser::char_iterator_base::next_unit;

    char_t consume_char()
    {
      char_t c = operator*();
      operator++();
      return c;
    }

    // Consume next character if it equals given one.

    bool  consume_char(char c) noexcept
    {
      assert(!at_end());
      if (c != get_unit())
        return false;
      next_unit();
      return true;
    }

    /*
      Consume next character if it is one of the character in the given
      string. Returns consumed character, '\0' otherwise.
    */

    char_t consume_char(const char *set) noexcept
    {
      if (!cur_char_in(set))
        return 0;
      char_t c = (unsigned)get_unit();
      next_unit();
      return c;
    }

    /*
      Consume given sequence of characters. Returns true if it was possible.
      If not, the position within input string is not changed.
    */

    bool consume_chars(const char *chars) noexcept
    {
      pos_type pos = cur_pos();
      for (; chars && *chars && !at_end() && consume_char(*chars); ++chars);
      if (*chars)
      {
        set_pos(pos);
        return false;
      }
      return true;
    }

    bool skip_ws() noexcept
    {
      bool ret = false;
      for (; !at_end() && cur_char_is_space(); ret=true, next_unit());
      return ret;
    }


    // -- Other --------------------------------------------------------------


    bool operator==(const char_iterator &other) const noexcept
    {
      if (at_end() && other.at_end())
        return true;
      return (cur_pos() == other.cur_pos()) && (get_end() == other.get_end());
    }

    bool operator!=(const char_iterator &other) const noexcept
    {
      return !operator==(other);
    }


    // -- Error reporting ----------------------------------------------------


    pos_type m_ctx_beg = nullptr;

    /*
      Methods below give access to characters before and after current
      iterator's position. They take into account utf8 encoding and always
      return regions of the input string that include complete characters.

      - Get_seen() returns region of characters preceding the current position
        which is not longer than len bytes; if complete is not null it is set
        to true if this region contains all characters from the beginning of
        the parsed string.

      - Get_ahead() returns region of characters in front of the current position (including character at the current position) which is not
        longer than len bytes; if complete is not null it is set to
        true if this region contains all remaining characters from the
        input string.
    */

    bytes  get_seen(size_t len, bool *complete = nullptr);
    bytes  get_ahead(size_t len, bool *complete = nullptr);

  };


}  // parser

#endif
