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
*/

#ifndef MYSQL_CDK_FOUNDATION_DIAGNOSTICS_H
#define MYSQL_CDK_FOUNDATION_DIAGNOSTICS_H

#include "error.h"
#include "types.h"

PUSH_SYS_WARNINGS_CDK
#include <vector>
#include <map>
POP_SYS_WARNINGS_CDK

namespace cdk {
namespace foundation {
namespace api {

struct Severity
{
  enum value {
    INFO=0,
    WARNING=1,
    ERROR=2
  };
};


class Diagnostics
{
public:

  typedef cdk::foundation::api::Severity Severity;

  class Entry;
  class Iterator;

  // Return number of diagnostic entries with given error level (defaults to ERROR).
  virtual unsigned int entry_count(Severity::value level=Severity::ERROR) = 0;

  // Get an iterator to iterate over diagnostic entries with level above or equal to given one
  // (for example, if level is WARNING then iterates over all warnings and errors).
  // By default returns iterator over errors only. The Error_iterator interface extends
  // Iterator interface with single Error_iterator::error() method that returns the current error entry from the sequence.
  virtual Iterator& get_entries(Severity::value level=Severity::ERROR) = 0;

  // Convenience method to return first error entry (if any).
  // Equivalent to get_erros().error(). Note that this method can throw exception if there is no error available.
  virtual const Error& get_error() = 0;

};


/*
  Diagnostic entry extends Error class providing information about severity
  (ERROR, WARNING, INFO etc).
*/

class Diagnostics::Entry : public Error
{
  const Severity::value  m_severity;
  const Error *m_error;

public:

  /*
    Create diagnostic entry for given error. The error instance passed to
    this constructor is owned by this entry and will be deleted when entry
    is destroyed. If this is not desired, clone the error before passing it
    to this constructor.
  */

  Entry(Severity::value level, const Error *e)
    : Error(e->code())
    , m_severity(level)
    , m_error(e)
  {
    m_what_prefix= "CDK ";
  }

  /*
    When copying diagnostic entries we need to clone the error since it
    can not be owned by original entry and the copy.
  */

  Entry(const Entry &e)
    : Error(e.code())
    , m_severity(e.m_severity)
    , m_error(e.m_error->clone())
  {
    m_what_prefix= "CDK ";
  }

  ~Entry() throw ()
  {
    delete m_error;
  }

  Severity::value severity() const { return m_severity; }
  const Error& get_error() const { return *m_error; }

private:

  void do_describe(std::ostream&) const;
};

inline
void Diagnostics::Entry::do_describe(std::ostream &out) const
{
  switch (m_severity)
  {
  case   Severity::ERROR: out <<"Error";   break;
  case Severity::WARNING: out <<"Warning"; break;
  case    Severity::INFO: out <<"Info";    break;
  }
  out <<": " <<*m_error;
}


/*
  Interface to be implemented by iterators over diagnostic entries.
*/

class Diagnostics::Iterator : public foundation::Iterator
{
public:

  typedef Diagnostics::Entry Entry;

  virtual const Entry& entry() = 0;
};



}}} //cdk::foundation::api



namespace cdk {
namespace foundation {


/*
  Base implementation of Diagnostics interface and the iterator required
  by it.
*/


class Diagnostic_arena;

/*
  Implementation of Diagnostics::Iterator interface that iterates over
  diagnostic entries of given severity stored inside std::vector.

  This implementation defines copy assignment, so that the following
  code works as expected:

    Diagnostic_arena da, da1;
    Diganostic_iterator it = da.get_entries();

    it= da1.get_entries();

  The assignment replaces iterator with new one.
*/

class Diagnostic_iterator : public api::Diagnostics::Iterator
{
  typedef api::Severity Severity;
  typedef api::Diagnostics::Entry Entry;
  typedef std::vector<Entry*> Entry_list;

  const Entry_list *m_entries;
  Entry_list::const_iterator  m_it;
  bool                        m_at_begin;
  Severity::value m_level;

  Diagnostic_iterator(const Entry_list &entries, Severity::value level)
    : m_entries(&entries), m_level(level)
  {
    reset(m_level);
  }

public:

  Diagnostic_iterator& operator=(const Diagnostic_iterator &other)
  {
    m_entries= other.m_entries;
    m_at_begin= other.m_at_begin;
    if (!m_at_begin)
      m_it= other.m_it;
    m_level= other.m_level;
    return *this;
  }

  Diagnostic_iterator()
    : m_entries(NULL), m_level(Severity::ERROR)
  {}

  virtual ~Diagnostic_iterator() {}

  const Entry& entry()
  {
    return *(*m_it);
  }

  void reset(Severity::value level);

  bool next();

  friend class Diagnostic_arena;
};


/*
  Implementation of Diagnostics interface which uses a vector of
  pointers to dynamically allocated entries.

  Note: it is not possible to create STL container with diagnostic
  entries because they inherit from Error class which is not
  copy-assignable as required by STL containers.
*/

class Diagnostic_arena
  : public api::Diagnostics
{
  typedef Diagnostic_iterator::Entry_list Entry_list;
  typedef api::Severity Severity;

  Entry_list m_entries;
  std::map<Severity::value, unsigned int> m_counts;
  Diagnostic_iterator m_it;

public:

  typedef Diagnostic_iterator Iterator;

  Diagnostic_arena()
    : m_it(m_entries, Severity::ERROR)
  {}

  virtual ~Diagnostic_arena()
  { clear(); }


  /*
    Add diagnostic entry to the arena. Given error is owned by
    the arena and will be deleted when arena is destroyed.
  */

  void add_entry(Severity::value level, const Error *e)
  {
    m_entries.push_back(new Entry(level, e));
    m_counts[level]++;
  }

  void clear();

  unsigned int entry_count(Severity::value level=Severity::ERROR)
  {
    return m_counts[level];
  }

  Diagnostic_iterator& get_entries(Severity::value level=Severity::ERROR)
  {
    m_it.reset(level);
    return m_it;
  }

  Entry* get_entry(unsigned int pos)
  {
    return m_entries.at(pos);
  }

  const Error& get_error();

  friend class Diagnostic_iterator;
};


}} // cdk::foundation

#endif // MYSQL_CDK_FOUNDATION_DIAGNOSTICS_H

