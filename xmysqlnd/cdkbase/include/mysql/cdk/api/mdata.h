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

#ifndef CDK_API_MDATA_H
#define CDK_API_MDATA_H

#include "mysql/cdk/foundation.h"


namespace cdk {
namespace api {

template <class Traits>
class Meta_data
{
public:

  typedef typename Traits::col_count_t   col_count_t;
  typedef typename Traits::Type_info     Type_info;
  typedef typename Traits::Format_info   Format_info;
  typedef typename Traits::Column_info   Column_info;

  virtual col_count_t col_count() const =0;

  virtual Type_info   type(col_count_t) =0;
  virtual Format_info format(col_count_t) =0;
  virtual Column_info col_info(col_count_t) =0;

};

}} // cdk::api

#endif
