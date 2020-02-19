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

#ifndef CDK_API_CURSOR_H
#define CDK_API_CURSOR_H

#include "processors.h"


namespace cdk {
namespace api {

template <class Traits>
class Cursor : public Async_op<void>
{
public:

  typedef typename Traits::row_count_t   row_count_t;
  typedef cdk::api::Row_processor<Traits> Row_processor;


  /*
     Fetch given amount of rows from the cursor and pass them to a row processor,
     one-by-one. This method returns immediately after starting an asynchronous
     operation that is controlled using methods from Async_op interface.
  */
  virtual void get_rows(Row_processor& rp) = 0;
  virtual void get_rows(Row_processor& rp, row_count_t limit) = 0;


  /*
     Convenience method that calls get_rows(rp, 1) to fetch a single row, then
     waits for this operation to complete and then returns true if a row was
     fetched or false if there are no more rows in the cursor.
  */
  virtual bool get_row(Row_processor& rp) = 0;


  /*
     Close cursor and free all resources before it is destroyed. Using the
     cursor after close() throws an error.
  */
  virtual void close() = 0;


  //  TODO: Add seek()/rewind() methods when implemented.

#if 0
  /*
     Method seek() changes current position within the cursor. Convenience
     method rewind() is equivalent to seek(BEGIN). If current position of the
     cursor can not be changed then these methods should throw error. Possible
     starting positions for seek() are: BEGIN, END and CURRENT. Possible
     directions are: BACK and FORWARD.
  */

  enum from
  {
    BEGIN,
    END,
    CURRENT
  };

  enum direction
  {
    BACK,
    FORWARD
  };


  virtual void rewind() = 0;
  virtual void seek(enum from, row_count_t count=0,
                    enum direction=FORWARD) = 0;
#endif

};

}} // cdk::api

#endif
