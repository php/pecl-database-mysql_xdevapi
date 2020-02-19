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

#ifndef CDK_FOUNDATION_SOCKET_H
#define CDK_FOUNDATION_SOCKET_H

/*
  TCP/IP socket which can accept connections from other hosts.

  Usage
  =====

  ::

    Socket sock(port);

    Socket::Connection conn(sock);
    conn.wait();

  Socket::Connection instance is an asynchronous object which is completed
  when a connection is accepted. After that, the Socket::Connection instance
  conn can be used like any other connection object. For example, one can
  read some bytes sent by the peer like this::

    Socket::Connection::Read_some_op  read(conn, buf);
    read.wait();
*/


#include "connection_tcpip.h"
#include "opaque_impl.h"


namespace cdk {
namespace foundation {


class Socket
{
  unsigned short m_port;

public:

  class Connection;

  Socket(unsigned short port)
    : m_port(port)
  {}
};


class Socket::Connection
  : public connection::TCPIP
  , public api::Async_op<void>
  , opaque_impl<Socket::Connection>
{
public:

  Connection(const Socket&);

private:

  typedef TCPIP::Impl  Impl;

  Impl& get_base_impl();

  void do_wait();
  bool is_completed() const;

  bool do_cont()
  {
    wait();
    return true;
  }

  void do_cancel()
  { THROW("Not implemented"); }

  api::Event_info* get_event_info() const
  { return NULL; }
};


}}  // cdk::foundation

#endif
