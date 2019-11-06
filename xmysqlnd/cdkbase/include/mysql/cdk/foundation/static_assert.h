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

#ifndef SDK_FOUNDATION_STATIC_ASSERT_H
#define SDK_FOUNDATION_STATIC_ASSERT_H

/*
  Emulation of some C++11 features used in CDK code.
*/

#include <mysql/cdk/config.h>


namespace cdk {
namespace foundation {

#ifndef HAVE_STATIC_ASSERT

/*
  Idea: Instantiation of static_assertion_test<false> is not defined and compiler
  will choke on sizeof() in static_assert(). But static_assertion_test<true> is
  defined and compiler should go through static_assert().
*/

template <bool test>
struct static_assert_test;

template <>
struct static_assert_test<true>
{
};

// TODO: Fix this implementation or think how to avoid it altogether
// TODO: Better message when assertion fails

#define static_assert(B,Msg)
/*
  enum { static_assert_test ## __LINE_ \
         = sizeof(cdk::foundation::static_assert_test< (bool)(B) >) }
*/

#endif

}}  // cdk::foundation


#endif
