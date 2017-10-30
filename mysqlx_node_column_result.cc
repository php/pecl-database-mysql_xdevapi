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
#include "php_api.h"
extern "C" {
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_structs.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_stmt.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result.h"
#include "xmysqlnd/xmysqlnd_node_stmt_result_meta.h"
#include "xmysqlnd/xmysqlnd_rowset.h"
#include "xmysqlnd/xmysqlnd_rowset_buffered.h"
#include "xmysqlnd/xmysqlnd_rowset_fwd.h"
#include "xmysqlnd/xmysqlnd_warning_list.h"
#include "xmysqlnd/xmysqlnd_stmt_execution_state.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_enum_n_def.h"
#include "mysqlx_warning.h"
#include "mysqlx_node_row_result_iterator.h"
#include "mysqlx_node_row_result.h"
#include "mysqlx_node_base_result.h"
#include "mysqlx_field_metadata.h"
#include "mysqlx_node_column_result.h"
#include "mysqlx_exception.h"
#include "phputils/allocator.h"
#include "phputils/object.h"
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

static zend_class_entry *mysqlx_node_column_result_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_schema_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_table_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_table_label,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_column_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_column_label,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_length,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_fractional_digits,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_is_number_signed,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_collation_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_character_set_name,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_get_type,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_node_column_result_is_padded,
					   0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

#define MYSQLX_FETCH_NODE_COLUMN_RESULT_FROM_ZVAL(_to, _from) \
{ \
	const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (struct st_mysqlx_node_column_result *) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(NULL, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

/* {{{ mysqlx_node_column_result::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, __construct)
{
}
/* }}} */

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

/* {{{ get_column_meta_field */
static uint64_t
get_column_type(const struct st_xmysqlnd_result_field_meta * const meta)
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
					mysqlnd_find_charset_nr(meta->collation);
			if (set == nullptr) {
				RAISE_EXCEPTION(10001,"Unable to extract metadata");
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
		RAISE_EXCEPTION(err_msg_meta_fail);
		break;
	}
}
/* }}} */

/* {{{ get_column_meta_field */
static zend_bool
is_type_signed(const struct st_xmysqlnd_result_field_meta * const meta)
{
	zend_bool is_signed = FALSE;
	switch(meta->type) {
	case XMYSQLND_TYPE_SIGNED_INT:
		is_signed = TRUE;
		break;
	case XMYSQLND_TYPE_FLOAT:
	case XMYSQLND_TYPE_DOUBLE:
	case XMYSQLND_TYPE_DECIMAL:
		{
			if(! (meta->flags_set && meta->flags & ALL_UNSIGNED) ) {
				is_signed = TRUE;
			}
		}
		break;
	default:
		break;
	}
	return is_signed;
}
/* }}} */

namespace {

/* {{{ get_column_length */
void
get_column_length(zval* return_value, std::uint32_t length)
{
#if SIZEOF_ZEND_LONG==4
	if (std::numeric_limits<zend_long>::max() < length) {
		ZVAL_DOUBLE(return_value, length);
	} else
#endif /* #if SIZEOF_LONG==4 */
	{
		ZVAL_LONG(return_value, length);
	}
}
/* }}} */

} // anonymous namespace

/* {{{ get_column_meta_field */
static void
get_column_meta_field(INTERNAL_FUNCTION_PARAMETERS,
					meta_fields selected_meta_field)
{
	zval * object_zv;
	struct st_mysqlx_node_column_result * object;
	DBG_ENTER("get_column_meta_field");

	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
								&object_zv,
								mysqlx_node_column_result_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_NODE_COLUMN_RESULT_FROM_ZVAL(object, object_zv);

	RETVAL_FALSE;

	if (object->meta) {
		switch(selected_meta_field) {
		case schema_name:
			ZVAL_STRINGL(return_value,object->meta->schema.s,
						 object->meta->schema.l);
			break;
		case table_name:
			ZVAL_STRINGL(return_value,object->meta->original_table.s,
						 object->meta->original_table.l);
			break;
		case table_label:
			ZVAL_STRINGL(return_value,object->meta->table.s,
						 object->meta->table.l);
			break;
		case column_name:
			ZVAL_STRINGL(return_value,object->meta->original_name.s,
						 object->meta->original_name.l);
			break;
		case column_label:
			ZVAL_STRINGL(return_value,object->meta->name.s,
						 object->meta->name.l);
			break;
		case type:
			ZVAL_LONG(return_value,get_column_type(object->meta));
			break;
		case length:
			get_column_length(return_value, object->meta->length);
			break;
		case fractional_digit:
			ZVAL_LONG(return_value,object->meta->fractional_digits);
			break;
		case is_number_signed:
			ZVAL_LONG(return_value,is_type_signed(object->meta));
			break;
		case collation_name:
		case characterset_name:
			{
				const struct st_mysqlnd_charset * set =
						mysqlnd_find_charset_nr(object->meta->collation);
				if( set != NULL && set->collation != NULL ) {
					if( selected_meta_field == collation_name ) {
						ZVAL_STRINGL(return_value,
									 set->collation,
									 strlen(set->collation));
					} else {
						ZVAL_STRINGL(return_value,
									 set->name,
									 strlen(set->name));
					}
				}
				else {
					ZVAL_NULL(return_value);
				}

			}
			break;
		case is_padded:
			{
				zend_bool is_padded = FALSE;
				if( object->meta->type == XMYSQLND_TYPE_BYTES &&
					(object->meta->flags_set && object->meta->flags & BYTES_RIGHTPAD)) {
					is_padded = TRUE;
				}
				ZVAL_LONG(return_value,is_padded);
			}
			break;
		default:
			RAISE_EXCEPTION(err_msg_meta_fail);
			break;
		}


	} else {
		RAISE_EXCEPTION(err_msg_meta_fail);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getSchemaName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getSchemaName)
{
	DBG_ENTER("mysqlx_node_column_result::getSchemaName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				schema_name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getTableName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getTableName)
{
	DBG_ENTER("mysqlx_node_column_result::getTableName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				table_name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getTableLabel */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getTableLabel)
{
	DBG_ENTER("mysqlx_node_column_result::getTableLabel");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				table_label);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getColumnName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getColumnName)
{
	DBG_ENTER("mysqlx_node_column_result::getColumnName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				column_name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getColumnLabel */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getColumnLabel)
{
	DBG_ENTER("mysqlx_node_column_result::getColumnLabel");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				column_label);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getLength */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getLength)
{
	DBG_ENTER("mysqlx_node_column_result::getLength");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				length);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getFractionalDigits */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getFractionalDigits)
{
	DBG_ENTER("mysqlx_node_column_result::getFractionalDigits");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				fractional_digit);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::isNumberSigned */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, isNumberSigned)
{
	DBG_ENTER("mysqlx_node_column_result::isNumberSigned");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				is_number_signed);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getCollationName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getCollationName)
{
	DBG_ENTER("mysqlx_node_column_result::getCollationName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				collation_name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getCharacterSetName */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getCharacterSetName)
{
	DBG_ENTER("mysqlx_node_column_result::getCharacterSetName");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				characterset_name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::getType */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, getType)
{
	DBG_ENTER("mysqlx_node_column_result::getType");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				type);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_node_column_result::isPadded */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_node_column_result, isPadded)
{
	DBG_ENTER("mysqlx_node_column_result::isPadded");
	get_column_meta_field(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				is_padded);
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlx_node_column_methods[] */
static const zend_function_entry mysqlx_node_column_result_methods[] = {
	PHP_ME(mysqlx_node_column_result,
		__construct,
		NULL,	ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_node_column_result,
		getSchemaName,
		arginfo_mysqlx_node_column_result_get_schema_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getTableName,
		arginfo_mysqlx_node_column_result_get_table_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getTableLabel,
		arginfo_mysqlx_node_column_result_get_table_label,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getColumnName,
		arginfo_mysqlx_node_column_result_get_column_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getColumnLabel,
		arginfo_mysqlx_node_column_result_get_column_label,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getLength,
		arginfo_mysqlx_node_column_result_get_length,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getFractionalDigits,
		arginfo_mysqlx_node_column_result_get_fractional_digits,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		isNumberSigned,
		arginfo_mysqlx_node_column_result_is_number_signed,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getCollationName,
		arginfo_mysqlx_node_column_result_get_collation_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getCharacterSetName,
		arginfo_mysqlx_node_column_result_get_character_set_name,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		getType,
		arginfo_mysqlx_node_column_result_get_type,
		ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_node_column_result,
		isPadded,
		arginfo_mysqlx_node_column_result_is_padded,
		ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */


static zend_object_handlers mysqlx_object_node_column_result_handlers;
static HashTable mysqlx_node_column_result_properties;

const struct st_mysqlx_property_entry mysqlx_node_column_result_property_entries[] =
{
	{{NULL,	0}, NULL, NULL}
};

} // anonymous namespace

/* {{{ mysqlx_new_column_result */
void
mysqlx_new_column_result(
	zval * return_value,
	const st_xmysqlnd_result_field_meta * meta)
{
	DBG_ENTER("mysqlx_new_column");

	if (SUCCESS == object_init_ex(return_value, mysqlx_node_column_result_class_entry) &&
			IS_OBJECT == Z_TYPE_P(return_value)) {
		const struct st_mysqlx_object * const mysqlx_object = Z_MYSQLX_P(return_value);
		struct st_mysqlx_node_column_result * const object =
				(struct st_mysqlx_node_column_result *) mysqlx_object->ptr;
		if (object) {
			object->meta = meta;
		} else {
			php_error_docref(NULL, E_WARNING, "invalid object of class %s",
							 ZSTR_VAL(mysqlx_object->zo.ce->name));
			zval_ptr_dtor(return_value);
			ZVAL_NULL(return_value);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ php_mysqlx_column_result_object_allocator */
static zend_object *
php_mysqlx_column_result_object_allocator(zend_class_entry * class_type)
{
	DBG_ENTER("php_mysqlx_column_result_object_allocator");
	st_mysqlx_object* mysqlx_object = phputils::alloc_object<st_mysqlx_node_column_result>(
		class_type,
		&mysqlx_object_node_column_result_handlers,
		&mysqlx_node_column_result_properties);
	DBG_RETURN(&mysqlx_object->zo);
}
/* }}} */


/* {{{ mysqlx_node_column_result_free_storage */
static void
mysqlx_node_column_result_free_storage(zend_object * object)
{
	struct st_mysqlx_object * mysqlx_object = mysqlx_fetch_object_from_zo(object);
	struct st_mysqlx_node_column_result * inner_obj = (
				struct st_mysqlx_node_column_result *) mysqlx_object->ptr;

	if (inner_obj) {
		//Do not delete meta, that's someone else responsability
		inner_obj->meta = NULL;
		mnd_efree(inner_obj);
	}

	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ mysqlx_register_column_result_class */
void
mysqlx_register_node_column_result_class(INIT_FUNC_ARGS,
						zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_node_column_result_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_node_column_result_handlers.free_obj =
			mysqlx_node_column_result_free_storage;
	{
		zend_class_entry tmp_ce;
		INIT_NS_CLASS_ENTRY(tmp_ce,
					"mysql_xdevapi",
					"ColumnResult",
					mysqlx_node_column_result_methods);
		tmp_ce.create_object = php_mysqlx_column_result_object_allocator;

		mysqlx_node_column_result_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_node_column_result_properties,
				   0, NULL, mysqlx_free_property_cb, 1);

	/* Add name + getter + setter to the hash table with the properties for the class */
	mysqlx_add_properties(&mysqlx_node_column_result_properties,
					mysqlx_node_column_result_property_entries);
}
/* }}} */


/* {{{ mysqlx_unregister_node_column_result_class */
void
mysqlx_unregister_node_column_result_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_node_column_result_properties);
}
/* }}} */

} // namespace devapi

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
