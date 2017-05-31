/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  +----------------------------------------------------------------------+
*/
#ifndef MYSQLX_SESSION_CONFIG_MANAGER_H
#define MYSQLX_SESSION_CONFIG_MANAGER_H

#include <utility>
#include "phputils/strings.h"
#include "phputils/types.h"
#include "phputils/allocator.h"

namespace mysqlx {

namespace devapi {

/*
 * Small utility object which helps to
 * create URIs. We need to store the session
 * information as URI anyway, so it helps to have
 * this little utility
 */
class URI_builder
{
public:
	URI_builder() = default;
	URI_builder& user( const phputils::string& value );
	URI_builder& password( const phputils::string& value );
	URI_builder& host( const phputils::string& value );
	URI_builder& port( const std::size_t value );
	URI_builder& schema( const phputils::string& value );
	URI_builder& add_attrib( const phputils::string& key,const phputils::string& value );
	phputils::string build();
private:
	phputils::string saved_user;
	phputils::string saved_pass;
	phputils::string saved_host;
	phputils::string saved_port;
	phputils::string saved_schema;
	//Like TLS options &c.
	phputils::map<phputils::string,phputils::string> attributes;
};

/*
 * Implementation of the Session config object, this object
 * can be created and is manipulated by Session_config_manager.
 */
class Session_config : public phputils::custom_allocable
{
public:
	Session_config() = default;
	explicit Session_config( const phputils::string& name );
	explicit Session_config( const phputils::string& name,
				   const phputils::string& uri );
	phputils::string get_name() const;
	void set_uri( const phputils::string& uri );
	phputils::string get_uri() const;

	void set_app_data( const phputils::string& name, const phputils::string& value );
	bool delete_app_data( const phputils::string& key );
	void clear_app_data();
	phputils::string get_app_data( const phputils::string& key ) const;

	phputils::string get_json() const;

	bool add_attributes( const std::pair<phputils::string,phputils::string>& attrib );
	bool operator == ( const Session_config& other ) const;
	bool operator == ( const phputils::string& name ) const;
	~Session_config() = default;
private:
	phputils::string session_name;
	phputils::string session_uri;
	phputils::map<phputils::string,phputils::string> session_app_data;
	/*
	 * There are a certain amount of attributes allowed
	 * in the JSON/Dictionary, those are stored in this
	 * 'attribues' map
	 */
	phputils::map<phputils::string,phputils::string> attributes;
};

/*
 * session_json_def helps to easily
 * move the session jsons with their name
 * between Session_config_manager and the
 * persistence handler.
 */
using session_json_def = std::pair<phputils::string,phputils::string>;
using session_json_defs = phputils::vector<std::pair<phputils::string,phputils::string>>;
using raw_loaded_sessions = phputils::map<phputils::string,zval>;

/*
 * Interface for the persistence handler
 */
class Persistence_handler : public phputils::custom_allocable
{
public:
	/*
	 * Store in the sessions in the user
	 * configuration data file
	 */
	virtual std::size_t store( const session_json_defs& sessions ) = 0;
	/*
	 * Load the sessions from the system configuration
	 * file and from the user configuration file.
	 * If two sessions exist in both files, the one in the
	 * system conf. file overrides the one in the user conf.
	 * file
	 */
	virtual raw_loaded_sessions load() = 0;
	virtual ~Persistence_handler() {}
};

/*
 * Implementation for the handler of the session
 * configuration, this is the ony object allowed to
 * create Session_config.
 */
class Session_config_manager : public phputils::custom_allocable
{
public:
	Session_config_manager();

	Session_config save( const phputils::string& session_name,
						  zval* session_json );

	Session_config save( const phputils::string& session_name,
						 const phputils::string& session_uri );

	Session_config save( const Session_config& session );

	Session_config save( const phputils::string& session_name,
						 const phputils::string& session_uri,
						 zval* appdata );

	Session_config get( const phputils::string& name );
	bool remove( const phputils::string& name );
	phputils::vector<phputils::string> list();

	static Session_config_manager* get_instance() {
		/*
		 * Ugly way of creating a singleton, a static reference
		 * should be preferred to avoid leaks, but our allocator
		 * do not work with static variables now..
		 */
		static Session_config_manager* instance{ nullptr };
		if( nullptr == instance ) {
			instance = new Session_config_manager;
		}
		return instance;
	}
	~Session_config_manager();
private:
	std::shared_ptr<Persistence_handler> persistence_handler;
	using sessions_it = phputils::vector<Session_config>::iterator;
	phputils::vector<Session_config> sessions;
	/*
	 * Map an attribute with a proper
	 * action number, this allows to avoid
	 * a list of if/switch full of strcmp
	 */
	enum class base_attribs {
		user,
		password,
		host,
		port,
		schema,
		appdata,
		uri
	};
	phputils::map<phputils::string,base_attribs> attrib_map;
	bool process_appdata_array( Session_config& config, zval* appdata );
	sessions_it find_session( const phputils::string& name );
	Session_config create_session_from_json(const phputils::string &session_name,
								zval* session_json);
	Session_config parse_loaded_session(const phputils::string &session_name,
								zval *session_zval);
	bool add_session( Session_config& new_session );
	void store_sessions();
};

/*
 * Default implementation of the
 * persistance handler
 */
class Default_persistence_handler : public Persistence_handler
{
public:
	Default_persistence_handler();
	std::size_t store( const session_json_defs& session ) override;
	raw_loaded_sessions load() override;
	~Default_persistence_handler();
private:
	phputils::string user_config_file_path;
	phputils::string system_config_file_path;
	std::size_t write_session_data();
	php_stream* open_file( const phputils::string& filename,
						   const phputils::string& mode );
	raw_loaded_sessions load_impl( const phputils::string& filename );
};

} // namespace devapi

} // namespace mysqlx

#endif /* MYSQLX_SESSION_CONFIG_MANAGER_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
