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
#include <ext/json/php_json.h>
#include <ext/json/php_json_parser.h>
}

#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_session.h"
#include "mysqlx_node_session_configuration.h"
#include "phputils/object.h"
#include "xmysqlnd/xmysqlnd_session_config.h"
#include "mysqlx_class_properties.h"

namespace mysqlx {

namespace devapi {


zend_class_entry *mysqlx_node_session_config_class_entry;

bool istanceof_session_config(zval *object)
{
	return 	TRUE == instanceof_function( Z_OBJCE_P(object),
			mysqlx_node_session_config_class_entry );
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_get_name, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_set_uri, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_get_uri, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_set_app_data, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, value, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_get_app_data, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_delete_app_data, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_session_config::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, __construct)
{
}
/* }}} */

namespace
{

/*
 * Some of the function which are extracting or manipulating
 * information from Session_config are mostly identical,
 * the common implementation is here to avoid duplication
 *
 */
enum class required_field {
	name,
	get_uri,
	set_uri,
	set_app_data,
	get_app_data,
	del_app_data
};

/* {{{ session_config_common_ops */
void session_config_common_ops( INTERNAL_FUNCTION_PARAMETERS,
							required_field field )
{
	zval * object_zv;
	MYSQLND_CSTRING input_string_1 = { nullptr, 0 };
	MYSQLND_CSTRING input_string_2 = { nullptr, 0 };
	phputils::string input_format_string = "O";
	if( field == required_field::get_app_data ||
		field == required_field::del_app_data ||
		field == required_field::set_uri ) {
		input_format_string += "s";
	} else if( field == required_field::set_app_data ) {
		input_format_string += "ss";
	}

	DBG_ENTER("session_config_common_ops");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(),
									getThis(),
									input_format_string.c_str(),
									&object_zv,
									mysqlx_node_session_config_class_entry,
									&(input_string_1.s), &(input_string_1.l),
									&(input_string_2.s), &(input_string_2.l)))
	{
		DBG_VOID_RETURN;
	}

	Session_config& config = phputils::fetch_data_object<Session_config>(object_zv);
	RETVAL_FALSE;

	if( false == config.get_name().empty() ) {
		switch( field ) {
		case required_field::name:
			RETVAL_STRINGL(config.get_name().c_str(),
					config.get_name().size());
			break;
		case required_field::get_uri:
			RETVAL_STRINGL(config.get_uri().c_str(),
					config.get_uri().size());
			break;
		case required_field::set_uri:
			config.set_uri( input_string_1.s );
			break;
		case required_field::set_app_data:
			config.set_app_data( input_string_1.s, input_string_2.s );
			RETVAL_TRUE;
			break;
		case required_field::get_app_data:
			{
				phputils::string data = config.get_app_data( input_string_1.s );
				if( false == data.empty() ) {
					RETVAL_STRINGL(data.c_str(),
							data.size());
				} else {
					ZVAL_NULL( return_value );
				}
			}
			break;
		case required_field::del_app_data:
			if( config.delete_app_data( input_string_1.s ) ) {
				RETVAL_TRUE;
			} else {
				RETVAL_FALSE;
			}
			break;
		default:
			DBG_ERR_FMT("Asking for an unknown field!.");
			RAISE_EXCEPTION( err_msg_internal_error );
			break;
		}
	} else {
		DBG_ERR_FMT("The Session_config is not configured properly!");
		RAISE_EXCEPTION( err_msg_internal_error );
	}

	DBG_VOID_RETURN;
}
/* }}} */
}


/* {{{ mysqlx_session_config::getName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, getName)
{
	DBG_ENTER("mysqlx_session_config::getName");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::name );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config::getUri */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, getUri)
{
	DBG_ENTER("mysqlx_session_config::getUri");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::get_uri );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config::setUri */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, setUri)
{
	DBG_ENTER("mysqlx_session_config::setUri");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::set_uri );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config::setAppData */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, setAppData)
{
	DBG_ENTER("mysqlx_session_config::setAppData");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::set_app_data );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config::getAppData */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, getAppData)
{
	DBG_ENTER("mysqlx_session_config::getAppData");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::get_app_data );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config::deleteAppData */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config, deleteAppData)
{
	DBG_ENTER("mysqlx_session_config::deleteAppData");
	session_config_common_ops( INTERNAL_FUNCTION_PARAM_PASSTHRU,
							required_field::del_app_data );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session_config_methods[] */
const zend_function_entry mysqlx_node_session_config_methods[] = {
	PHP_ME(mysqlx_session_config, __construct, 	arginfo_mysqlx_session_config__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_session_config, getName, 	arginfo_mysqlx_get_name, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config, getUri, 	arginfo_mysqlx_get_uri, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config, setUri, 	arginfo_mysqlx_set_uri, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config, setAppData, 	arginfo_mysqlx_set_app_data, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config, getAppData, 	arginfo_mysqlx_get_app_data, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config, deleteAppData, 	arginfo_mysqlx_delete_app_data, ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */

zend_object_handlers mysqlx_object_node_session_config_handlers;
HashTable mysqlx_node_session_config_properties;

const st_mysqlx_property_entry mysqlx_node_session_config_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_session_config_free_storage */
void
mysqlx_node_session_config_free_storage(zend_object * object)
{
	st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	Session_config* inner_obj = static_cast<Session_config*>( mysqlx_object->ptr );

	delete inner_obj;

	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_session_config_object_allocator */
zend_object *
php_mysqlx_node_session_config_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_session_config_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Session_config>(
		class_type,
		&mysqlx_object_node_session_config_handlers,
		&mysqlx_node_session_config_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */

/* {{{ mysqlx_register_node_session_config_class */
void
mysqlx_register_node_session_config_class(INIT_FUNC_ARGS,
				zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
				mysqlx_node_session_config_class_entry,
				"SessionConfig",
				mysqlx_std_object_handlers,
				mysqlx_object_node_session_config_handlers,
				php_mysqlx_node_session_config_object_allocator,
				mysqlx_node_session_config_free_storage,
				mysqlx_node_session_config_methods,
				mysqlx_node_session_config_properties,
				mysqlx_node_session_config_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_session_config_class */
void
mysqlx_unregister_node_session_config_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_session_config_properties);
}
/* }}} */


/* {{{ mysqlx_new_session_config */
enum_func_status
mysqlx_new_session_config(zval * return_value)
{
	DBG_ENTER("mysqlx_new_session_config");
	DBG_RETURN(SUCCESS == object_init_ex(return_value,
						mysqlx_node_session_config_class_entry)? PASS:FAIL);
}
/* }}} */


/* {{{ get_session_config */

void get_session_config(zval * session_zval,
						const Session_config& config)
{
	DBG_ENTER("get_session_config");
	if (PASS == mysqlx::devapi::mysqlx_new_session_config(session_zval)) {
		const st_mysqlx_object*const mysqlx_object = Z_MYSQLX_P(session_zval);
		Session_config* inner_obj = static_cast<Session_config*>( mysqlx_object->ptr );
		*inner_obj = config;
	} else {
		zval_ptr_dtor(session_zval);
		ZVAL_NULL(session_zval);
	}
	DBG_VOID_RETURN;
}
/* }}} */


 zend_class_entry *mysqlx_node_session_config_manager_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager_get, 0, ZEND_RETURN_VALUE, 1)
		ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager_list, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager_delete, 0, ZEND_RETURN_VALUE, 1)
		ZEND_ARG_TYPE_INFO(no_pass_by_ref, name, IS_STRING, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager_update, 0, ZEND_RETURN_VALUE, 1)
		ZEND_ARG_TYPE_INFO(no_pass_by_ref, session, IS_OBJECT, dont_allow_null)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_session_config_manager__save, 0, ZEND_RETURN_VALUE, 1)
		ZEND_ARG_INFO(no_pass_by_ref, input)
ZEND_END_ARG_INFO()


/* {{{ mysqlx_session_config_manager::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, __construct)
{
}
/* }}} */


/* {{{ mysqlx_session_config_manager::get */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, get)
{
	zval * object_zv;
	MYSQLND_CSTRING session_name = { nullptr, 0 };

	DBG_ENTER("mysqlx_session_config_manager::get");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
									&object_zv,
									mysqlx_node_session_config_manager_class_entry,
									&(session_name.s), &(session_name.l)))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	if( nullptr != session_name.s && 0 != session_name.l ) {
		Session_config config = Session_config_manager::get_instance()->get( session_name.s );
		if( false == config.get_name().empty() )
		{
			get_session_config( return_value, config );
		} else {
			DBG_ERR_FMT("Returned an invalid Session_config from get");
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlx_session_config_manager::list */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, list)
{
	zval * object_zv;

	DBG_ENTER("mysqlx_session_config_manager::list");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
									&object_zv,
									mysqlx_node_session_config_manager_class_entry))
	{
		DBG_VOID_RETURN;
	}

	ZVAL_NULL( return_value );

	auto list_of_sessions = Session_config_manager::get_instance()->list();

	if( false == list_of_sessions.empty() ) {
		array_init_size( return_value, list_of_sessions.size() );
		for( auto& name : list_of_sessions ) {
			zval zval_name;
			ZVAL_STRINGL( &zval_name, name.c_str(), name.size() );
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &zval_name);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlx_session_config_manager::delete */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, delete)
{
	zval * object_zv;
	MYSQLND_CSTRING session_name = { nullptr, 0 };

	DBG_ENTER("mysqlx_session_config_manager::delete");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os",
									&object_zv,
									mysqlx_node_session_config_manager_class_entry,
									&(session_name.s), &(session_name.l)))
	{
		DBG_VOID_RETURN;
	}

	if( nullptr == session_name.s || 0 == session_name.l ) {
		RETVAL_FALSE;
	} else {
		bool success = Session_config_manager::get_instance()->remove( session_name.s );

		if( success ) {
			RETVAL_TRUE;
		} else {
			RETVAL_FALSE;
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_session_config_manager::update */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, update)
{
	zval * object_zv;
	zval * session_obj;

	DBG_ENTER("mysqlx_session_config_manager::update");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OO",
									&object_zv,
									mysqlx_node_session_config_manager_class_entry,
									&session_obj,
									mysqlx_node_session_config_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	Session_config& config = phputils::fetch_data_object<Session_config>( session_obj );

	if( true == Session_config_manager::get_instance()->update( config ) ){
		ZVAL_COPY( return_value, session_obj );
	}

	DBG_VOID_RETURN;
}
/* }}} */

namespace
{

/*
 * Handling functions for 'save', each one
 * is specific for the amount of arguments
 * supplied
 */
/* {{{ handle_one_arg_save */
void handle_one_arg_save(INTERNAL_FUNCTION_PARAMETERS,
					zval* input_parameters)
{
	/*
	 * The argument is expected to be an object
	 * of type Session_config.
	 */
	DBG_ENTER("handle_one_arg_save");
	if( Z_TYPE( input_parameters[0] ) != IS_OBJECT ||
		false == istanceof_session_config( &input_parameters[0] ) ) {
		DBG_ERR_FMT("The argument should be an instance of Session_config");
		RAISE_EXCEPTION( err_msg_wrong_param_6 );
	} else {
		Session_config& session_conf = phputils::fetch_data_object<Session_config>( &input_parameters[0] );
		if( session_conf.get_name().empty() ) {
			RAISE_EXCEPTION( err_msg_wrong_param_6 );
		}
		Session_config session = Session_config_manager::get_instance()->save(
					session_conf );
		get_session_config( return_value, session );
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ handle_two_arg_save */
void handle_two_arg_save(INTERNAL_FUNCTION_PARAMETERS,
						 zval* input_parameters)
{
	/*
	 * This can either be:
	 * 1) session name, session json
	 * 2) session name, session URI
	 */
	DBG_ENTER("handle_two_arg_save");
	if( Z_TYPE( input_parameters[0] ) != IS_STRING ||
		Z_TYPE( input_parameters[1] ) != IS_STRING ) {
		RAISE_EXCEPTION( err_msg_wrong_param_6 );
	}
	/*
	 * Attempt to decode the JSON, if it fails then
	 * we assume is an URI. If is not an URI then
	 * the code will fail and raise an exception later
	 */
	Session_config session;
	zval decoded_json;
	php_json_decode_ex( &decoded_json,
						Z_STRVAL( input_parameters[1] ),
						Z_STRLEN( input_parameters[1] ),
						PHP_JSON_OBJECT_AS_ARRAY,
						PHP_JSON_PARSER_DEFAULT_DEPTH );
	if( ZVAL_IS_NULL( &decoded_json ) ) {
		DBG_INF_FMT("The second argument seems to be an URI");
		//Should be an URI
		session = Session_config_manager::get_instance()->save(
					Z_STRVAL( input_parameters[0] ),
					Z_STRVAL( input_parameters[1] ) );
	} else {
		DBG_INF_FMT("The second argument seems to be a JSON");
		//Should be a JSON
		session = Session_config_manager::get_instance()->save(
					Z_STRVAL( input_parameters[0] ),
					&decoded_json );
		zval_dtor(&decoded_json);
	}
	get_session_config( return_value, session );
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ handle_three_arg_save */
void handle_three_arg_save(INTERNAL_FUNCTION_PARAMETERS,
					zval* input_parameters)
{
	/*
	 * The argument is expected to be:
	 * session name, session URI, appData in JSON format
	 */
	DBG_ENTER("handle_three_arg_save");
	Session_config session;
	zval decoded_json;
	php_json_decode_ex( &decoded_json,
						Z_STRVAL( input_parameters[2] ),
						Z_STRLEN( input_parameters[2] ),
						PHP_JSON_OBJECT_AS_ARRAY,
						PHP_JSON_PARSER_DEFAULT_DEPTH );
	if( ZVAL_IS_NULL( &decoded_json ) ) {
		DBG_ERR_FMT("The third argument seems not to be a valid JSON");
		RAISE_EXCEPTION( err_msg_wrong_param_6 );
	} else {
		session = Session_config_manager::get_instance()->save(
					Z_STRVAL( input_parameters[0] ),
					Z_STRVAL( input_parameters[1] ),
					&decoded_json );
		zval_dtor(&decoded_json);
		get_session_config( return_value, session );
	}
	DBG_VOID_RETURN;
}
/* }}} */

}

/* {{{ mysqlx_session_config_manager::save*/
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_session_config_manager, save)
{
	DBG_ENTER("mysqlx_session_config_manager::save");
	zval * object_zv;
	zval * input_parameters{ nullptr };
	int    num_of_parameters{ 0 };

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O+",
									&object_zv,
									mysqlx_node_session_config_manager_class_entry,
									&input_parameters,
									&num_of_parameters))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;
	DBG_INF_FMT("Number of arguments %d",
				num_of_parameters);


	typedef void (*handler_ptr)(INTERNAL_FUNCTION_PARAMETERS,zval*);
	const int max_num_of_arguments{ 3 };
	static handler_ptr save_handlers[ max_num_of_arguments ] = {
		&handle_one_arg_save,
		&handle_two_arg_save,
		&handle_three_arg_save
	};

	if( num_of_parameters > max_num_of_arguments ||
		num_of_parameters <= 0 ) {
		RAISE_EXCEPTION( err_msg_wrong_param_6 );
	}

	save_handlers[ num_of_parameters - 1 ]( INTERNAL_FUNCTION_PARAM_PASSTHRU, input_parameters);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_session_methods[] */
const zend_function_entry mysqlx_node_session_config_manager_methods[] = {
	PHP_ME(mysqlx_session_config_manager, __construct, 	arginfo_mysqlx_session_config_manager__construct, ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_session_config_manager, save, 	arginfo_mysqlx_session_config_manager__save, ZEND_ACC_PUBLIC)

	PHP_ME(mysqlx_session_config_manager, get, 	arginfo_mysqlx_session_config_manager_get, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config_manager, list, 	arginfo_mysqlx_session_config_manager_list, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config_manager, delete, 	arginfo_mysqlx_session_config_manager_delete, ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_session_config_manager, update, 	arginfo_mysqlx_session_config_manager_update, ZEND_ACC_PUBLIC)


	{nullptr, nullptr, nullptr}
};
/* }}} */

zend_object_handlers mysqlx_object_node_session_manager_config_handlers;
HashTable mysqlx_node_session_config_manager_properties;

const st_mysqlx_property_entry mysqlx_node_session_config_manager_property_entries[] =
{
	{{nullptr,	0}, nullptr, nullptr}
};

/* {{{ mysqlx_node_session_config_manager_free_storage */
void
mysqlx_node_session_config_manager_free_storage(zend_object * object)
{
	st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	Session_config* inner_obj = static_cast<Session_config*>( mysqlx_object->ptr );

	/*
	 * Only reasonable way to avoid memory leaks..
	 */
	delete Session_config_manager::get_instance();

	if (inner_obj) {
		delete inner_obj;
	}

	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_node_session_config_manager_object_allocator */
zend_object *
php_mysqlx_node_session_config_manager_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_node_session_config_manager_object_allocator");
	/*
	 * This object is not used anywhere, we force this allocation
	 * to be sure that when the script it turning down the
	 * deallocator for session_config_manager is called, and then
	 * Session_config_manager can be freed
	 */
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<Session_config>(
		class_type,
		&mysqlx_object_node_session_manager_config_handlers,
		&mysqlx_node_session_config_manager_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_register_node_session_config_manager_class */
void
mysqlx_register_node_session_config_manager_class(INIT_FUNC_ARGS,
				zend_object_handlers* mysqlx_std_object_handlers)
{
	mysqlx_object_node_session_manager_config_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_session_manager_config_handlers.free_obj = mysqlx_node_session_config_manager_free_storage;

	zend_class_entry tmp_ce;
	INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "SessionConfigManager",
						mysqlx_node_session_config_manager_methods);
	tmp_ce.create_object = php_mysqlx_node_session_config_manager_object_allocator;
	mysqlx_node_session_config_manager_class_entry = zend_register_internal_class(&tmp_ce);
	zend_class_implements(mysqlx_node_session_config_manager_class_entry, 0);

	zend_hash_init(&mysqlx_node_session_config_manager_properties,
				   0, nullptr, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_session_config_manager_properties,
						  mysqlx_node_session_config_manager_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_session_config_manager_class */
void
mysqlx_unregister_node_session_config_manager_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_session_config_manager_properties);
}
/* }}} */


/* {{{ mysqlx_new_session_config_manager */
enum_func_status
mysqlx_new_session_config_manager(zval * return_value)
{
	DBG_ENTER("mysqlx_new_session_config_manager");
	DBG_RETURN(SUCCESS == object_init_ex(return_value,
						mysqlx_node_session_config_manager_class_entry)? PASS:FAIL);
}
/* }}} */


/* {{{ get_session_config_manager */
void get_session_config_manager(zval * session_zval)
{
	DBG_ENTER("get_session_config_manager");
	if (PASS == mysqlx::devapi::mysqlx_new_session_config_manager(session_zval)) {

	} else {
		zval_ptr_dtor(session_zval);
		ZVAL_NULL(session_zval);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlx\\mysql_xdevapi__sessions() */
PHP_FUNCTION(mysql_xdevapi__sessions)
{
	DBG_ENTER("mysql_xdevapi__sessions");

	get_session_config_manager( return_value );

	DBG_VOID_RETURN;
}

/* }}} */

}

}
