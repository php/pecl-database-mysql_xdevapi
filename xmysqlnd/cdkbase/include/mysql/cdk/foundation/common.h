/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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

#ifndef CDK_FOUNDATION_COMMON_H
#define CDK_FOUNDATION_COMMON_H

/*
  Macros used to disable warnings for fragments of code.
*/

#if defined __GNUC__ || defined __clang__

#define PRAGMA(X) _Pragma(#X)
#define DISABLE_WARNING(W) PRAGMA(GCC diagnostic ignored #W)

#if defined __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define DIAGNOSTIC_PUSH PRAGMA(GCC diagnostic push)
#define DIAGNOSTIC_POP  PRAGMA(GCC diagnostic pop)
#else
#define DIAGNOSTIC_PUSH
#define DIAGNOSTIC_POP
#endif

#elif defined _MSC_VER


#define PRAGMA(X) __pragma(X)
#define DISABLE_WARNING(W) PRAGMA(warning (disable:W))

#define DIAGNOSTIC_PUSH  PRAGMA(warning (push))
#define DIAGNOSTIC_POP   PRAGMA(warning (pop))

#else

#define PRAGMA(X)
#define DISABLE_WARNING(W)

#define DIAGNOSTIC_PUSH
#define DIAGNOSTIC_POP

#endif


/*
  Macros to disable compile warnings in system headers. Put
  PUSH_SYS_WARNINGS/POP_SYS_WARNINGS around sytem header includes.
*/

#if defined _MSC_VER

/*
  Warning 4350 is triggered by std::shared_ptr<> implementation
  - see https://msdn.microsoft.com/en-us/library/0eestyah.aspx
*/

#define PUSH_SYS_WARNINGS \
  PRAGMA(warning (push,2)) \
  DISABLE_WARNING(4350) \
  DISABLE_WARNING(4738) \
  DISABLE_WARNING(4548)

#else

#define PUSH_SYS_WARNINGS DIAGNOSTIC_PUSH

#endif

#define POP_SYS_WARNINGS  DIAGNOSTIC_POP


// Avoid warnings from Protobuf includes

#if defined _MSC_VER

/*
  Turn /W1 level (severe warnings) - we are not really interested
  in tracing protbuf code warnings.
*/

#define PUSH_PB_WARNINGS  PRAGMA(warning(push,1))

#else

#define PUSH_PB_WARNINGS DIAGNOSTIC_PUSH \
    DISABLE_WARNING(-Wshadow) \
    DISABLE_WARNING(-Wunused-parameter) \
    DISABLE_WARNING(-Wdeprecated-declarations) \

#endif

#define POP_PB_WARNINGS   DIAGNOSTIC_POP


/*
  Include common system headers.
*/

PUSH_SYS_WARNINGS

#if defined(_WIN32)

#if _WIN32_WINNT < 0x0600
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #undef WIN32_LEAN_AND_MEAN
#else
  #include <windows.h>
#endif

#else

#if defined(__APPLE__)
  #include <sys/time.h>
#endif
#include <errno.h>

#endif


#if defined(_MSC_VER) && _MSC_VER < 1900
  #define NOEXCEPT
#else
  #define NOEXCEPT noexcept
#endif


#include <mysql/cdk/config.h>

#include <cstddef>
#include <assert.h>
#include <limits>
#include <utility>

POP_SYS_WARNINGS

#undef max
#undef THROW

/*
  Note: we add throw statement to the definition of THROW() so that compiler won't
  complain if it is used in contexts where, e.g., a value should be returned from
  a function.
*/

#ifdef THROW_AS_ASSERT

#define THROW(MSG)  do { assert(false && (MSG)); throw (MSG); } while(false)

#else

/*
  Code which uses this macro must ensure that function throw_error(const char*)
  is available in the context in which the macro is used. Primarily this should
  be function cdk::foundation::throw_error(const char*) defined in error.h. But
  it is intentionally left unqualified so that code can use different function
  implementations if needed.
*/

#define THROW(MSG) do { throw_error(MSG); throw (MSG); } while(false)

#endif






#endif
