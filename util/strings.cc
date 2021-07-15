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
#include "php_api.h"
#include "mysqlnd_api.h"
#include "strings.h"

namespace mysqlx {

namespace util {

std::ostream& operator<<(std::ostream& os, const string& str)
{
	return os << str.c_str();
}


void single_separator_split(
        util::vector< util::string >& output,
        const util::string& input,
        const char separator
)
{
    if( input.empty() ) {
        return;
    }
    size_t idx{ 0 }, last{ 0 };
    for( ; idx < input.size() ; ++idx ){
        if( input[ idx ] == separator ) {
            util::string token = input.substr( last, idx - last );
            output.push_back( token );
            last = idx + 1;
        }
    }
    if( idx >= last ) {
        util::string token = input.substr( last );
        output.push_back( token );
    }
}

} // namespace util

} // namespace mysqlx
