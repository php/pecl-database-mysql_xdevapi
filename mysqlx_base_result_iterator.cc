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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
extern "C" {
#include <zend_interfaces.h>
#include <ext/mysqlnd/mysqlnd.h>
#include <ext/mysqlnd/mysqlnd_debug.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "mysqlx_base_result.h"
#include "mysqlx_object.h"
#include "mysqlx_class_properties.h"
#include "util/allocator.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

struct st_mysqlx_base_result_iterator : util::custom_allocable
{
	zend_object_iterator intern;
	XMYSQLND_STMT_RESULT * result;
	zval current_row;
	size_t row_num;
	zend_bool started;
	zend_bool usable;
};


/* {{{ mysqlx_base_result_iterator::dtor */
static void
XMYSQLND_METHOD(mysqlx_base_result_iterator, dtor)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::dtor");
	if (iterator->result) {
		iterator->result->m.free_reference(iterator->result, nullptr, nullptr);
	}

	/* cleanup handled in sxe_object_dtor as we dont always have an iterator wrapper */
	zval_ptr_dtor(&iterator->intern.data);
	zval_ptr_dtor(&iterator->current_row);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_result_iterator::valid */
static int
XMYSQLND_METHOD(mysqlx_base_result_iterator, valid)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::valid");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	DBG_RETURN(iterator->usable? SUCCESS:FAILURE);
}
/* }}} */


#include <ext/standard/php_var.h>

/* {{{ mysqlx_base_result_iterator::current_data */
static zval *
XMYSQLND_METHOD(mysqlx_base_result_iterator, current_data)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::current_data");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	DBG_RETURN((iterator->result && iterator->usable)? &iterator->current_row : nullptr);
}
/* }}} */


/* {{{ mysqlx_base_result_iterator::fetch_current_data */
static enum_func_status
XMYSQLND_METHOD(mysqlx_base_result_iterator, fetch_current_data)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::fetch_current_data");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	if (iterator->result && iterator->usable) {
		zval_ptr_dtor(&iterator->current_row);
		ZVAL_UNDEF(&iterator->current_row);

		if (PASS == iterator->result->m.fetch_current(iterator->result, &iterator->current_row, nullptr, nullptr) &&
			IS_ARRAY == Z_TYPE(iterator->current_row))
		{
			DBG_RETURN(PASS);
		} else {
			DBG_RETURN(FAIL);
		}
	}
	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ mysqlx_base_result_iterator::next */
static void
XMYSQLND_METHOD(mysqlx_base_result_iterator, next)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::next");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	if (iterator->result && iterator->usable) {
		if (PASS == iterator->result->m.next(iterator->result, nullptr, nullptr) &&
			PASS == XMYSQLND_METHOD(mysqlx_base_result_iterator, fetch_current_data)(iter))
		{
			iterator->row_num++;
		} else {
			iterator->usable = FALSE;
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_result_iterator::rewind */
static void
XMYSQLND_METHOD(mysqlx_base_result_iterator, rewind)(zend_object_iterator * iter)
{
	st_mysqlx_base_result_iterator* iterator = (st_mysqlx_base_result_iterator*) iter;
	DBG_ENTER("mysqlx_base_result_iterator::rewind");
	if (iterator->result && iterator->usable) {
		iterator->started = FALSE;
		iterator->row_num = 0;
		if (PASS == iterator->result->m.rewind(iterator->result) &&
			PASS == XMYSQLND_METHOD(mysqlx_base_result_iterator, fetch_current_data)(iter))
		{
			iterator->usable = TRUE;
			iterator->started = TRUE;
		} else {
			iterator->usable = FALSE;
		}
//			XMYSQLND_METHOD(mysqlx_base_result_iterator, next)(iter);
		DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlx_base_result_iterator_funcs */
static zend_object_iterator_funcs mysqlx_base_result_iterator_funcs =
{
	XMYSQLND_METHOD(mysqlx_base_result_iterator, dtor),
	XMYSQLND_METHOD(mysqlx_base_result_iterator, valid),
	XMYSQLND_METHOD(mysqlx_base_result_iterator, current_data),
	nullptr, /* not provided, thus Zend will provide auto_inc keys */
	XMYSQLND_METHOD(mysqlx_base_result_iterator, next),
	XMYSQLND_METHOD(mysqlx_base_result_iterator, rewind),
};
/* }}} */


/* {{{ mysqlx_base_result_create_iterator */
static zend_object_iterator *
mysqlx_base_result_create_iterator(zend_class_entry * ce, zval * object, int by_ref)
{
	DBG_ENTER("mysqlx_base_result_create_iterator");
	auto iterator = util::create_result_iterator<st_mysqlx_base_result, st_mysqlx_base_result_iterator>(
		ce,
		&mysqlx_base_result_iterator_funcs,
		object,
		by_ref);
	DBG_RETURN(iterator);
}
/* }}} */


/* {{{ mysqlx_register_base_result_iterator */
void
mysqlx_register_base_result_iterator(zend_class_entry * ce)
{
	ce->get_iterator = mysqlx_base_result_create_iterator;
#if PHP_VERSION_ID < 70300
	ce->iterator_funcs.funcs = &mysqlx_base_result_iterator_funcs;
#endif

	zend_class_implements(ce, 1, zend_ce_traversable);
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
