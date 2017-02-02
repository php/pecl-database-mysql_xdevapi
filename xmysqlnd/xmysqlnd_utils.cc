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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
extern "C" {
#include <php.h>
#undef ERROR
#undef inline
#include <ext/json/php_json.h>
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_structs.h"
}
#include "xmysqlnd_utils.h"

namespace mysqlx {

namespace drv {

/* {{{ make_mysqlnd_cstr */
MYSQLND_CSTRING make_mysqlnd_cstr(const char * str) {
	MYSQLND_CSTRING ret =  { str, str != NULL ? strlen(str) : 0 };
	return ret;
}
/* }}} */

/* {{{ compare_mysqlnd_cstr */
int
equal_mysqlnd_cstr(const MYSQLND_CSTRING* lhs, const MYSQLND_CSTRING* rhs)
{
	int result = 0;
	if (lhs->l == rhs->l)
	{
		result = !memcmp(lhs->s, rhs->s, lhs->l);
	}
	return result;
}
/* }}} */


/* {{{ xmysqlnd_utils_decode_doc_row */
void
xmysqlnd_utils_decode_doc_row(zval* src, zval* dest)
{
	HashTable * row_ht = Z_ARRVAL_P(src);
	zval* row_data = zend_hash_str_find(row_ht, "doc", sizeof("doc") - 1);
	if (row_data && Z_TYPE_P(row_data) == IS_STRING) {
		php_json_decode(dest, Z_STRVAL_P(row_data), Z_STRLEN_P(row_data), TRUE, PHP_JSON_PARSER_DEFAULT_DEPTH);
	}
}
/* }}} */


/* {{{ xmysqlnd_utils_decode_doc_rows */
void
xmysqlnd_utils_decode_doc_rows(zval* src, zval* dest)
{
	array_init(dest);
	if (Z_TYPE_P(src) == IS_ARRAY) {
		zval* raw_row;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(src), raw_row) {
			zval row;
			xmysqlnd_utils_decode_doc_row(raw_row, &row);
			add_next_index_zval(dest, &row);
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

} // namespace drv

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
