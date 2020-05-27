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
#ifndef XMYSQLND_INDEX_COLLECTION_COMMANDS_H
#define XMYSQLND_INDEX_COLLECTION_COMMANDS_H

#include "xmysqlnd_crud_commands.h"
#include "util/strings.h"
#include <optional>
#include "xmysqlnd/xmysqlnd_session.h"

namespace mysqlx {

namespace drv {

class xmysqlnd_session;
struct st_xmysqlnd_session_on_error_bind;
struct xmysqlnd_collection;

struct Index_field
{
	util::string path;
	util::string type;
	std::optional<bool> required;
	std::optional<util::string> collation;
	std::optional<unsigned int> options;
	std::optional<unsigned int> srid;
	std::optional<bool> is_array;

	bool is_geojson() const;
	bool is_required() const;

};

struct Index_definition : util::custom_allocable
{
	enum class Type {
		Index,
		Spatial,
		Default = Index
	};

	using Fields = util::vector<Index_field>;

	util::string name;
	std::optional<bool> is_unique;
	std::optional<Type> type;
	Fields fields;

	Index_definition(const util::string_view& index_name);

	std::optional<util::string> get_type_str() const;
};

bool collection_create_index_execute(XMYSQLND_SESSION session,
	const util::string_view& schema_name,
	const util::string_view& collection_name,
	const Index_definition& index_def,
	st_xmysqlnd_session_on_error_bind on_error);

bool collection_drop_index_execute(XMYSQLND_SESSION session,
	const util::string_view& schema_name,
	const util::string_view& collection_name,
	const util::string_view& index_name,
	st_xmysqlnd_session_on_error_bind on_error);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_INDEX_COLLECTION_COMMANDS_H */
