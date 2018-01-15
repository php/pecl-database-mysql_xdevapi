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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <zend_smart_str.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
#include <ext/mysqlnd/mysqlnd_alloc.h>
#include <ext/mysqlnd/mysqlnd_statistics.h>
}
#include <ext/mysqlnd/mysql_float_to_double.h>
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_node_session.h"
#include "php_mysqlx.h"
#include "mysqlx_class_properties.h"

#include "mysqlx_resultset__data_row.h"
#include "mysqlx_resultset__column_metadata.h"
#include "mysqlx_resultset__resultset_metadata.h"

#include "util/object.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

namespace mysqlx {

namespace devapi {

namespace msg {

using namespace drv;

static zend_class_entry *mysqlx_data_row_class_entry;

struct st_mysqlx_data_row
{
	Mysqlx::Resultset::Row message;
	zend_bool persistent;
};

#define MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(_to, _from) \
{ \
	st_mysqlx_object* mysqlx_object = Z_MYSQLX_P((_from)); \
	(_to) = (st_mysqlx_data_row*) mysqlx_object->ptr; \
	if (!(_to)) { \
		php_error_docref(nullptr, E_WARNING, "invalid object of class %s", ZSTR_VAL(mysqlx_object->zo.ce->name)); \
		RETVAL_NULL(); \
		DBG_VOID_RETURN; \
	} \
} \

ZEND_BEGIN_ARG_INFO_EX(mysqlx_data_row__decode, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_TYPE_INFO(0, metadata, IS_OBJECT, 0)
ZEND_END_ARG_INFO()

/* {{{ mysqlx_data_row::__construct */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_data_row, __construct)
{
}
/* }}} */

/* {{{ proto long mysqlx_data_row::decode(object messsage, array metadata) */
MYSQL_XDEVAPI_PHP_METHOD(mysqlx_data_row, decode)
{
	zval * object_zv;
	zval * metadata_zv;
	st_mysqlx_data_row* object;
	st_mysqlx_resultset_metadata* metadata;

	DBG_ENTER("mysqlx_data_row::decode");
	if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OO",
												&object_zv, mysqlx_data_row_class_entry,
												&metadata_zv, mysqlx_resultset_metadata_class_entry))
	{
		DBG_VOID_RETURN;
	}

	MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(object, object_zv);
	MYSQLX_FETCH_MESSAGE__RESULTSET_METADATA_FROM_ZVAL(metadata, metadata_zv);

	RETVAL_FALSE;
	{
		zval * entry;
		const size_t column_count = zend_hash_num_elements(&metadata->resultset_metadata_ht);
		if (!column_count) {
			php_error_docref(nullptr, E_WARNING, "Zero columns");
			DBG_VOID_RETURN;
		}
		//TODO marines
        const size_t max_column_count = 256;
        assert(column_count < max_column_count);
        //const st_mysqlx_column_metadata* meta_ar[column_count];
        const st_mysqlx_column_metadata* meta_ar[max_column_count];
        unsigned int i = 0;
		/* ZEND_HASH_FOREACH_PTR ?? */
		ZEND_HASH_FOREACH_VAL(&metadata->resultset_metadata_ht, entry) {
			if (Z_TYPE_P(entry) == IS_OBJECT && Z_OBJ_P(entry)->ce == mysqlx_column_metadata_class_entry) {
				st_mysqlx_column_metadata* column_entry{nullptr};
				MYSQLX_FETCH_MESSAGE__COLUMN_METADATA_FROM_ZVAL(column_entry, entry);

				const Mysqlx::Resultset::ColumnMetaData & meta = column_entry->message;
				if (!meta.has_type()) {
					php_error_docref(nullptr, E_WARNING, "Type is not set for position %u", i);
					DBG_VOID_RETURN;
				}
				switch (meta.type()) {
					case Mysqlx::Resultset::ColumnMetaData_FieldType_SINT:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_UINT:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_DOUBLE:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_FLOAT:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_BYTES:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_TIME:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_DATETIME:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_SET:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_ENUM:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_BIT:
					case Mysqlx::Resultset::ColumnMetaData_FieldType_DECIMAL:
						break;
					default:
						php_error_docref(nullptr, E_WARNING, "Unknown type %s(%u) for position %u",
										 Mysqlx::Resultset::ColumnMetaData::FieldType_Name(meta.type()).c_str(),
										 meta.type(),
										 i);
						DBG_VOID_RETURN;
				}
				meta_ar[i++] = column_entry;
			}
		} ZEND_HASH_FOREACH_END();

		array_init_size(return_value, column_count);
		for (i = 0; i < column_count; ++i) {
			const Mysqlx::Resultset::ColumnMetaData & meta = meta_ar[i]->message;
			const uint8_t * buf = reinterpret_cast<const uint8_t*>(object->message.field(i).c_str());
			const size_t buf_size = object->message.field(i).size();
			zval zv;
			DBG_INF_FMT("buf_size=%u", (uint) buf_size);
			for (unsigned j{0}; j < buf_size; j++) {
				DBG_INF_FMT("[%02u]=x%02X", j, buf[j]);
			}
			/*
			  Precaution, as if something misbehaves and doesn't initialize `zv` then `zv` will be at
			  the same place in the stack and have the previous value. String reuse will lead to
			  double-free and a crash.
			*/
			ZVAL_NULL(&zv);
			if (buf_size != 0) {
				switch (meta.type()) {
				case Mysqlx::Resultset::ColumnMetaData_FieldType_SINT:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 gval;
					DBG_INF("SINT");
					if (input_stream.ReadVarint64(&gval)) {
						int64_t ival = ::google::protobuf::internal::WireFormatLite::ZigZagDecode64(gval);
#if SIZEOF_ZEND_LONG==4
						if (UNEXPECTED(ival >= ZEND_LONG_MAX)) {
							ZVAL_NEW_STR(&zv, strpprintf(0, MYSQLND_LLU_SPEC, ival));
						} else
#endif
						{
							ZVAL_LONG(&zv, ival);
						}
					} else {
						php_error_docref(nullptr, E_WARNING, "Error decoding SINT");
					}
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_BIT:
					DBG_INF("BIT handled as UINT");
				case Mysqlx::Resultset::ColumnMetaData_FieldType_UINT:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 gval;
					DBG_INF("UINT");
					if (input_stream.ReadVarint64(&gval)) {
#if SIZEOF_ZEND_LONG==8
						if (gval > 9223372036854775807L) {
#elif SIZEOF_ZEND_LONG==4
						if (gval > L64(2147483647)) {
#endif
							ZVAL_NEW_STR(&zv, strpprintf(0, MYSQLND_LLU_SPEC, gval));
						} else {
							ZVAL_LONG(&zv, gval);
						}
					} else {
						php_error_docref(nullptr, E_WARNING, "Error decoding UINT");
					}
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_DOUBLE:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 gval;
					DBG_INF("DOUBLE");
					if (input_stream.ReadLittleEndian64(&gval)) {
						ZVAL_DOUBLE(&zv, ::google::protobuf::internal::WireFormatLite::DecodeDouble(gval));
					} else {
						php_error_docref(nullptr, E_WARNING, "Error decoding DOUBLE");
						ZVAL_NULL(&zv);
					}
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_FLOAT:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint32 gval;
					DBG_INF("FLOAT");
					if (input_stream.ReadLittleEndian32(&gval)) {
						const float fval = ::google::protobuf::internal::WireFormatLite::DecodeFloat(gval);
						const unsigned int fractional_digits = meta.fractional_digits();
#ifndef NOT_FIXED_DEC
# define NOT_FIXED_DEC 31
#endif
						const double dval = mysql_float_to_double(fval, (fractional_digits >= NOT_FIXED_DEC) ? -1 : fractional_digits);

						ZVAL_DOUBLE(&zv, dval);
					} else {
						php_error_docref(nullptr, E_WARNING, "Error decoding FLOAT");
					}
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_ENUM:
					DBG_INF("ENUM handled as STRING");
				case Mysqlx::Resultset::ColumnMetaData_FieldType_BYTES:{
					if (buf_size) {
						DBG_INF("STRING");
						ZVAL_STRINGL(&zv, reinterpret_cast<const char *>(buf), buf_size - 1); /* skip the ending \0 */
					} else {
						DBG_INF("NULL");
						ZVAL_NULL(&zv);
					}
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_TIME:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 neg = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
					DBG_INF("TIME");
					if (!buf_size) {
						break;
					}
					if (buf_size == 1) {
						if (!buf[0]) {
							#define	TIME_NULL_VALUE "00:00:00.00"
							ZVAL_NEW_STR(return_value, zend_string_init(TIME_NULL_VALUE, sizeof(TIME_NULL_VALUE)-1, 0));
							#undef TIME_NULL_VALUE
						} else {
							ZVAL_NULL(&zv);
							php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of TIME", (uint)(buf[0]));
						}
						break;
					}
					do {
						if (!input_stream.ReadVarint64(&neg)) break;		DBG_INF_FMT("neg  =" MYSQLND_LLU_SPEC, neg);
						if (!input_stream.ReadVarint64(&hours)) break;		DBG_INF_FMT("hours=" MYSQLND_LLU_SPEC, hours);
						if (!input_stream.ReadVarint64(&minutes)) break;	DBG_INF_FMT("mins =" MYSQLND_LLU_SPEC, minutes);
						if (!input_stream.ReadVarint64(&seconds)) break;	DBG_INF_FMT("secs =" MYSQLND_LLU_SPEC, seconds);
						if (!input_stream.ReadVarint64(&useconds)) break;	DBG_INF_FMT("usecs=" MYSQLND_LLU_SPEC, useconds);
					} while (0);
					#define TIME_FMT_STR "%s%02u:%02u:%02u.%08u"
					ZVAL_NEW_STR(&zv, strpprintf(0, TIME_FMT_STR , neg? "-":"",
											    (unsigned int) hours,
												(unsigned int) minutes,
												(unsigned int) seconds,
												(unsigned int) useconds));
					#undef TIME_FMT_STR
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_DATETIME:{
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 year = 0, month = 0, day = 0, hours = 0, minutes = 0, seconds = 0, useconds = 0;
					DBG_INF("DATETIME");
					if (!buf_size) {
						break;
					}
					if (buf_size == 1) {
						if (!buf[0]) {
							#define	DATETIME_NULL_VALUE "0000-00-00 00:00:00.00"
							ZVAL_NEW_STR(return_value, zend_string_init(DATETIME_NULL_VALUE, sizeof(DATETIME_NULL_VALUE)-1, 0));
							#undef DATETIME_NULL_VALUE
						} else {
							php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of TIME", (uint)(buf[0]));
						}
						break;
					}
					do {
						if (!input_stream.ReadVarint64(&year)) break; 		DBG_INF_FMT("year =" MYSQLND_LLU_SPEC, year);
						if (!input_stream.ReadVarint64(&month)) break;		DBG_INF_FMT("month=" MYSQLND_LLU_SPEC, month);
						if (!input_stream.ReadVarint64(&day)) break;		DBG_INF_FMT("day  =" MYSQLND_LLU_SPEC, day);
						if (!input_stream.ReadVarint64(&hours)) break;		DBG_INF_FMT("hours=" MYSQLND_LLU_SPEC, hours);
						if (!input_stream.ReadVarint64(&minutes)) break;	DBG_INF_FMT("mins =" MYSQLND_LLU_SPEC, minutes);
						if (!input_stream.ReadVarint64(&seconds)) break;	DBG_INF_FMT("secs =" MYSQLND_LLU_SPEC, seconds);
						if (!input_stream.ReadVarint64(&useconds)) break;	DBG_INF_FMT("usecs=" MYSQLND_LLU_SPEC, useconds);
					} while (0);
					#define DATETIME_FMT_STR "%04u-%02u-%02u %02u:%02u:%02u"
					ZVAL_NEW_STR(&zv, strpprintf(0, DATETIME_FMT_STR ,
												 (unsigned int) year,
												 (unsigned int) month,
												 (unsigned int) day,
												 (unsigned int) hours,
												 (unsigned int) minutes,
												 (unsigned int) seconds,
												 (unsigned int) useconds));
					#undef DATETIME_FMT_STR
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_SET:{
					DBG_INF("SET");
					::google::protobuf::io::CodedInputStream input_stream(buf, buf_size);
					::google::protobuf::uint64 gval;
					bool length_read_ok{true};
					array_init(&zv);
					if (buf_size == 1 && buf[0] == 0x1) { /* Empty set */
						break;
					}
					while (length_read_ok) {
						if ((length_read_ok = input_stream.ReadVarint64(&gval))) {
							char* set_value{nullptr};
							int rest_buffer_size{0};
							if (input_stream.GetDirectBufferPointer((const void**) &set_value, &rest_buffer_size)) {
								zval set_entry;
								DBG_INF_FMT("value length=%3u  rest_buffer_size=%3d", (uint) gval, rest_buffer_size);
								if (gval > rest_buffer_size) {
									php_error_docref(nullptr, E_WARNING, "Length pointing outside of the buffer");
									break;
								}
								ZVAL_STRINGL(&set_entry, set_value, gval);
								zend_hash_next_index_insert(Z_ARRVAL(zv), &set_entry);
								if (!input_stream.Skip(gval)) {
									break;
								}
							}
						}
					}
					DBG_INF_FMT("set elements=%u", zend_hash_num_elements(Z_ARRVAL(zv)));
					break;
				}
				case Mysqlx::Resultset::ColumnMetaData_FieldType_DECIMAL:{
					DBG_INF("DECIMAL");
					if (!buf_size) {
						break;
					}
					if (buf_size == 1) {
						php_error_docref(nullptr, E_WARNING, "Unexpected value %d for first byte of TIME");
					}
					const uint8_t scale = buf[0];
					const uint8_t last_byte = buf[buf_size - 1]; /* last byte is the sign and the last 4 bits, if any */
					const uint8_t sign = ((last_byte & 0xF)? last_byte  : last_byte >> 4) & 0xF;
					const size_t digits = (buf_size - 2 /* scale & last */) * 2  + ((last_byte & 0xF) > 0x9? 1:0);
					DBG_INF_FMT("scale     =%u", (uint) scale);
					DBG_INF_FMT("sign      =%u", (uint) sign);
					DBG_INF_FMT("digits    =%u", (uint) digits);
					if (!digits) {
						php_error_docref(nullptr, E_WARNING, "Wrong value for DECIMAL. scale=%u  last_byte=%u", (uint) scale, last_byte);
						break;
					}
					const size_t d_val_len = digits + (sign == 0xD? 1:0) + (digits > scale? 1:0); /* one for the dot, one for the sign*/
					char * d_val = new char [d_val_len + 1];
					d_val[d_val_len] = '\0';
					char * p = d_val;
					if (sign == 0xD) {
						*(p++) = '-';
					}
					const size_t dot_position = digits - scale - 1;
					for (unsigned int pos = 0; pos < digits; ++pos) {
						const size_t offset = 1 + (pos >> 1);
						/* if uneven (&0x01) then use the second 4-bits, otherwise shift (>>) the first 4 to the right and then use them */
						const uint8_t digit = (pos & 0x01 ? buf[offset] : buf[offset] >> 4) & 0x0F;
						*(p++) = '0' + digit;
						if (pos == dot_position) {
							*(p++) = '.';
						}
					}
					DBG_INF_FMT("value=%*s", d_val_len, d_val);
					ZVAL_STRINGL(&zv, d_val, d_val_len);
					delete [] d_val;
					break;
				}
				}
			}
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &zv);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */



/* {{{ mysqlx_data_row_methods[] */
static const zend_function_entry mysqlx_data_row_methods[] = {
	PHP_ME(mysqlx_data_row, __construct,	nullptr,						ZEND_ACC_PRIVATE)
	PHP_ME(mysqlx_data_row, decode,			mysqlx_data_row__decode,	ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}
};
/* }}} */


/* {{{ mysqlx_column_meta_property_entries[] */
static const struct st_mysqlx_property_entry mysqlx_column_meta_property_entries[] =
{
	{{nullptr, 0}, nullptr, nullptr}
};
/* }}} */



static zend_object_handlers mysqlx_object_data_row_handlers;
static HashTable mysqlx_data_row_properties;

/* {{{ mysqlx_data_row_free_storage */
static void
mysqlx_data_row_free_storage(zend_object * object)
{
	st_mysqlx_object* mysqlx_object = mysqlx_fetch_object_from_zo(object);
	st_mysqlx_data_row* message = (st_mysqlx_data_row*) mysqlx_object->ptr;

	delete message;
	mysqlx_object_free_storage(object);
}
/* }}} */


/* {{{ php_mysqlx_data_row_object_allocator */
static zend_object *
php_mysqlx_data_row_object_allocator(zend_class_entry * class_type)
{
	const zend_bool persistent = FALSE;
	st_mysqlx_object* mysqlx_object = (st_mysqlx_object*) mnd_pecalloc(1, sizeof(struct st_mysqlx_object) + zend_object_properties_size(class_type), persistent);
	st_mysqlx_data_row* message = new (std::nothrow) struct st_mysqlx_data_row();

	DBG_ENTER("php_mysqlx_data_row_object_allocator");
	if ( mysqlx_object && message) {
		mysqlx_object->ptr = message;

		message->persistent = persistent;
		zend_object_std_init(&mysqlx_object->zo, class_type);
		object_properties_init(&mysqlx_object->zo, class_type);

		mysqlx_object->zo.handlers = &mysqlx_object_data_row_handlers;
		mysqlx_object->properties = &mysqlx_data_row_properties;

		DBG_RETURN(&mysqlx_object->zo);

	}
	if (mysqlx_object) {
		mnd_pefree(mysqlx_object, persistent);
	}
	delete message;
	DBG_RETURN(nullptr);
}
/* }}} */


/* {{{ mysqlx_register_data_row_class */
void
mysqlx_register_data_row_class(INIT_FUNC_ARGS, zend_object_handlers * mysqlx_std_object_handlers)
{
	mysqlx_object_data_row_handlers = *mysqlx_std_object_handlers;
	mysqlx_object_data_row_handlers.free_obj = mysqlx_data_row_free_storage;

	{
		zend_class_entry tmp_ce;
		INIT_CLASS_ENTRY(tmp_ce, "mysqlx_data_row", mysqlx_data_row_methods);
//		INIT_NS_CLASS_ENTRY(tmp_ce, "mysql_xdevapi", "node_pfc", mysqlx_data_row_methods);
		tmp_ce.create_object = php_mysqlx_data_row_object_allocator;
		mysqlx_data_row_class_entry = zend_register_internal_class(&tmp_ce);
	}

	zend_hash_init(&mysqlx_data_row_properties, 0, nullptr, mysqlx_free_property_cb, 1);
}
/* }}} */


/* {{{ mysqlx_unregister_data_row_class */
void
mysqlx_unregister_data_row_class(SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&mysqlx_data_row_properties);
}
/* }}} */


/* {{{ mysqlx_new_data_row */
void
mysqlx_new_data_row(zval * return_value, const Mysqlx::Resultset::Row & message)
{
	st_mysqlx_data_row* obj;
	DBG_ENTER("mysqlx_new_data_row");
	object_init_ex(return_value, mysqlx_data_row_class_entry);
	MYSQLX_FETCH_MESSAGE__DATA_ROW_FROM_ZVAL(obj, return_value);
	obj->message.CopyFrom(message);
	DBG_VOID_RETURN;
}
/* }}} */

} // namespace msg

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
