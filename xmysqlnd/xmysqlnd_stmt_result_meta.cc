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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "xmysqlnd.h"
#include "xmysqlnd_priv.h" // XMYSQLND_INC_SESSION_STATISTIC_W_VALUE3
#include "xmysqlnd_stmt.h"
#include "xmysqlnd_wireprotocol.h"
#include "xmysqlnd_driver.h"
#include "xmysqlnd_stmt_result_meta.h"

namespace mysqlx {

namespace drv {

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, init)(XMYSQLND_RESULT_FIELD_META * const field,
												  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory,
												  MYSQLND_STATS * const /*stats*/,
												  MYSQLND_ERROR_INFO * const /*error_info*/)
{
	field->object_factory = factory;
	return PASS;
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_type)(XMYSQLND_RESULT_FIELD_META * const field, enum xmysqlnd_field_type type)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_type");
	field->type = type;
	field->type_set = TRUE;
	DBG_RETURN(PASS);
}

static inline enum_func_status
xmysqlnd_set_mysqlnd_string(util::string* str, const char * const value, const size_t value_len, const zend_bool persistent MYSQLND_MEM_D)
{
#if ZEND_DEBUG
	UNUSED(__zend_lineno);
	UNUSED(__zend_filename);
#endif

	if (value) {
		*str = value ? value : "";
		return !str->empty() ? PASS:FAIL;
	}
	return PASS;
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	zend_ulong idx;

	DBG_ENTER("xmysqlnd_result_field_meta::set_name");
	if (len) {
		field->zend_hash_key.sname = zend_string_init(str, len, field->persistent);
		field->name = util::to_string(field->zend_hash_key.sname);
	} else {
		field->zend_hash_key.sname = ZSTR_EMPTY_ALLOC();
		field->name.clear();
	}

	if (field->zend_hash_key.is_numeric == ZEND_HANDLE_NUMERIC(field->zend_hash_key.sname, idx)) {
		field->zend_hash_key.key = idx;
	}

	DBG_RETURN(!field->name.empty()? PASS:FAIL);
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_name)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_original_name");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->original_name, str, len, field->persistent MYSQLND_MEM_C));
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_table");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->table, str, len, field->persistent MYSQLND_MEM_C));
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_table)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_original_table");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->original_table, str, len, field->persistent MYSQLND_MEM_C));
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_schema)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_schema");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->schema, str, len, field->persistent MYSQLND_MEM_C));
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_catalog)(XMYSQLND_RESULT_FIELD_META * const field, const char * const str, const size_t len)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_catalog");
	DBG_RETURN(xmysqlnd_set_mysqlnd_string(&field->catalog, str, len, field->persistent MYSQLND_MEM_C));
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_collation)(XMYSQLND_RESULT_FIELD_META * const field, const uint64_t collation)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_collation");
	field->collation = collation;
	field->collation_set = TRUE;
	DBG_RETURN(PASS);
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_fractional_digits)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t digits)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_fractional_digits");
	field->fractional_digits = digits;
	field->fractional_digits_set = TRUE;
	DBG_RETURN(PASS);
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_length)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t length)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_length");
	field->length = length;
	field->length_set = TRUE;
	DBG_RETURN(PASS);
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_flags)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t flags)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_flags");
	field->flags = flags;
	field->flags_set = TRUE;
	DBG_RETURN(PASS);
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_content_type)(XMYSQLND_RESULT_FIELD_META * const field, const uint32_t content_type)
{
	DBG_ENTER("xmysqlnd_result_field_meta::set_content_type");
	field->content_type = content_type;
	field->content_type_set = TRUE;
	DBG_RETURN(PASS);
}

static XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_result_field_meta, clone)(const XMYSQLND_RESULT_FIELD_META * const origin, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_RESULT_FIELD_META* cloned{nullptr};
	DBG_ENTER("xmysqlnd_result_field_meta::clone");
	cloned = xmysqlnd_result_field_meta_create(origin->persistent, origin->object_factory, stats, error_info);
	if (cloned) {
		cloned->m->set_type(cloned, origin->type);
		cloned->m->set_name(cloned, origin->name.data(), origin->name.length());
		cloned->m->set_original_name(cloned, origin->original_name.data(), origin->original_name.length());
		cloned->m->set_table(cloned, origin->table.data(), origin->table.length());
		cloned->m->set_original_table(cloned, origin->original_table.data(), origin->original_table.length());
		cloned->m->set_schema(cloned, origin->schema.c_str(), origin->schema.length());
		cloned->m->set_catalog(cloned, origin->catalog.data(), origin->catalog.length());
		cloned->m->set_collation(cloned, origin->collation);
		cloned->m->set_fractional_digits(cloned, origin->fractional_digits);
		cloned->m->set_length(cloned, origin->length);
		cloned->m->set_flags(cloned, origin->flags);
		cloned->m->set_content_type(cloned, origin->content_type);
	}
	DBG_RETURN(cloned);
}

static void
XMYSQLND_METHOD(xmysqlnd_result_field_meta, free_contents)(XMYSQLND_RESULT_FIELD_META * const field)
{
	DBG_ENTER("xmysqlnd_result_field_meta::free_contents");

	field->name.clear();
	field->original_name.clear();
	field->table.clear();
	field->original_table.clear();
	field->schema.clear();
	field->catalog.clear();
	if (field->zend_hash_key.sname) {
		zend_string_release(field->zend_hash_key.sname);
		field->zend_hash_key.sname = nullptr;
	}
	field->zend_hash_key.is_numeric = FALSE;
	field->zend_hash_key.key = 0;

	field->collation_set =
		field->fractional_digits_set =
			field->length_set =
				field->flags_set =
					field->content_type_set = FALSE;
	DBG_VOID_RETURN;
}

static void
XMYSQLND_METHOD(xmysqlnd_result_field_meta, dtor)(
	XMYSQLND_RESULT_FIELD_META* const field,
	MYSQLND_STATS* /*stats*/,
	MYSQLND_ERROR_INFO* /*error_info*/)
{
	DBG_ENTER("xmysqlnd_result_field_meta::dtor");
	if (field) {
		field->m->free_contents(field);
		mnd_efree(field);
	}
	DBG_VOID_RETURN;
}

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_result_field_meta)
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, init),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_type),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_name),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_name),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_table),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_original_table),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_schema),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_catalog),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_collation),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_fractional_digits),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_length),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_flags),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, set_content_type),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, clone),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, free_contents),
	XMYSQLND_METHOD(xmysqlnd_result_field_meta, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_result_field_meta);

PHP_MYSQL_XDEVAPI_API XMYSQLND_RESULT_FIELD_META *
xmysqlnd_result_field_meta_create(const zend_bool persistent,
								  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
								  MYSQLND_STATS * stats,
								  MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_RESULT_FIELD_META* object{nullptr};
	DBG_ENTER("xmysqlnd_result_field_meta_create");
	object = object_factory->get_result_field_meta(object_factory, persistent, stats, error_info);
	DBG_RETURN(object);
}

PHP_MYSQL_XDEVAPI_API void
xmysqlnd_result_field_meta_free(XMYSQLND_RESULT_FIELD_META * const object, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_result_field_meta_free");
	if (object) {
		object->m->dtor(object, stats, error_info);
	}
	DBG_VOID_RETURN;
}

/*******************************************************************************************************************************************/


static enum_func_status
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, init)(
	XMYSQLND_STMT_RESULT_META* const /*meta*/,
	MYSQLND_STATS* const /*stats*/,
	MYSQLND_ERROR_INFO* const /*error_info*/)
{
	return PASS;
}

static enum_func_status
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, add_field)(
	XMYSQLND_STMT_RESULT_META* const meta,
	XMYSQLND_RESULT_FIELD_META* field,
	MYSQLND_STATS* /*stats*/,
	MYSQLND_ERROR_INFO* error_info)
{
	DBG_ENTER("xmysqlnd_stmt_result_meta::add_field");
	if (!meta->fields || meta->field_count == meta->fields_size) {
		meta->fields_size += 8;
		meta->fields = static_cast<XMYSQLND_RESULT_FIELD_META**>(mnd_erealloc(meta->fields, meta->fields_size * sizeof(field)));
		if (!meta->fields) {
			SET_OOM_ERROR(error_info);
			DBG_RETURN(FAIL);
		}
	}
	meta->fields[meta->field_count++] = field;

	DBG_RETURN(PASS);
}

static unsigned int
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, count)(const XMYSQLND_STMT_RESULT_META * const meta)
{
	return (meta->field_count);
}

static const XMYSQLND_RESULT_FIELD_META *
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, get_field)(const XMYSQLND_STMT_RESULT_META * const meta, unsigned int field)
{
	return((meta->field_count > 0 && field < meta->field_count)? meta->fields[field] : nullptr);
}

static void
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, free_contents)(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_stmt_result_meta::free_contents");
	if (meta->fields) {
		for (unsigned int i{0}; i < meta->field_count; ++i) {
			meta->fields[i]->m->dtor(meta->fields[i], stats, error_info);
		}
		mnd_efree(meta->fields);
		meta->fields = nullptr;
	}
	DBG_VOID_RETURN;
}

static void
XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, dtor)(XMYSQLND_STMT_RESULT_META * const meta, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_stmt_result_meta::dtor");
	if (meta) {
		meta->m->free_contents(meta, stats, error_info);
		mnd_efree(meta);
	}
	DBG_VOID_RETURN;
}

static
MYSQLND_CLASS_METHODS_START(xmysqlnd_stmt_result_meta)
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, init),
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, add_field),
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, count),
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, get_field),
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, free_contents),
	XMYSQLND_METHOD(xmysqlnd_stmt_result_meta, dtor),
MYSQLND_CLASS_METHODS_END;


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DEFINE(xmysqlnd_stmt_result_meta);

PHP_MYSQL_XDEVAPI_API XMYSQLND_STMT_RESULT_META *
xmysqlnd_stmt_result_meta_create(const zend_bool persistent,
									  const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,
									  MYSQLND_STATS * stats,
									  MYSQLND_ERROR_INFO * error_info)
{
	XMYSQLND_STMT_RESULT_META* object{nullptr};
	DBG_ENTER("xmysqlnd_stmt_result_meta_create");
	object = object_factory->get_stmt_result_meta(object_factory, persistent, stats, error_info);
	DBG_RETURN(object);
}

PHP_MYSQL_XDEVAPI_API void
xmysqlnd_stmt_result_meta_free(XMYSQLND_STMT_RESULT_META * const object, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info)
{
	DBG_ENTER("xmysqlnd_stmt_result_meta_free");
	if (object) {
		object->m->dtor(object, stats, error_info);
	}
	DBG_VOID_RETURN;
}

} // namespace drv

} // namespace mysqlx
