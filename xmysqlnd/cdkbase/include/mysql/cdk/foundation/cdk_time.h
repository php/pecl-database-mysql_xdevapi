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

#ifndef SDK_FOUNDATION_CDK_TIME_H
#define SDK_FOUNDATION_CDK_TIME_H

#include "common.h"

PUSH_SYS_WARNINGS_CDK
#include <time.h>
POP_SYS_WARNINGS_CDK

namespace cdk {
namespace foundation {

// In CDK, time points are expressed in milliseconds since epoch

typedef  long long  time_t;


// Get the current time point

inline
time_t  get_time()
{
  ::time_t   now_sec;
  long int   now_ms;

#if defined(_WIN32)

  SYSTEMTIME now;
  GetSystemTime(&now);
  now_ms = now.wMilliseconds;

#elif defined(__APPLE__)

  struct timeval now;
  gettimeofday(&now, NULL);
  now_ms = now.tv_usec / 1000;

#else

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  now_ms = now.tv_nsec / 1000000;

#endif

  ::time(&now_sec);

  return 1000U*now_sec + now_ms;
}


// Sleep for given number of milliseconds

inline
void sleep(time_t howlong)
{
#if defined(_WIN32)

  Sleep((DWORD)howlong);

#else

  struct timespec time;
  time.tv_sec = howlong/1000;
  time.tv_nsec = (howlong%1000)*1000000;

  while (!nanosleep(&time, &time))
  {
    if (EINTR != errno)
      break;
  }

#endif
}


}}  // cdk::foundation

#endif
