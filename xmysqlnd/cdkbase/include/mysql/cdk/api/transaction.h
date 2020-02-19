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

#ifndef CDK_API_TRANSACTION_H
#define CDK_API_TRANSACTION_H

#include "mysql/cdk/foundation.h"


namespace cdk {
namespace api {

template <class Traits>
class Transaction
    : public Diagnostics
{

public:

  typedef typename Traits::transaction_id_t transaction_id_t;
  typedef typename Traits::savepoint_id_t   savepoint_id_t;

  virtual transaction_id_t  commit() = 0;

  /*
    Rollback transaction to the given savepoint. Default Savepoint id
    (savepoint_id_t()) means beginning of the transaction.
  */
  virtual void rollback(savepoint_id_t id) = 0;

  /*
    TODO:
    Returns true if there are any data modification requests collected in
    the transaction.
  */
  //virtual bool has_changes() = 0;

  /*
    Create a savepoint with given id. If a savepoint with the same id was
    created earlier in the same transaction, then it is replaced by the new one.
    It is an error to create savepoint with id 0, which is reserved for
    the beginning of the current transaction.
  */
  virtual void savepoint_set(savepoint_id_t id) = 0;

  /*
    Remove a savepoint with given id.
  */
  virtual void savepoint_remove(savepoint_id_t id) = 0;

};


}} // cdk::api

#endif
