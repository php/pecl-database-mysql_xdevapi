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

#ifndef CDK_CURSOR_H
#define CDK_CURSOR_H

#include "common.h"
#include "api/cursor.h"
#include "api/mdata.h"
#include "reply.h"
#include "mysqlx.h"
//#include "foundation/codec.h"


namespace cdk {


class Cursor
  : public api::Cursor<Traits>
  , public api::Meta_data<Traits>
{

  mysqlx::Cursor m_impl;

public:

  Cursor(Reply &r) : m_impl(r.m_impl)
  {}

  // Cursor interface

  void get_rows(Row_processor& rp)
  { m_impl.get_rows(rp); }
  void get_rows(Row_processor& rp, row_count_t limit)
  { m_impl.get_rows(rp, limit); }
  bool get_row(Row_processor& rp) { return m_impl.get_row(rp); }
  void close()
  {
    m_impl.close();
  }

  // Meta_data interface

  col_count_t col_count() const
  { return m_impl.col_count(); }
  Type_info   type(col_count_t pos)     { return m_impl.type(pos); }
  Format_info format(col_count_t pos)   { return m_impl.format(pos); }
  Column_info col_info(col_count_t pos) { return m_impl.col_info(pos); }

  // Async_op interface

  bool is_completed() const { return m_impl.is_completed(); }
  const cdk::api::Event_info* get_event_info() const
  { return m_impl.get_event_info(); }

private:

  // Async_op

  bool do_cont() { return m_impl.cont(); }
  void do_wait() { return m_impl.wait(); }
  void do_cancel() { return m_impl.cancel(); }

};

}

#endif
