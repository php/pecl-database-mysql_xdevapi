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

#ifndef CDK_MYSQLX_COMMON_H
#define CDK_MYSQLX_COMMON_H

#include <mysql/cdk/common.h>  // Traits
#include <mysql/cdk/api/processors.h>
#include <mysql/cdk/protocol/mysqlx.h>

namespace cdk {
namespace mysqlx {

/*
  Content type values.

  For fields that contain raw BLOBs, server can send additional 'content_type'
  information in column meta-data. This list contains knonw content type
  values.

  See documentation for protobuf Mysqlx.Resultset.ColumnMetaData message.
*/

#define CONTENT_TYPE_LIST(X) \
  X(GEOMETRY, 0x0001) \
  X(JSON,     0x0002) \
  X(XML,      0x0003)

#define CONTENT_TYPE_ENUM(A,B) A = B,

struct content_type
{
  enum value
  {
    UNKNOWN = 0,
    CONTENT_TYPE_LIST(CONTENT_TYPE_ENUM)
  };
};

using foundation::string;
using foundation::byte;
using foundation::bytes;

using protocol::mysqlx::Protocol;
using protocol::mysqlx::sql_state_t;
using protocol::mysqlx::row_count_t;
using protocol::mysqlx::col_count_t;
using protocol::mysqlx::collation_id_t;
using protocol::mysqlx::insert_id_t;

typedef api::Async_op<void>   Async_op;
typedef api::Async_op<size_t> Proto_op;
typedef api::Severity Severity;

using cdk::api::Table_ref;
using cdk::api::Schema_ref;

typedef cdk::api::Row_processor<cdk::Traits>  Row_processor;

const error_category& server_error_category();
error_code server_error(int code);


class Server_error
    : public Error_class<Server_error>
{
public:

  typedef protocol::mysqlx::sql_state_t sql_state_t;

  Server_error(unsigned num, sql_state_t, const string& desc = string()) throw()
    : Error_base(NULL, server_error(static_cast<int>(num)), desc)
  {
    assert(num < (unsigned)std::numeric_limits<int>::max());
  }

  virtual ~Server_error() throw() {}

};


class Server_prepare_error
    : public Error_class<Server_prepare_error>
{
public:

  typedef protocol::mysqlx::sql_state_t sql_state_t;

  Server_prepare_error(unsigned num, sql_state_t, const string& desc = string()) throw()
    : Error_base(NULL, server_error(static_cast<int>(num)), desc)
  {
    assert(num < (unsigned)std::numeric_limits<int>::max());
  }

  virtual ~Server_prepare_error() throw() {}

};


}}  // cdk::mysql

#endif
