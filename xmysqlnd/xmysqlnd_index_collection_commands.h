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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_INDEX_COLLECTION_COMMANDS_H
#define XMYSQLND_INDEX_COLLECTION_COMMANDS_H

#include "xmysqlnd_crud_commands.h"
#include "util/strings.h"
#include <boost/optional.hpp>
#include "xmysqlnd/xmysqlnd_session.h"

namespace mysqlx {

namespace drv {

struct st_xmysqlnd_session;
struct st_xmysqlnd_session_on_error_bind;
struct st_xmysqlnd_node_collection;

struct Index_field
{
	util::string path;
	util::string type;
	boost::optional<bool> required;
	boost::optional<util::string> collation;
	boost::optional<unsigned int> options;
	boost::optional<unsigned int> srid;

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
	boost::optional<bool> is_unique;
	boost::optional<Type> type;
	Fields fields;

	Index_definition(const util::string_view& index_name);

	boost::optional<util::string> get_type_str() const;
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

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
