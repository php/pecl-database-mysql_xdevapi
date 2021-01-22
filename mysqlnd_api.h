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
#ifndef MYSQL_XDEVAPI_MYSQLND_API_H
#define MYSQL_XDEVAPI_MYSQLND_API_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PHP_WIN32
#pragma warning( push )
#pragma warning( disable : 4018 4244 4267)
#endif // PHP_WIN32

#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_priv.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_charset.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
#include <ext/mysqlnd/mysql_float_to_double.h>
#include <ext/mysqlnd/mysqlnd_portability.h>
#include <ext/mysqlnd/mysqlnd_vio.h>
#include <ext/mysqlnd/mysqlnd_connection.h>
#include <ext/mysqlnd/mysqlnd_auth.h>
#include <ext/mysqlnd/mysqlnd_wireprotocol.h>

#ifdef PHP_WIN32
#pragma warning( pop )
#endif // PHP_WIN32


#ifdef PHP_WIN32

#if MYSQLND_DBG_ENABLED == 1

#undef DBG_ENTER
#define DBG_ENTER(func_name) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127 4267)) \
	DBG_ENTER_EX(MYSQLND_G(dbg), (func_name)); \
	__pragma(warning(pop)) \
	{}

#undef DBG_INF
#define DBG_INF(msg) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127 4245)) \
	DBG_INF_EX(MYSQLND_G(dbg), (msg)); \
	__pragma(warning(pop)) \
	{}

#undef DBG_INF_FMT
#define DBG_INF_FMT(...) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127 4245)) \
	DBG_INF_FMT_EX(MYSQLND_G(dbg), __VA_ARGS__); \
	__pragma(warning(pop)) \
	{}

#undef DBG_ERR
#define DBG_ERR(msg) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4245)) \
	DBG_ERR_EX(MYSQLND_G(dbg), (msg)); \
	__pragma(warning(pop)) \
	{}

#undef DBG_ERR_FMT
#define DBG_ERR_FMT(...) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4245)) \
	DBG_ERR_FMT_EX(MYSQLND_G(dbg), __VA_ARGS__); \
	__pragma(warning(pop)) \
	{}

#undef DBG_RETURN
#define DBG_RETURN(value) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	DBG_RETURN_EX(MYSQLND_G(dbg), (value)); \
	__pragma(warning(pop)) \
	{}

#undef DBG_VOID_RETURN
#define DBG_VOID_RETURN \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	DBG_VOID_RETURN_EX(MYSQLND_G(dbg)); \
	__pragma(warning(pop)) \
	{}

#endif // MYSQLND_DBG_ENABLED == 1

#endif // PHP_WIN32

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MYSQL_XDEVAPI_MYSQLND_API_H
