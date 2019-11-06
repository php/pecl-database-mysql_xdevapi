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

#ifndef CDK_FOUNDATION_CONNECTION_OPENSSL_H
#define CDK_FOUNDATION_CONNECTION_OPENSSL_H

#include "connection_tcpip.h"
#include "stream.h"
#include "error.h"

#include <functional>

namespace cdk {
namespace foundation {
namespace connection {


class TLS
  : public Socket_base
  , opaque_impl<TLS>
{
public:

  class Options;

  TLS(Socket_base* tcpip,
      const Options& Opts);

  bool is_secure() const
  {
    return true;
  }

  class Read_op;
  class Read_some_op;
  class Write_op;
  class Write_some_op;

private:
  Socket_base::Impl& get_base_impl();
};


class TLS::Options
{
public:

  /*
    Note: Normally m_use_tls should be always true: using TLS options object
    implies an intent to have TLS connection. A TLS::Options object with
    m_use_tls set to false is only used to disable TLS connection inside
    TCPIP::Options object. The TCPIP::Options object holds an instance
    of TLS::Options. Calling TCPIP::Options::set_tls(false) will alter this
    internal TLS::Options instance so that m_use_tls is false and then the
    TCPIP::Options object knows that TLS should not be used for the connection.
  */

  enum class SSL_MODE
  {
    DISABLED,
    PREFERRED,
    REQUIRED,
    VERIFY_CA,
    VERIFY_IDENTITY
  };

  Options(SSL_MODE ssl_mode = SSL_MODE::PREFERRED)
    : m_ssl_mode(ssl_mode)
  {}

  void set_ssl_mode(SSL_MODE ssl_mode) { m_ssl_mode = ssl_mode; }
  SSL_MODE ssl_mode() const { return m_ssl_mode; }

  void set_key(const string &key) { m_key = key; }
  const std::string &get_key() const { return m_key; }

  void set_ca(const string &ca) { m_ca = ca; }
  void set_ca_path(const string &ca_path) { m_ca_path = ca_path; }

  const std::string &get_ca() const { return m_ca; }
  const std::string &get_ca_path() const { return m_ca_path; }
  const std::string &get_host_name() const { return m_host_name; }

  void set_host_name(const std::string &host_name)
  {
    m_host_name = host_name;
  }

protected:

  SSL_MODE m_ssl_mode;
  std::string m_key;
  std::string m_ca;
  std::string m_ca_path;
  std::string m_host_name;

};


class TLS::Read_op : public Socket_base::IO_op
{
public:
  Read_op(TLS &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  TLS& m_tls;
  unsigned int m_currentBufferIdx;
  size_t m_currentBufferOffset;

  bool common_read();
};


class TLS::Read_some_op : public Socket_base::IO_op
{
public:
  Read_some_op(TLS &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  TLS& m_tls;

  bool common_read();
};


class TLS::Write_op : public Socket_base::IO_op
{
public:
  Write_op(TLS &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  TLS& m_tls;
  unsigned int m_currentBufferIdx;
  size_t m_currentBufferOffset;

  bool common_write();
};


class TLS::Write_some_op : public Socket_base::IO_op
{
public:
  Write_some_op(TLS &conn, const buffers &bufs, time_t deadline = 0);

  virtual bool do_cont();
  virtual void do_wait();

private:
  TLS& m_tls;

  bool common_write();
};


} // namespace connection
} // namespace foundation
} // namespace cdk

#endif // CDK_FOUNDATION_CONNECTION_OPENSSL_H
