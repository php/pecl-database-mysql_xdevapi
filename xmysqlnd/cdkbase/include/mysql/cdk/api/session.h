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

#ifndef CDK_API_SESSION_H
#define CDK_API_SESSION_H

#include "mysql/cdk/foundation.h"


namespace cdk {
namespace api {


class Session
    : public Diagnostics
    , public Async_op<void>
{
public:

  virtual ~Session() {}

  // Check if given session is valid. Function is_valid() performs a lightweight, local check while
  // check_valid() might communicate with the data store to perform this check.
  // Both is_valid() and check_valid() return UNKNOWN if session state could not be determined.

  virtual option_t is_valid() = 0;
  virtual option_t check_valid() = 0;

  // Clear diagnostic information that accumulated for the session.
  // Diagnostics interface methods such as Diagnostics::error_count()
  // and Diagnostics::get_errors() report only new diagnostics entries
  // since last call to clear_errors() (or since session creation if
  // clear_errors() was not called).
  virtual void clear_errors() = 0;

  virtual void close() = 0;

};

}} // cdk::api

#endif // CDK_API_SESSION_H
