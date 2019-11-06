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

#ifndef SDK_FOUNDATION_ASYNC_H
#define SDK_FOUNDATION_ASYNC_H

#include "types.h"   // for nocopy
#include <stddef.h>  // for NULL

namespace cdk {
namespace foundation {
namespace api {

class Event_info
{
public:
  enum event_type {OTHER, SOCKET_RD, SOCKET_WR, ASYNC_OP };
  virtual event_type type() const { return OTHER; }
};


class Async_op_base : nocopy
{
public:

  virtual ~Async_op_base() {}

  virtual bool is_completed() const =0;

  virtual const Event_info* waits_for() const
  {
    if (is_completed()) return NULL;
    return get_event_info();
  }

  virtual void cancel()
  {
    if (is_completed()) return;
    do_cancel();
  }

  bool cont()
  {
    if (is_completed()) return true;
    return do_cont();
  }

  void wait()
  {
    if (is_completed()) return;
    do_wait();
  }

private:

  virtual bool do_cont() =0;
  virtual void do_wait() =0;
  virtual void do_cancel() =0;
  virtual const Event_info* get_event_info() const =0;
};



template  <typename T>
class Async_op
  : public Async_op_base
{
public:
  typedef T result_type;

  T get_result()
  {
    wait();
    return do_get_result();
  }

private:

  virtual T do_get_result() =0;
};


template<>
class Async_op<void>
  : public Async_op_base
{
};


}}} // cdk::foundation::api

#endif
