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

#ifndef MYSQL_CDK_H
#define MYSQL_CDK_H

#include "cdk/foundation.h"
#include "cdk/common.h"
#include "cdk/session.h"
#include "cdk/reply.h"
#include "cdk/cursor.h"
#include "cdk/codec.h"

/*
  On Windows, external dependencies can be declared using
  #pragma comment directive.
*/

#ifdef _WIN32
#pragma comment(lib,"ws2_32")
#if defined(WITH_SSL) && !defined(WITH_SSL_WOLFSSL)
  #pragma comment(lib,"ssleay32")
  #pragma comment(lib,"libeay32")
#endif
#endif


namespace cdk {

typedef Cursor::Row_processor Row_processor;

}  // cdk

#endif
