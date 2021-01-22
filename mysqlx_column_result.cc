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
  | Authors: Filip Janiszewski <fjanisze@php.net>                        |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "xmysqlnd/xmysqlnd_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_rowset.h"
#include "xmysqlnd/xmysqlnd_rowset_buffered.h"
#include "xmysqlnd/xmysqlnd_rowset_fwd.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_warning.h"
#include "mysqlx_row_result_iterator.h"
#include "mysqlx_row_result.h"
#include "mysqlx_base_result.h"
#include "mysqlx_column_result.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/object.h"
#include "util/functions.h"
#include <limits>

namespace mysqlx {

namespace devapi {

namespace {

using namespace drv;

enum column_metadata_content_type {
	CT_PLAIN =    0x0000,
	CT_GEOMETRY = 0x0001,
	CT_JSON =     0x0002,
	CT_XML =      0x0003
};

enum column_metadata_flags {
	UINT_ZEROFILL         = 0x0001, // UINT zerofill
	DOUBLE_UNSIGNED       = 0x0001, // DOUBLE = 0x0001 unsigned
	FLOAT_UNSIGNED        = 0x0001, // FLOAT  = 0x0001 unsigned
	DECIMAL_UNSIGNED      = 0x0001, // DECIMAL = 0x0001 unsigned
	ALL_UNSIGNED          = 0x0001,
	BYTES_RIGHTPAD        = 0x0001, // BYTES  = 0x0001 rightpad
	DATETIME_TIMESTAMP    = 0x0001, // DATETIME = 0x0001 timestamp

	NOT_NULL              = 0x0010,
	PRIMARY_KEY           = 0x0020,
	UNIQUE_KEY            = 0x0040,
	MULTIPLE_KEY          = 0x0080,
	AUTO_INCREMENT        = 0x0100
};

static zend_class_entry *mysqlx_column_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result__construct, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_schema_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_table_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_table_label,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_column_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_column_label,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_length,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_fractional_digits,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_is_number_signed,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_collation_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_character_set_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_get_type,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_column_result_is_padded,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

typedef enum
{
	schema_name,
	table_name,
	table_label,
	column_name,
	column_label,
	type,
	length,
	fractional_digit,
	is_number_signed,
	collation_name,
	characterset_name,
	is_padded
} meta_fields;

static uint64_t int_type_mappings[] = {
	0,0,0,
	FIELD_TYPE_TINY, //3
	FIELD_TYPE_TINY, //4
	FIELD_TYPE_SMALLINT,//5
	FIELD_TYPE_SMALLINT,//6
	0,
	FIELD_TYPE_MEDIUMINT,//8
	FIELD_TYPE_MEDIUMINT,//9
	FIELD_TYPE_INT,//10
	FIELD_TYPE_INT//11
};

uint64_t
get_column_type(const st_xmysqlnd_result_field_meta* const meta)
{
	switch(meta->type) {
	case XMYSQLND_TYPE_SIGNED_INT:
	case XMYSQLND_TYPE_UNSIGNED_INT:
		{
			if(meta->length <= 11)
				return int_type_mappings[meta->length];
			return FIELD_TYPE_BIGINT;
		}
		break;
	case XMYSQLND_TYPE_FLOAT:
		return FIELD_TYPE_FLOAT;
	case XMYSQLND_TYPE_DOUBLE:
		return FIELD_TYPE_DOUBLE;
	case XMYSQLND_TYPE_DECIMAL:
		return FIELD_TYPE_DECIMAL;
	case XMYSQLND_TYPE_BYTES:
		{
			if(meta->content_type == CT_JSON)
				return FIELD_TYPE_JSON;
			else if(meta->content_type == CT_GEOMETRY)
				return FIELD_TYPE_GEOMETRY;
			const st_mysqlnd_charset * set =
					mysqlnd_find_charset_nr(static_cast<unsigned int>(meta->collation));
			if (set == nullptr) {
				throw util::xdevapi_exception(util::xdevapi_exception::Code::meta_fail);
			} else if (std::strcmp(set->collation, "binary")) {
				return FIELD_TYPE_BYTES;
			} else {
				return FIELD_TYPE_STRING;
			}
		}
		break;
	case XMYSQLND_TYPE_TIME:
		return FIELD_TYPE_TIME;
	case XMYSQLND_TYPE_DATETIME:
		{
			if(meta->length == 10)
				return FIELD_TYPE_DATE;
			else if(meta->length == 19 &&
				!(meta->flags_set && meta->flags & DATETIME_TIMESTAMP))
				return FIELD_TYPE_DATETIME;
			else
				return FIELD_TYPE_TIMESTAMP;
		}
		break;
	case XMYSQLND_TYPE_SET:
		return FIELD_TYPE_SET;
	case XMYSQLND_TYPE_ENUM:
		return FIELD_TYPE_ENUM;
	case XMYSQLND_TYPE_BIT:
		return FIELD_TYPE_BIT;
	case XMYSQLND_TYPE_NONE:
	default:
		throw util::xdevapi_exception(util::xdevapi_exception::Code::meta_fail);
	}
}

bool
is_type_signed(const st_xmysqlnd_result_field_meta* const meta)
{
	switch(meta->type) {
	case XMYSQLND_TYPE_SIGNED_INT:
		return true;

	case XMYSQLND_TYPE_FLOAT:
	case XMYSQLND_TYPE_DOUBLE:
	case XMYSQLND_TYPE_DECIMAL:
		return !(meta->flags_set && (meta->flags & ALL_UNSIGNED));

	default:
		return false;
	}
}

util::zvalue
get_column_length(std::uint32_t length)
{
#if SIZEOF_ZEND_LONG==4
	if (static_cast<std::uint32_t>(std::numeric_limits<zend_long>::max()) < length) {
		return static_cast<double>(length);
	} else
#endif /* #if SIZEOF_LONG==4 */
	{
		return static_cast<zend_long>(length);
	}
}

void
get_column_meta_field(INTERNAL_FUNCTION_PARAMETERS,
					meta_fields selected_meta_field)
{
	DBG_ENTER("get_column_meta_field");
	util::raw_zval* object_zv{nullptr};
	if (FAILURE == util::get_method_arguments(execute_data, getThis(), "O",
								&object_zv,
								mysqlx_column_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	RETVAL_FALSE;

	auto& data_object{ util::fetch_data_object<st_mysqlx_column_result>(object_zv) };
	const drv::st_xmysqlnd_result_field_meta* meta{ data_object.meta };
	util::zvalue result;
	if (meta) {
		switch(selected_meta_field) {
		case schema_name:
			result = meta->schema;
			break;
		case table_name:
			result = meta->original_table;
			break;
		case table_label:
			result = meta->table;
			break;
		case column_name:
			result = meta->original_name;
			break;
		case column_label:
			result = meta->name;
			break;
		case type:
			result = get_column_type(meta);
			break;
		case length:
			result = get_column_length(meta->length);
			break;
		case fractional_digit:
			result = meta->fractional_digits;
			break;
		case is_number_signed:
			result = is_type_signed(meta);
			break;
		case collation_name:
		case characterset_name:
			{
				const st_mysqlnd_charset* set =
					mysqlnd_find_charset_nr(static_cast<unsigned int>(meta->collation));
				if( set != nullptr && set->collation != nullptr ) {
					if( selected_meta_field == collation_name ) {
						result = set->collation;
					} else {
						result = set->name;
					}
				}
				else {
					result = nullptr;
				}

			}
			break;
		case is_padded:
			{
				if( meta->type == XMYSQLND_TYPE_BYTES &&
					(meta->flags_set && meta->flags & BYTES_RIGHTPAD)) {
					result = true;
				} else {
					result = false;
				}
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_meta_fail);
			break;
		}
	} else {
		RAISE_EXCEPTION(err_msg_meta_fail);
	}
	result.move_to(return_value);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getSchemaName)
{
	DBG_ENTER("mysqlx_column_result::getSchemaName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				schema_name);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getTableName)
{
	DBG_ENTER("mysqlx_column_result::getTableName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				table_name);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getTableLabel)
{
	DBG_ENTER("mysqlx_column_result::getTableLabel");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				table_label);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getColumnName)
{
	DBG_ENTER("mysqlx_column_result::getColumnName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				column_name);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getColumnLabel)
{
	DBG_ENTER("mysqlx_column_result::getColumnLabel");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				column_label);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getLength)
{
	DBG_ENTER("mysqlx_column_result::getLength");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				length);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getFractionalDigits)
{
	DBG_ENTER("mysqlx_column_result::getFractionalDigits");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				fractional_digit);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, isNumberSigned)
{
	DBG_ENTER("mysqlx_column_result::isNumberSigned");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				is_number_signed);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getCollationName)
{
	DBG_ENTER("mysqlx_column_result::getCollationName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				collation_name);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getCharacterSetName)
{
	DBG_ENTER("mysqlx_column_result::getCharacterSetName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				characterset_name);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, getType)
{
	DBG_ENTER("mysqlx_column_result::getType");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				type);
	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_column_result, isPadded)
{
	DBG_ENTER("mysqlx_column_result::isPadded");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				is_padded);
	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_column_result_methods[] = {
	PHP_ME(mysqlx_column_result,
		__construct,
		arginfo_mysqlx_column_result__construct,
		ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_column_result,
		getSchemaName,
		arginfo_mysqlx_column_result_get_schema_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getTableName,
		arginfo_mysqlx_column_result_get_table_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getTableLabel,
		arginfo_mysqlx_column_result_get_table_label,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getColumnName,
		arginfo_mysqlx_column_result_get_column_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getColumnLabel,
		arginfo_mysqlx_column_result_get_column_label,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getLength,
		arginfo_mysqlx_column_result_get_length,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getFractionalDigits,
		arginfo_mysqlx_column_result_get_fractional_digits,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		isNumberSigned,
		arginfo_mysqlx_column_result_is_number_signed,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getCollationName,
		arginfo_mysqlx_column_result_get_collation_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getCharacterSetName,
		arginfo_mysqlx_column_result_get_character_set_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		getType,
		arginfo_mysqlx_column_result_get_type,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_column_result,
		isPadded,
		arginfo_mysqlx_column_result_is_padded,
		ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};

static zend_object_handlers mysqlx_object_column_result_handlers;
static HashTable mysqlx_column_result_properties;

const st_mysqlx_property_entry mysqlx_column_result_property_entries[] =
{
	{std::string_view{}, nullptr, nullptr}
};

} // anonymous namespace

util::zvalue
create_column_result(
	const st_xmysqlnd_result_field_meta* meta)
{
	DBG_ENTER("create_column_result");

	util::zvalue column_result;
	st_mysqlx_column_result& data_object{
		util::init_object<st_mysqlx_column_result>(mysqlx_column_result_class_entry, column_result) };
	data_object.meta = meta;

	DBG_RETURN(column_result);
}

static zend_object *
php_mysqlx_column_result_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_column_result_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<st_mysqlx_column_result>(
		class_type,
		&mysqlx_object_column_result_handlers,
		&mysqlx_column_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

static void
mysqlx_column_result_free_storage(zend_object * object)
{
	util::free_object<st_mysqlx_column_result>(object);
}

void
mysqlx_register_column_result_class(UNUSED_INIT_FUNC_ARGS,
						zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		mysqlx_column_result_class_entry,
		"ColumnResult",
		mysqlx_std_object_handlers,
		mysqlx_object_column_result_handlers,
		php_mysqlx_column_result_object_allocator,
		mysqlx_column_result_free_storage,
		mysqlx_column_result_methods,
		mysqlx_column_result_properties,
		mysqlx_column_result_property_entries);
}

void
mysqlx_unregister_column_result_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_column_result_properties);
}

} // namespace devapi

} // namespace mysqlx
