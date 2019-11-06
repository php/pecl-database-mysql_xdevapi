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
  | Authors: Oracle Corp                                                 |
  +----------------------------------------------------------------------+
*/

#ifndef COMPILERUTILS_H
#define COMPILERUTILS_H

#ifdef UNUSED
#  elif defined(__GNUC__)
#    define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#  elif defined(__LCLINT__)
#    define UNUSED(x) /*@unused@*/ x
#  elif defined(__cplusplus)
#    define UNUSED(x)
#  else
#    define UNUSED(x) x
#endif

#endif  /* COMPILERUTILS_H */
