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

#ifndef CDK_REPLY_H
#define CDK_REPLY_H

#include "common.h"
#include "api/reply.h"
#include "mysqlx.h"


namespace cdk {

class Session;
class Cursor;

class Reply
  : public api::Reply<Traits>
{
protected:

  mysqlx::Reply m_impl;
  typedef mysqlx::Reply_init& Initializer;

public:

  Reply()
  {}

  Reply(Initializer _init)
  {
    m_impl= _init;
  }

  Reply& operator=(Initializer _init)
  {
    m_impl= _init;
    return *this;
  }

  // Reply interface

  bool has_results() { return m_impl.has_results(); }
  void skip_result() { m_impl.skip_result(); }
  row_count_t affected_rows() { return m_impl.affected_rows(); }
  row_count_t last_insert_id() { return m_impl.last_insert_id(); }
  const std::vector<std::string>& generated_ids() const
  { return m_impl.generated_ids(); }
  void discard() { m_impl.discard(); }

  // Diagnostics interface

  unsigned int entry_count(Severity::value level=Severity::ERROR)
  { return m_impl.entry_count(level); }

  Diagnostic_iterator& get_entries(Severity::value level=Severity::ERROR)
  { return m_impl.get_entries(level); }

  const Error& get_error()
  { return m_impl.get_error(); }

  // Async_op interface

  bool is_completed() const { return m_impl.is_completed(); }

private:

  // Async_op

  bool do_cont() { return m_impl.cont(); }
  void do_wait() { return m_impl.wait(); }
  void do_cancel() { return m_impl.cancel(); }
  const cdk::api::Event_info* get_event_info() const
  { return m_impl.get_event_info(); }


  friend class Session;
  friend class Cursor;
};

}

#endif
