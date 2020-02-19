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

#ifndef CDK_API_OBJ_REF_H
#define CDK_API_OBJ_REF_H

#include "../foundation.h" // for string

namespace cdk {
namespace api {

/*
  Classes for describing database object references of the form:

   [[<catalog>.<schema>.]<table>.]<column>

*/

class Ref_base
{
public:

  virtual ~Ref_base() {}

  virtual const string name() const =0;
  virtual const string orig_name() const { return name(); }
};


class Schema_ref
    : public Ref_base
{
public:

  virtual const Ref_base* catalog() const { return NULL; }
};

class Object_ref
    : public Ref_base
{
public:
  virtual const Schema_ref* schema() const =0;
};


typedef Object_ref Table_ref;


class Column_ref
    : public Ref_base
{
public:
  virtual const Table_ref* table() const =0;
};


}}  // cdk::api


#endif
