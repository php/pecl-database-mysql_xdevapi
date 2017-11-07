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
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/standard/url.h>
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
}

#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include <phputils/object.h>
#include "xmysqlnd/xmysqlnd_session_config.h"
#include <mysqlx_class_properties.h>
#include <cctype>
#include <algorithm>

namespace mysqlx {

namespace devapi {

/* {{{ is_valid_identifier */
static bool
is_valid_identifier( const phputils::string& str )
{
	const int max_identifier_name_length{ 31 };
	if( str.size() > max_identifier_name_length ) {
		return false;
	}
	int num_of_letter{ 0 };
	int num_of_digit{ 0 };
	for( auto& ch : str ) {
        if( false == std::isalnum( ch ) ) {
			return false;
		}
        if( 0 < std::isalpha( ch ) ) {
			++num_of_letter;
		} else {
			++num_of_digit;
		}
	}
	return num_of_letter > 0;
}
/* }}} */

/* {{{ Session_config::Session_config */
Session_config::Session_config(const phputils::string &name)
{
	if( is_valid_identifier( name ) ) {
		session_name = name;
	} else {
		RAISE_EXCEPTION( err_msg_invalid_identifier );
	}
}
/* }}} */


/* {{{ Session_config::Session_config */
Session_config::Session_config(const mysqlx::phputils::string &name,
							const mysqlx::phputils::string &uri)
{
	if( is_valid_identifier( name ) &&
		false == uri.empty() ) {
		session_name = name;
		session_uri = uri;
	} else {
		RAISE_EXCEPTION( err_msg_invalid_identifier );
	}
}
/* }}} */


/* {{{ Session_config::get_name */
phputils::string Session_config::get_name() const
{
	return session_name;
}
/* }}} */


/* {{{ Session_config::set_uri */
void Session_config::set_uri(const phputils::string &uri)
{
	DBG_ENTER("Session_config::set_uri");
	DBG_INF_FMT("Changing URI for session configuration %s to %s",
				session_name.c_str(),
				uri.c_str());
	session_uri = uri;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Session_config::get_uri */
phputils::string Session_config::get_uri() const
{
	return session_uri;
}
/* }}} */


/* {{{ Session_config::set_app_data */
void Session_config::set_app_data(const phputils::string &name,
					const phputils::string &value)
{
	DBG_ENTER("Session_config::set_app_data");
	DBG_INF_FMT("Setting AppData for session configuration %s: %s:%s",
				session_name.c_str(),
				name.c_str(),
				value.c_str());
	session_app_data[ name ] = value;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Session_config::delete_app_data */
bool Session_config::delete_app_data(const phputils::string &key)
{
	DBG_ENTER("Session_config::delete_app_data");
	bool ret{ true };
	DBG_INF_FMT("Deleting AppData for session configuration %s, key %s",
				session_name.c_str(),
				key.c_str());
	auto it = session_app_data.find( key );
	if( it != session_app_data.end() ) {
		session_app_data.erase( key );
	} else {
		ret = false;
	}
	DBG_RETURN( ret );
}

void Session_config::clear_app_data()
{
	DBG_ENTER("Session_config::clear_app_data");
	session_app_data.clear();
}
/* }}} */


/* {{{ Session_config::get_app_data */
phputils::string Session_config::get_app_data(const phputils::string &key) const
{
	DBG_ENTER("Session_config::get_app_data");
	auto it = session_app_data.find( key );
	if( it == session_app_data.end() ) {
		DBG_RETURN( phputils::string() );
	}
	DBG_RETURN( it->second );
}
/* }}} */

/*
 * Create the JSON in the format used
 * to save and store the session configuration
 * on disk
 */
/* {{{ Session_config::get_json */
phputils::string Session_config::get_json() const
{
	DBG_ENTER("Session_config::get_json");
	phputils::string json;
	//Insert the URI first
	json += "{\"uri\":\"";
	json += get_uri() + "\"";
	//Add the appdata (if any)
	if( !session_app_data.empty() ) {
		json += ",\"appdata\":{";
		std::size_t cnt = session_app_data.size();
		for( auto& data: session_app_data ) {
			json += "\""+data.first+"\":\""+data.second+"\"";
			if( --cnt )
				json += ",";
		}
		json += "}";
	}
	//Done
	json += "}";
	DBG_INF_FMT("The created JSON is: %s\n",
				json.c_str());
	DBG_RETURN( json );
}
/* }}} */


/* {{{ Session_config::add_attributes */
bool Session_config::add_attributes(const std::pair<phputils::string, phputils::string> &attrib)
{
	DBG_ENTER("Session_config::add_attributes");
	bool ret = true;
	auto it = attributes.find( attrib.first );
	if( it != attributes.end() ) {
		//Twice the same attribute? Must be an error
		DBG_ERR_FMT("Adding twice the same attribute (%s), are you sure?",
					attrib.first.c_str());
		ret = false;
	} else {
		DBG_INF_FMT("Adding attribute: %s:%s",
					attrib.first.c_str(),
					attrib.second.c_str());
		attributes.insert( attrib );
	}
	DBG_RETURN( ret );
}
/* }}} */


/* {{{ Session_config::operator == */
bool Session_config::operator ==(const Session_config &other) const
{
	return session_name == other.session_name;
}
/* }}} */

/*
 * Comparing a Session_config with a session name
 * make sense since each Session_config should have
 * an unique session name.
 */
/* {{{ Session_config::operator == */
bool Session_config::operator ==(const phputils::string &name) const
{
	return session_name == name;
}
/* }}} */


/* {{{ Session_config::Session_config_manager */
Session_config_manager::Session_config_manager()
{
	DBG_ENTER("Session_config_manager::Session_config_manager");
	/*
	 * Create the proper mapping of
	 * base uri components
	 */
	attrib_map["user"] = base_attribs::user;
	attrib_map["password"] = base_attribs::password;
	attrib_map["host"] = base_attribs::host;
	attrib_map["port"] = base_attribs::port;
	attrib_map["schema"] = base_attribs::schema;
	attrib_map["appdata"] = base_attribs::appdata;
	/*
	 * Instantiate the persistenace handler and
	 * load the available sessions
	 */
	persistence_handler = std::make_shared<Default_persistence_handler>();
	if( nullptr == persistence_handler ) {
		RAISE_EXCEPTION( err_msg_internal_error );
		DBG_VOID_RETURN;
	}
	//Attempt to load the sessions
	raw_loaded_sessions loaded_session = persistence_handler->load();
	for( auto session : loaded_session )
	{
		Session_config new_config = parse_loaded_session( session.first,
														&session.second );
		if( false == add_session( new_config ) ) {
			RAISE_EXCEPTION( err_msg_not_uniq_conf_name );
			DBG_VOID_RETURN;
		}
		zval_dtor( &session.second );
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Session_config::get */
Session_config Session_config_manager::get(const phputils::string &name)
{
	DBG_ENTER("Session_config_manager::get");
	Session_config session;
	auto it = std::find( sessions.begin(),
						 sessions.end(),
						 name );
	if( it == sessions.end() ) {
		DBG_ERR_FMT("Unable to find the session %s",
					name.c_str());
	} else {
		session = *it;
	}
	DBG_RETURN( session );
}
/* }}} */


/* {{{ Session_config::save */
Session_config Session_config_manager::save(const phputils::string &session_name,
											 zval* session_json)
{
	DBG_ENTER("Session_config_manager::save(name,json)");
	Session_config new_config;

	new_config = create_session_from_json( session_name,
									session_json );

	if( false == add_session( new_config ) ) {
		RAISE_EXCEPTION( err_msg_not_uniq_conf_name );
	} else {
		store_sessions();
	}

	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::save */
Session_config Session_config_manager::save(const phputils::string &session_name,
								const phputils::string &session_uri)
{
	DBG_ENTER("Session_config_manager::save(name,uri)");
	Session_config new_config( session_name );
	new_config.set_uri( session_uri );
	if( false == add_session( new_config ) ) {
		RAISE_EXCEPTION( err_msg_not_uniq_conf_name );
	} else {
		store_sessions();
	}

	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::save */
Session_config Session_config_manager::save(const Session_config &session)
{
	DBG_ENTER("Session_config_manager::save(session)");
	Session_config new_config = session;
	/*
	 * The session must be already stored, what we're
	 * saving is the internally stored session.
	 *
	 */
	auto it = find_session( session.get_name() );
	if( it == sessions.end() ) {
		DBG_INF_FMT("Storing again the session, was not found.");
		/*
		 * This can happen if the user call 'remove'
		 * on the session and then decide to 'save' it
		 * again.
		 */
		if( false == add_session( new_config ) ) {
			RAISE_EXCEPTION( err_msg_internal_error );
			DBG_RETURN(Session_config{});
		}
	} else {
		DBG_INF_FMT("Storing an existing session.");
		//Extract and return the exiting Session_config
		//BugID 25860579, 'update' is removed. now save override
		//the existing configuration
		*it = new_config;
	}
	store_sessions();
	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::save */
Session_config Session_config_manager::save(const phputils::string &session_name,
								const phputils::string &session_uri,
								zval *appdata)
{
	DBG_ENTER("Session_config_manager::save(name,uri,appdata)");
	Session_config new_config( session_name );
	new_config.set_uri( session_uri );
	process_appdata_array( new_config, appdata );
	if( false == add_session( new_config ) ) {
		RAISE_EXCEPTION( err_msg_not_uniq_conf_name );
	} else {
		store_sessions();
	}
	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::process_appdata_array */
bool Session_config_manager::process_appdata_array( Session_config& config,
										zval *appdata )
{
	DBG_ENTER("Session_config_manager::process_appdata_array");

	if( Z_TYPE_P(appdata) != IS_ARRAY ) {
		DBG_ERR_FMT("Appdata is expected to be an array!");
		DBG_RETURN(false);
	}
	//BugID 25829054, clear existing AppData entirely when
	//a new dictionary is provided.
	config.clear_app_data();

	zend_string* key;
	zval* val;
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(appdata), key, val) {
		if( Z_TYPE_P( val ) != IS_STRING ) {
			DBG_RETURN(false);
		}
		config.set_app_data( ZSTR_VAL( key ),
							 Z_STRVAL_P( val ) );
	} ZEND_HASH_FOREACH_END();

	DBG_RETURN(true);
}
/* }}} */


/* {{{ Session_config::find_session */
Session_config_manager::sessions_it
Session_config_manager::find_session(const phputils::string &name)
{
	return std::find( sessions.begin(),
					  sessions.end(),
					  name );
}
/* }}} */


/* {{{ Session_config::create_session_from_json */
Session_config Session_config_manager::create_session_from_json(const phputils::string &session_name,
										zval *session_json)
{
	DBG_ENTER("Session_config_manager::create_session_from_json");
	bool ret{ true };
	Session_config new_config( session_name );
	URI_builder uri;
	/*
	 * Extract the relevant information
	 * from the decoded JSON
	 */
	zend_string * key;
	zval * val;
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(session_json), key, val) {
		auto it = attrib_map.find( ZSTR_VAL( key ) );
		if( it == attrib_map.end() ) {
			/*
				 * There are non-base attribute
				 * which we need to save, like SSL
				 * or other, also for backward
				 * compatibility reason (dbUser &c)
				 */
			if( Z_TYPE_P( val ) == IS_STRING ) {
				uri.add_attrib(ZSTR_VAL( key ),
							   Z_STRVAL_P( val ));
			} else {
				ret = false;
			}
		} else {
			switch( it->second ) {
			case base_attribs::user:
				uri.user( Z_STRVAL_P( val ) );
				break;
			case base_attribs::password:
				uri.password( Z_STRVAL_P( val ) );
				break;
			case base_attribs::host:
				uri.host( Z_STRVAL_P( val ) );
				break;
			case base_attribs::port:
			{
				if( Z_TYPE_P( val ) != IS_LONG ) {
					DBG_ERR_FMT("The URI 'port' field is not a number!");
					ret = false;
				} else {
					uri.port( Z_LVAL_P( val ) );
				}
			}
				break;
			case base_attribs::schema:
				uri.schema( Z_STRVAL_P( val ) );
				break;
			case base_attribs::appdata:
				ret = process_appdata_array( new_config, val );
				break;
			default:
				break;
			}
		}
		if( false == ret ) {
			RAISE_EXCEPTION( err_msg_wrong_param_5 );
			DBG_RETURN(Session_config{});
		}

	} ZEND_HASH_FOREACH_END();

	new_config.set_uri( uri.build() );
	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::parse_loaded_session */
Session_config Session_config_manager::parse_loaded_session(
				const phputils::string &session_name,
				zval *session_zval)
{
	DBG_ENTER("Session_config_manager::parse_loaded_session");
	bool ret{ true };
	Session_config new_config( session_name );
	zend_string * key;
	zval * val;
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(session_zval), key, val) {
		/*
		 * We expect only two fields in the session zval:
		 * 1) uri
		 * 2) appdata
		 * Everything else is stored as attribute in the URI,
		 * any additional information is an error!
		 */
		phputils::string str_key( ZSTR_VAL( key ) );
		if( str_key == "uri" ) {
			new_config.set_uri( Z_STRVAL_P( val ) );
		} else if( str_key == "appdata" ) {
			ret = process_appdata_array( new_config, val );
		} else {
			DBG_ERR_FMT("Unable to parse the session data file!");
			ret = false;
			break;
		}
	} ZEND_HASH_FOREACH_END();

	if( false == ret ) {
		RAISE_EXCEPTION( err_msg_sess_file_corrupted );
	}
	DBG_RETURN( new_config );
}
/* }}} */


/* {{{ Session_config::add_session */
bool Session_config_manager::add_session(Session_config& new_session)
{
	DBG_ENTER("Session_config_manager::add_session");
	bool ret{ true };
	auto it = find_session( new_session.get_name() );
	if( it != sessions.end() ) {
		DBG_ERR_FMT("Cannot add two session with the same name: %s",
					new_session.get_name().c_str() );
		ret = false;
	} else {
		sessions.emplace_back( std::move(new_session) );
	}
	DBG_RETURN( ret );
}
/* }}} */


/* {{{ Session_config::store_sessions */
void Session_config_manager::store_sessions()
{
	DBG_ENTER("Session_config_manager::store_sessions");
	if( nullptr != persistence_handler ) {
		session_json_defs session_data;
		for( auto& session : sessions ) {
			session_data.push_back( {session.get_name(), session.get_json() } );
		}
		persistence_handler->store( session_data );
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ Session_config::remove */
bool Session_config_manager::remove(const phputils::string& name)
{
	DBG_ENTER("Session_config_manager::remove");
	bool ret{ true };
	auto it = find_session( name );
	if( it != sessions.end() ) {
		DBG_INF_FMT("Removing session configuration: %s",
					name.c_str() );
		sessions.erase( it );
		store_sessions();
	} else {
		DBG_INF_FMT("Session configuration %s not found!",
					name.c_str() );
		ret = false;
	}
	DBG_RETURN( ret );
}
/* }}} */


/* {{{ Session_config::list */
phputils::vector<phputils::string> Session_config_manager::list()
{
	DBG_ENTER("Session_config_manager::list");
	phputils::vector<phputils::string> list_of_sessions;
	for( auto&& session : sessions ) {
		list_of_sessions.emplace_back( session.get_name() );
	}
	DBG_INF_FMT("Amount of session: %d",
				list_of_sessions.size());
	DBG_RETURN( list_of_sessions );
}

/* }}} */


/* {{{ Session_config::~Session_config_manager */
Session_config_manager::~Session_config_manager()
{}
/* }}} */


/* {{{ Session_config::user */
URI_builder &URI_builder::user(const phputils::string& value)
{
	DBG_ENTER("URI_builder::user");
	DBG_INF_FMT("Setting user to %s",
				value.c_str());
	saved_user = value;
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::password */
URI_builder &URI_builder::password(const phputils::string& value)
{
	DBG_ENTER("URI_builder::password");
	DBG_INF_FMT("Setting password to %s",
				value.c_str());
	saved_pass = value;
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::host */
URI_builder &URI_builder::host(const phputils::string& value)
{
	DBG_ENTER("URI_builder::host");
	DBG_INF_FMT("Setting host to %s",
				value.c_str());
	saved_host = value;
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::port */
URI_builder &URI_builder::port(const std::size_t value)
{
	DBG_ENTER("URI_builder::port");
	DBG_INF_FMT("Setting port to %d",
				value);
	saved_port = std::to_string( value ).c_str();
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::schema */
URI_builder &URI_builder::schema(const phputils::string& value)
{
	DBG_ENTER("URI_builder::schema");
	DBG_INF_FMT("Setting schema to %s",
				value.c_str());
	saved_schema = value;
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::add_attrib */
URI_builder &URI_builder::add_attrib(
		const phputils::string& key,
		const phputils::string& value)
{
	DBG_ENTER("URI_builder::add_attrib");
	DBG_INF_FMT("Adding attrib %s:%s",
				key.c_str(),value.c_str());
	attributes[ key ] = value;
	DBG_RETURN(*this);
}
/* }}} */


/* {{{ Session_config::build */
phputils::string URI_builder::build()
{
	DBG_ENTER("URI_builder::build");
	phputils::string uri = "mysqlx://"+saved_user+":"+saved_pass+"@"+
			saved_host+":"+saved_port+
			(saved_schema.empty()?"":"/"+saved_schema);
	if( !attributes.empty() ) {
		uri += "/?";
		std::size_t cnt = attributes.size();
		for( auto& attr : attributes ) {
			uri += attr.first;
			//It might be an empty attrib like enable-ssl.
			if( !attr.second.empty() ) {
				uri += "=" + attr.second;
			}
			if( --cnt )
				uri += "&";
		}
	}
	DBG_INF_FMT("The URI is: %s",
				uri.c_str());
	DBG_RETURN( uri );
}
/* }}} */

namespace
{
/*
 * Those location are defined in the SPEC for MY-235
 */
const char* unix_global_config_path = "/etc/mysql/sessions.json";
const char* unix_user_config_path = "~/.mysql/session.json";

const char* win_global_config_path = "%PROGRAMDATA%MySQLsessions.json";
const char* win_user_config_path = "%APPDATA%MySQLsessions.json";
}
/* }}} */


/* {{{ DefaultPersistenceHandler::DefaultPersistenceHandler */
Default_persistence_handler::Default_persistence_handler()
{
	DBG_ENTER("DefaultPersistenceHandler::DefaultPersistenceHandler");
#ifndef PHP_WIN32
	system_config_file_path = unix_global_config_path;
	user_config_file_path = unix_user_config_path;
	std::size_t pos = user_config_file_path.find_first_of('~');
	user_config_file_path.erase(pos, 1);
	char* home = getenv("HOME");
	if( nullptr == home ) {
		DBG_ERR_FMT("Unable to parse the unix user config path!");
		RAISE_EXCEPTION( err_msg_internal_error );
	} else {
		user_config_file_path.insert(0, home);
	}
#else
	char* buffer;
	char* program_data = getenv("PROGRAMDATA");
	char* app_data = getenv("APPDATA");
	mnd_sprintf(&buffer, MAX_PATH, "%s/MySQLsessions.json", program_data);
	if( nullptr != buffer ) {
		system_config_file_path = buffer;
		mnd_efree( buffer );
	}
	mnd_sprintf(&buffer, MAX_PATH, "%s/MySQLsessions.json", app_data);
	if( nullptr != buffer ) {
		user_config_file_path = buffer;
		mnd_efree( buffer );
	}
#endif
	DBG_INF_FMT("Using the followig paths, user:%s, system:%s",
				user_config_file_path.c_str(),
				system_config_file_path.c_str());
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ DefaultPersistenceHandler::~DefaultPersistenceHandler */
Default_persistence_handler::~Default_persistence_handler()
{
}
/* }}} */


/* {{{ DefaultPersistenceHandler::store */
std::size_t Default_persistence_handler::store(const session_json_defs& session)
{
	DBG_ENTER("DefaultPersistenceHandler::store");
	std::size_t bytes_written{ 0 };
	php_stream* data_stream = open_file( user_config_file_path,
										 "w"); //Truncate the content!!
	if( data_stream ) {
		/*
		 * Left the file empty if no session
		 * should be stored
		 */
		if( false == session.empty() ) {
			 /*
			  * Prepare the buffer and write the data
			  */
			phputils::string data_buffer;
			data_buffer += "{\n";
			std::size_t cnt{ session.size() };
			for( auto& se : session )
			{
				data_buffer += "\"" + se.first + "\":";
				data_buffer += se.second;
				if( --cnt ) {
					data_buffer += ",";
				}
				data_buffer += "\n";
			}
			data_buffer += "}";

			bytes_written = php_stream_write( data_stream,
							  data_buffer.c_str(),
							  data_buffer.size() );
			DBG_INF_FMT("Bytes written to file: %d",
						bytes_written);
		}
		php_stream_close( data_stream );
	} else {
		DBG_ERR_FMT("Not able to store on file! File not opened");
	}
	DBG_RETURN( bytes_written );
}
/* }}} */


/* {{{ DefaultPersistenceHandler::load */
raw_loaded_sessions Default_persistence_handler::load()
{
	DBG_ENTER("DefaultPersistenceHandler::load");
	raw_loaded_sessions global_sessions, sessions;
	/*
	 * User configuration data shall
	 * overrided the configuration in the system
	 * conf file
	 */
	global_sessions = load_impl( system_config_file_path );
	sessions = load_impl( user_config_file_path );
	/*
	 * When inserting, already existing elements have precedence
	 * during the merge, this means that every configuration
	 * in the user file will not be overwtitten by the one in the system file
	 */
	sessions.insert( global_sessions.begin(), global_sessions.end() );
	DBG_INF_FMT("Number of loaded session: %d",
				sessions.size());
	DBG_RETURN( sessions );
}
/* }}} */


/* {{{ DefaultPersistenceHandler::open_file */
php_stream* Default_persistence_handler::open_file( const phputils::string& filename,
												const phputils::string& mode )
{
	DBG_ENTER("DefaultPersistenceHandler::open_file");
	DBG_INF_FMT("File:%s, mode:%s",
				filename.c_str(),
				mode.c_str());
	php_stream* stream = php_stream_open_wrapper(
				filename.c_str(),
				mode.c_str(),
				USE_PATH,
				nullptr);
	if( stream == nullptr ) {
		DBG_ERR_FMT("Unable to open/create the file %s",
					filename.c_str() );
	}
	DBG_RETURN( stream );
}
/* }}} */


/* {{{ DefaultPersistenceHandler::load_impl */
raw_loaded_sessions Default_persistence_handler::load_impl(const phputils::string& filename)
{
	DBG_ENTER("DefaultPersistenceHandler::load_impl");
	raw_loaded_sessions sessions;
	php_stream* data_stream = open_file( filename.c_str(),
										 "r" );
	if( data_stream ) {
		std::size_t size;
		php_stream_seek( data_stream, 0, SEEK_END );
		size = php_stream_tell( data_stream );
		DBG_INF_FMT("Preparing to read %d bytes from %s",
					size,
					filename.c_str());
		php_stream_seek( data_stream, 0, SEEK_SET );
		if( size > 0 ) {
			char* buffer = new char[ size + 1 ];
			memset( buffer, 0, size + 1);
			std::size_t amount = php_stream_read( data_stream, buffer, size );
			if( amount > 0 ) {
				DBG_INF_FMT("Reading complete, bytes: %d",
							amount);
				zval decoded_json;

				php_json_decode_ex( &decoded_json,
									buffer,
									amount,
									PHP_JSON_OBJECT_AS_ARRAY,
									PHP_JSON_PARSER_DEFAULT_DEPTH );
				if( ZVAL_IS_NULL( &decoded_json ) ) {
					DBG_ERR_FMT("Unable to parse the session configuration file!");
				} else {
					zend_string * key;
					zval * val;
					ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(decoded_json), key, val) {
						zval dup_val;
						ZVAL_COPY( &dup_val, val );
						sessions[ ZSTR_VAL( key ) ] = dup_val;
					} ZEND_HASH_FOREACH_END();

					zval_dtor( &decoded_json );
				}
			}
			delete[] buffer;
		}
		php_stream_close( data_stream );
	}
	DBG_RETURN( sessions );
}

} //devapi

} //mysqlx
