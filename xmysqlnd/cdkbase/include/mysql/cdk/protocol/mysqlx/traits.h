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

#ifndef MYSQL_CDK_PROTOCOL_MYSQLX_TRAITS_H
#define MYSQL_CDK_PROTOCOL_MYSQLX_TRAITS_H

namespace cdk {
namespace protocol {
namespace mysqlx {

// These are temporary type declarations
// TODO: remove when the types are defined

typedef uint32_t stmt_id_t;
typedef uint32_t cursor_id_t;
typedef uint64_t row_count_t;
typedef uint32_t col_count_t;
// Note: protocol uses 64bit numbers for collation ids
typedef uint64_t collation_id_t;
typedef uint64_t insert_id_t;

typedef int64_t  sint64_t;
using   ::uint64_t;

}}}

#endif
