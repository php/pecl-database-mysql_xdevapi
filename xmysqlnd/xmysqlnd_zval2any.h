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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_ZVAL2ANY_H
#define XMYSQLND_ZVAL2ANY_H

#include "proto_gen/mysqlx_datatypes.pb.h"
#include "util/strings.h"

namespace mysqlx {

namespace util { class zvalue; }

namespace drv {

enum_func_status scalar2zval(const Mysqlx::Datatypes::Scalar & scalar, zval * zv);
enum_func_status zval2any(const zval * const zv, Mysqlx::Datatypes::Any & any);
enum_func_status zval2any(const util::zvalue& zv, Mysqlx::Datatypes::Any& any);

enum_func_status any2zval(const Mysqlx::Datatypes::Any & any, zval * zv);
enum_func_status any2zval(const Mysqlx::Datatypes::Any& any, util::zvalue& zv);
util::zvalue any2zval(const Mysqlx::Datatypes::Any& any);

void any2log(const Mysqlx::Datatypes::Any & any);
void scalar2log(const Mysqlx::Datatypes::Scalar & scalar);
void repeated2log(
	const google::protobuf::RepeatedPtrField< Mysqlx::Datatypes::Scalar >& repeated);
uint64_t scalar2uint(const Mysqlx::Datatypes::Scalar & scalar);
int64_t scalar2sint(const Mysqlx::Datatypes::Scalar & scalar);
util::string scalar2string(const Mysqlx::Datatypes::Scalar & scalar);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_ZVAL2ANY_H */
