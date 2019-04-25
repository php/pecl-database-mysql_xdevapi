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
*/

#ifndef CDK_FOUNDATION_CONNECTION_TCPIP_H
#define CDK_FOUNDATION_CONNECTION_TCPIP_H

#include "async.h"
#include "stream.h"
#include "opaque_impl.h"
#include "error.h"


namespace cdk {
namespace foundation {
namespace connection {


class TCPIP;
class TLS;


/*
  Defining cdkio error category
  =======================
  See cdk/foundation/error_category.h for more information.
*/

#define EC_io_ERRORS(X) \
  CDK_ERROR(X, EOS, 1, \
            "Cannot perform this action because input stream is in EOS state") \
  CDK_ERROR(X, TIMEOUT, 2, \
            "Operation could not complete before deadline specified " \
            "when it was created") \
  CDK_ERROR(X, NO_CONNECTION, 3, \
            "Attempt to create i/o operation for a connection object that " \
            "was not yet connected or for which connection attempt has failed")

CDK_ERROR_CATEGORY(io, io_errc)


class IO_error : public Error_class<IO_error>
{
public:

  IO_error(int num)
    : Error_base(NULL, io_error(num))
  {}

  virtual ~IO_error() throw() {}

};


class Error_eos : public IO_error
{
public:
  Error_eos() : IO_error(io_errc::EOS) {}
};


class Error_timeout : public IO_error
{
public:
  Error_timeout() : IO_error(io_errc::TIMEOUT) {}
};


class Error_no_connection : public IO_error
{
public:
  Error_no_connection() : IO_error(io_errc::NO_CONNECTION) {}
};


class TCPIP_base
  : public Connection_class<TCPIP_base>
{
public:

  class Impl;
  class IO_op;

  // Connection interface

  virtual void connect();
  virtual void close();
  virtual bool is_closed() const;
  virtual unsigned int get_fd() const;

  // Input stream

  bool eos() const;
  bool has_bytes() const;

  // Output stream

  bool is_ended() const;
  bool has_space() const;
  void flush();

protected:

  virtual Impl& get_base_impl() =0;
  const Impl& get_base_impl() const
  {
    return const_cast<TCPIP_base*>(this)->get_base_impl();
  }

  friend class IO_op;
  friend class TCPIP;
  friend class TLS;
};


class TCPIP
  : public TCPIP_base
  , opaque_impl<TCPIP>
{
public:
  class Read_op;
  class Read_some_op;
  class Write_op;
  class Write_some_op;

  TCPIP(const std::string& host, unsigned short port);

private:

  TCPIP_base::Impl& get_base_impl();
};


class TCPIP_base::IO_op : public Base::IO_op
{
protected:

  typedef TCPIP_base::Impl Impl;

  IO_op(TCPIP_base &str, const buffers &bufs, time_t deadline =0)
    :  Base::IO_op(str, bufs, deadline)
  {}

  // Async_op interface

  // is_completed() is implemented in Base::IO_op
  // bool is_completed() const;
  virtual bool do_cont() = 0;
  virtual void do_cancel();
  virtual void do_wait() = 0;

  const api::Event_info* get_event_info() const { return  NULL; }
};


class TCPIP::Read_op : public IO_op
{
public:
  Read_op(TCPIP &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  unsigned int m_currentBufferIdx;
  size_t m_currentBufferOffset;
};


class TCPIP::Read_some_op : public IO_op
{
public:
  Read_some_op(TCPIP &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  void common_read(bool wait);
};


class TCPIP::Write_op : public IO_op
{
public:
  Write_op(TCPIP &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  unsigned int m_currentBufferIdx;
  size_t m_currentBufferOffset;
};


class TCPIP::Write_some_op : public IO_op
{
public:
  Write_some_op(TCPIP &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  void common_write(bool wait);
};


}}}  // namespace cdk::foundation::connection

#endif
