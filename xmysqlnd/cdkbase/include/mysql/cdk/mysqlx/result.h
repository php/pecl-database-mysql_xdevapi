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

#ifndef CDK_MYSQLX_RESULT_H
#define CDK_MYSQLX_RESULT_H

#include "common.h"
#include "session.h"


namespace cdk {
namespace mysqlx {


class Cursor;

typedef Session Reply_init;

class Reply
    : public Async_op
{

  friend class Session;
  friend class Cursor;

protected:

  mysqlx::Session* m_session;
  Diagnostic_arena m_da;
  std::vector<std::string> m_generated_ids;
  bool             m_error;

  Session& get_session()
  {
    if (!m_session)
      throw_error("Accessing session of empty result");
    return *m_session;
  }

public:

  Reply()
    : m_session(NULL)
    , m_error(false)
  {}

  Reply(Reply_init& _init)
  {
    init(_init);
  }

  Reply& operator=(Reply_init& _init);

  ~Reply();


  unsigned int entry_count(Severity::value level=Severity::ERROR)
  { return m_da.entry_count(level); }

  Diagnostic_iterator& get_entries(Severity::value level=Severity::ERROR)
  { return m_da.get_entries(level); }

  const Error& get_error()
  { return m_da.get_error(); }

  void error()
  {
    m_error = true;
  }


  /*
      Async (cdk::api::Async_op)
  */

  bool is_completed() const;
  const cdk::api::Event_info* get_event_info() const;

  /*
        Reply Interface
  */

  virtual bool has_results();

  virtual void skip_result();

  virtual row_count_t affected_rows()
  {
    if (!m_session || has_results() || !m_session->m_executed)
      throw_error("Only available after end of query execute");
    return m_session->m_stmt_stats.rows_affected;
  }

  row_count_t last_insert_id()
  {
    if (!m_session || has_results() || !m_session->m_executed)
      throw_error("Only available after end of query execute");
    return m_session->m_stmt_stats.last_insert_id;
  }

  const std::vector<std::string>& generated_ids() const
  {
    return m_generated_ids;
  }

  virtual void discard();

protected:

  void close_cursor();

private:

  //  Initialize class instance from Reply_init. Used on operator=()

  void init(Reply_init &init);

  /*
      Async (cdk::api::Async_op)
  */

  bool do_cont();

  void do_wait();

  void do_cancel();

};


class Cursor
    : public Async_op
    , private protocol::mysqlx::Row_processor
{

protected:

  Session& m_session;
  bool     m_closed;

  Proto_op*               m_rows_op;
  mysqlx::Row_processor*  m_row_prc;

  row_count_t  m_rows_limit;

  bool m_limited;
  bool m_more_rows;


public:

  Cursor(Reply &reply);
  ~Cursor();

  void get_rows(mysqlx::Row_processor& rp);
  void get_rows(mysqlx::Row_processor& rp, row_count_t limit);
  bool get_row(mysqlx::Row_processor& rp);

  void close();


  /*
      Metadata Interface
  */

  col_count_t col_count() const
  {
    size_t cnt = m_metadata->size();
    assert(cnt <= std::numeric_limits<col_count_t>::max());
    return (col_count_t)cnt;
  }

  // Information about type and encoding format of a column

  Type_info type(col_count_t pos) const;
  const Format_info& format(col_count_t pos) const;

  //  Give other information about the column (if any).

  const Column_info& col_info(col_count_t pos) const
  {
    return get_metadata(pos);
  }


  /*
      Async (cdk::api::Async_op)
  */

  bool is_completed() const;
  const cdk::api::Event_info* get_event_info() const;

private:

  cdk::scoped_ptr<Mdata_storage> m_metadata;

  const Col_metadata& get_metadata(col_count_t pos) const;
  void internal_get_rows(mysqlx::Row_processor& rp);

  /*
      Async (cdk::api::Async_op)
  */
  bool do_cont();
  void do_wait();
  void do_cancel();


  /*
     Row_processor (cdk::protocol::mysqlx::Row_processor)
  */

  bool   row_begin(row_count_t row);
  void   row_end(row_count_t row);
  void   col_null(col_count_t pos);
  void   col_unknown(col_count_t pos, int fmt);
  size_t col_begin(col_count_t pos, size_t data_len);
  size_t col_data(col_count_t pos, bytes data);
  void   col_end(col_count_t pos, size_t data_len);
  void   done(bool eod, bool more);
  bool message_end();

  void error(unsigned int code, short int severity,
                     sql_state_t sql_state, const string &msg)
  {
    m_more_rows = false;
    m_session.error(code, severity, sql_state, msg);
  }

  void notice(unsigned int type, short int scope, bytes payload)
  {
    m_session.notice(type, scope, payload);
  }

};


}} //cdk::mysqlx



#endif // CDK_MYSQLX_SESSION_H
