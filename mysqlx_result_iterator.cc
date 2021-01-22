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
  | Authors: Andrey Hristov <andrey@php.net>                             |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {
#include <zend_interfaces.h>
}
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_stmt_result.h"
#include "mysqlx_result.h"
#include "mysqlx_object.h"
#include "mysqlx_class_properties.h"
#include "util/object.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

struct st_mysqlx__result_iterator : util::custom_allocable
{
	zend_object_iterator  intern;
	XMYSQLND_STMT_RESULT* result;
	util::zvalue current_row;
	std::size_t row_num;
	bool started;
	bool usable;
};


static void
XMYSQLND_METHOD(mysqlx__result_iterator, dtor)(zend_object_iterator * iter)
{
	DBG_ENTER("mysqlx__result_iterator::dtor");
	st_mysqlx__result_iterator* iterator = reinterpret_cast<st_mysqlx__result_iterator*>(iter);
	if (iterator->result) {
		iterator->result->m.free_reference(iterator->result, nullptr, nullptr);
	}

	/* cleanup handled in sxe_object_dtor as we dont always have an iterator wrapper */
	zval_ptr_dtor(&iterator->intern.data);
	DBG_VOID_RETURN;
}

static int
XMYSQLND_METHOD(mysqlx__result_iterator, valid)(zend_object_iterator * iter)
{
	st_mysqlx__result_iterator* iterator = reinterpret_cast<st_mysqlx__result_iterator*>(iter);
	DBG_ENTER("mysqlx__result_iterator::valid");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	DBG_RETURN(iterator->usable? SUCCESS:FAILURE);
}

#include <ext/standard/php_var.h>

static util::raw_zval*
XMYSQLND_METHOD(mysqlx__result_iterator, current_data)(zend_object_iterator * iter)
{
	st_mysqlx__result_iterator* iterator = (st_mysqlx__result_iterator*) iter;
	DBG_ENTER("mysqlx__result_iterator::current_data");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	DBG_RETURN((iterator->result && iterator->usable)? iterator->current_row.ptr() : nullptr);
}

static enum_func_status
XMYSQLND_METHOD(mysqlx__result_iterator, fetch_current_data)(zend_object_iterator * iter)
{
	st_mysqlx__result_iterator* iterator = (st_mysqlx__result_iterator*) iter;
	DBG_ENTER("mysqlx__result_iterator::fetch_current_data");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	if (iterator->result && iterator->usable) {
		iterator->current_row.reset();

		if (PASS == iterator->result->m.fetch_current(iterator->result, iterator->current_row.ptr(), nullptr, nullptr) &&
			iterator->current_row.is_array())
		{
			DBG_RETURN(PASS);
		} else {
			DBG_RETURN(FAIL);
		}
	}
	DBG_RETURN(FAIL);
}

static void
XMYSQLND_METHOD(mysqlx__result_iterator, next)(zend_object_iterator * iter)
{
	st_mysqlx__result_iterator* iterator = (st_mysqlx__result_iterator*) iter;
	DBG_ENTER("mysqlx__result_iterator::next");
	DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	if (iterator->result && iterator->usable) {
		if (PASS == iterator->result->m.next(iterator->result, nullptr, nullptr) &&
			PASS == XMYSQLND_METHOD(mysqlx__result_iterator, fetch_current_data)(iter))
		{
			iterator->row_num++;
		} else {
			iterator->usable = false;
		}
	}
	DBG_VOID_RETURN;
}

static void
XMYSQLND_METHOD(mysqlx__result_iterator, rewind)(zend_object_iterator * iter)
{
	st_mysqlx__result_iterator* iterator = (st_mysqlx__result_iterator*) iter;
	DBG_ENTER("mysqlx__result_iterator::rewind");
	if (iterator->result && iterator->usable) {
		iterator->started = false;
		iterator->row_num = 0;
		if (PASS == iterator->result->m.rewind(iterator->result) &&
			PASS == XMYSQLND_METHOD(mysqlx__result_iterator, fetch_current_data)(iter))
		{
			iterator->usable = true;
			iterator->started = true;
		} else {
			iterator->usable = false;
		}
//			XMYSQLND_METHOD(mysqlx__result_iterator, next)(iter);
		DBG_INF_FMT("usable=%s  started=%s  row_num=%u", iterator->usable? "TRUE":"FALSE", iterator->started? "TRUE":"FALSE", iterator->row_num);
	}
	DBG_VOID_RETURN;
}

static zend_object_iterator_funcs mysqlx__result_iterator_funcs =
{
	XMYSQLND_METHOD(mysqlx__result_iterator, dtor),
	XMYSQLND_METHOD(mysqlx__result_iterator, valid),
	XMYSQLND_METHOD(mysqlx__result_iterator, current_data),
	nullptr, /* not provided, thus Zend will provide auto_inc keys */
	XMYSQLND_METHOD(mysqlx__result_iterator, next),
	XMYSQLND_METHOD(mysqlx__result_iterator, rewind),
};

static zend_object_iterator*
mysqlx__result_create_iterator(zend_class_entry* ce, util::raw_zval* object, int by_ref)
{
	DBG_ENTER("mysqlx_result_create_iterator");
	auto iterator = util::create_result_iterator<st_mysqlx_result, st_mysqlx__result_iterator>(
		ce,
		&mysqlx__result_iterator_funcs,
		object,
		by_ref);
	DBG_RETURN(iterator);
}

void
mysqlx_register_result_iterator(zend_class_entry * ce)
{
	ce->get_iterator = mysqlx__result_create_iterator;
#if PHP_VERSION_ID < 70300
	ce->iterator_funcs.funcs = &mysqlx__result_iterator_funcs;
#endif

#if PHP_VERSION_ID < 80000
	zend_class_implements(ce, 1, zend_ce_traversable);
#endif
}

} // namespace devapi

} // namespace mysqlx
