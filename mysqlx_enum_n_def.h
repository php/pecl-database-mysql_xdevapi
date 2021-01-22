/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_ENUM_N_DEF_H
#define MYSQLX_ENUM_N_DEF_H

namespace mysqlx {

const int MYSQL_TYPE_SMALLINT  = MYSQL_TYPE_BIT + 1;
const int MYSQL_TYPE_MEDIUMINT = MYSQL_TYPE_BIT + 2;
const int MYSQL_TYPE_INT       = MYSQL_TYPE_BIT + 3;
const int MYSQL_TYPE_BIGINT    = MYSQL_TYPE_BIT + 4;
const int MYSQL_TYPE_BYTES     = MYSQL_TYPE_BIT + 5;

#define FIELD_TYPE_SMALLINT		MYSQL_TYPE_SMALLINT
#define FIELD_TYPE_MEDIUMINT	MYSQL_TYPE_MEDIUMINT
#define FIELD_TYPE_INT			MYSQL_TYPE_INT
#define FIELD_TYPE_BIGINT		MYSQL_TYPE_BIGINT
#define FIELD_TYPE_BYTES		MYSQL_TYPE_BYTES

const int MYSQLX_LOCK_DEFAULT = 0;
const int MYSQLX_LOCK_NOWAIT = 1;
const int MYSQLX_LOCK_SKIP_LOCKED = 2;

} // namespace mysqlx

#endif // MYSQLX_ENUM_N_DEF_H
