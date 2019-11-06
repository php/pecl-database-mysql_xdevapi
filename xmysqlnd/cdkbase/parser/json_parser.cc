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

#include "json_parser.h"
#include <mysql/cdk.h>
PUSH_SYS_WARNINGS_CDK
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
POP_SYS_WARNINGS_CDK
#include <stack>


PUSH_SYS_WARNINGS_CDK
#include <stdlib.h>
POP_SYS_WARNINGS_CDK


/*
  The maximum absolute value that 64-bit signed integer can have.
  For most platforms it is the minimum negative LLONG_MIN.
  The absolute value for it cannot be obtained because
  ABS(LLONG_MAX) < ABS(LLONG_MIN). Therefore, we define a constant.
*/
#define INTEGER_ABS_MAX 9223372036854775808UL

using namespace parser;
using cdk::string;
using cdk::JSON;
typedef  cdk::JSON::Processor Processor;

void JSON_parser::process(Expr_base::Processor &prc) const
{
  rapidjson::Reader m_parser;

  /*
    Note: In-situ parsing could be faster but it would also destroy
    contents of m_json and we need it for error reporting.
  */

  rapidjson::GenericStringStream<rapidjson::UTF8<char>>
  ss(const_cast<char*>(m_json.data()));

  /*
    struct to be used as handler for rapidjson parser
  */
  struct Processor_cvt
  {

    struct processors
    {
      Processor *m_obj = nullptr;
      Processor::Any_prc* m_key = nullptr;
      Processor::Any_prc::List_prc* m_arr = nullptr;

      processors(Processor *obj)
      {
        m_obj = obj;
      }

      processors(Processor::Any_prc* key)
      {
        m_key = key;
      }

      processors(Processor::Any_prc::List_prc* arr)
      {
        m_arr = arr;
      }
    };

    std::stack<processors> m_stack;

    Processor_cvt (Processor &prc)
    {
      m_stack.push(&prc);
    }

    bool Null()
    {
      if (m_stack.empty())
        return false;

      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->null();
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->null();
      return true;
    }

    bool Bool(bool b)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->yesno(b);
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->yesno(b);
      else
        return false;

      return true;
    }

    bool Int(int i)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->num(static_cast<int64_t>(i));
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->num(static_cast<int64_t>(i));
      else
        return false;

      return true;
    }

    bool Uint(unsigned u)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->num(static_cast<uint64_t>(u));
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->num(static_cast<uint64_t>(u));
      else
        return false;

      return true;
    }

    bool Int64(int64_t i)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->num(i);
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->num(i);
      else
        return false;

      return true;
    }

    bool Uint64(uint64_t u)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->num(u);
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->num(u);
      else
        return false;
      return true;
    }

    bool Double(double d)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->num(d);
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->num(d);
      else
        return false;
      return true;
    }

    bool RawNumber(const char* /*str*/,
                   rapidjson::SizeType /*length*/,
                   bool /*copy*/)
    {
      // not needed
        return false;
    }

    bool String(const char* str, rapidjson::SizeType length, bool /*copy*/)
    {
      if (m_stack.empty())
        return false;


      if (m_stack.top().m_key)
      {
        m_stack.top().m_key->scalar()->str(std::string(str, length));
        m_stack.pop();
      }
      else if (m_stack.top().m_arr)
        m_stack.top().m_arr->list_el()->scalar()->str(std::string(str, length));

      return true;
    }

    bool Key(const char* str, rapidjson::SizeType length, bool /*copy*/)
    {
      if (m_stack.empty())
        return false;

      if (m_stack.top().m_obj)
        m_stack.push(m_stack.top().m_obj->key_val(std::string(str, length)));
      else
        return false;

      return true;
    }

    bool StartObject()
    {
      if (m_stack.empty())
        return false;

      if(m_stack.top().m_key)
      {
        m_stack.push(m_stack.top().m_key->doc());
      }
      else if (m_stack.top().m_arr)
      {
        m_stack.push(m_stack.top().m_arr->list_el()->doc());
      }
      else if (!m_stack.top().m_obj)
        return false;

      m_stack.top().m_obj->doc_begin();

      return true;
    }


    bool EndObject(rapidjson::SizeType )
    {
      if(m_stack.empty())
        return false;

      if (m_stack.top().m_obj)
      {
        m_stack.top().m_obj->doc_end();
      }

      m_stack.pop(); // Pop obj

      if (!m_stack.empty() && m_stack.top().m_key)
        m_stack.pop(); // Pop key

      return true;
    }
    bool StartArray()
    {
      if (m_stack.empty() )
        return false;

      if (m_stack.top().m_key)
      {
        m_stack.push(m_stack.top().m_key->arr());
      }
      else if (m_stack.top().m_arr)
      {
        m_stack.push(m_stack.top().m_arr->list_el()->arr());
      }
      else return false;

      m_stack.top().m_arr->list_begin();

      return true;
    }

    bool EndArray(rapidjson::SizeType )
    {
      if (m_stack.empty() )
        return false;

      if (!m_stack.top().m_arr)
        return false;

      m_stack.top().m_arr->list_end();
      m_stack.pop();// Pop array
      if (!m_stack.empty() && m_stack.top().m_key)
        m_stack.pop();// pop key

      return true;
    }

  };

  Processor_cvt cvt(prc);

  auto error = m_parser.Parse<>(ss, cvt);
  if (error.IsError())
  {
    throw JSON_parser::Error(m_json,
                             error.Offset(),
                             rapidjson::GetParseError_En(error.Code()));
  }
}

void json_parse(const std::string &json, Processor &dp)
{
  JSON_parser    parser(json);
  parser.process(dp);
}

