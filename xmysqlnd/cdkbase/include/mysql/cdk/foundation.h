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

#ifndef MYSQL_CDK_FOUNDATION_H
#define MYSQL_CDK_FOUNDATION_H

#include "foundation/common.h"
#include "foundation/types.h"
#include "foundation/error.h"
#include "foundation/async.h"
#include "foundation/stream.h"
#include "foundation/connection_tcpip.h"
#ifdef WITH_SSL
#include "foundation/connection_openssl.h"
#endif
#include "foundation/diagnostics.h"
#include "foundation/codec.h"
//#include "foundation/socket.h"

namespace cdk {

  using foundation::char_t;
  using foundation::invalid_char;
  using foundation::byte;
  using foundation::option_t;
  using foundation::string;
  using foundation::scoped_ptr;
  using foundation::shared_ptr;
  using foundation::variant;
  using foundation::opt;

  using foundation::bytes;
  using foundation::buffers;

  using foundation::Error;
  using foundation::Error_class;
  using foundation::error_condition;
  using foundation::error_category;
  using foundation::error_code;
  using foundation::errc;
  using foundation::cdkerrc;
  using foundation::throw_error;
  using foundation::throw_posix_error;
  using foundation::throw_system_error;
  using foundation::rethrow_error;

  using foundation::Diagnostic_arena;
  using foundation::Diagnostic_iterator;

  namespace api {

    using namespace cdk::foundation::api;

  }  // cdk::api

  namespace connection {

    using foundation::connection::TCPIP;
    using foundation::connection::TLS;
    using foundation::connection::Error_eos;
    using foundation::connection::Error_no_connection;
    using foundation::connection::Error_timeout;

  }

} // cdk


#endif
