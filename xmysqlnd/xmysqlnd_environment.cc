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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "xmysqlnd_environment.h"
#include "util/strings.h"
#include <cstdlib>

namespace mysqlx {

namespace drv {

util::string Environment::get_as_string(Variable var)
{
	struct Variable_info {
		const char* test_env_var;
		const char* common_env_var;
		const char* default_value;
	};

	static const std::map<Environment::Variable, Variable_info> var_to_info = {
		{ Variable::Mysql_port, {"MYSQL_TEST_PORT", "MYSQL_PORT", "3306"} },
		{ Variable::Mysqlx_port, {"MYSQLX_TEST_PORT", "MYSQLX_PORT", "33060"} },
		{ Variable::Mysqlx_connection_timeout,
			{"MYSQLX_TEST_CONNECTION_TIMEOUT", "MYSQLX_CONNECTION_TIMEOUT", "10"} },
	};

	const Variable_info& var_info = var_to_info.at(var);

	const char* value = std::getenv(var_info.test_env_var);
	if (value != nullptr) {
		return value;
	}

	value = std::getenv(var_info.common_env_var);
	if (value != nullptr) {
		return value;
	}

	return var_info.default_value;
}

int Environment::get_as_int(Variable var)
{
	const util::string& value_str = get_as_string(var);
	return std::stoi(value_str.c_str());
}

} // namespace drv

} // namespace mysqlx
