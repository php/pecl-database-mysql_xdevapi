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
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
extern "C" {
#include <ext/json/php_json.h>
}
#include "xmysqlnd_utils.h"
#include <algorithm>

namespace mysqlx {

namespace drv {

void
xmysqlnd_utils_decode_doc_row(zval* src, zval* dest)
{
	HashTable * row_ht = Z_ARRVAL_P(src);
	zval* row_data = zend_hash_str_find(row_ht, "doc", sizeof("doc") - 1);
	if (row_data && Z_TYPE_P(row_data) == IS_STRING) {
		php_json_decode(
			dest,
			Z_STRVAL_P(row_data),
			static_cast<int>(Z_STRLEN_P(row_data)),
			TRUE,
			PHP_JSON_PARSER_DEFAULT_DEPTH);
	}
}

void
xmysqlnd_utils_decode_doc_rows(zval* src, zval* dest)
{
	array_init(dest);
	if (Z_TYPE_P(src) == IS_ARRAY) {
		zval* raw_row{nullptr};
		MYSQLX_HASH_FOREACH_VAL(Z_ARRVAL_P(src), raw_row) {
			zval row;
			xmysqlnd_utils_decode_doc_row(raw_row, &row);
			if (add_next_index_zval(dest, &row) == FAILURE) {
				throw std::runtime_error("decode doc failure - cannot add element to result array");
			}
		} ZEND_HASH_FOREACH_END();
	}
}

/* The ascii-to-ebcdic table: */
/*
 * If system charset is not ascii then the URI
 * parser might not be able to decode properly the
 * address, in that case this conversion table
 * is needed
 */
#ifdef CHARSET_EBCDIC
const unsigned char os_toebcdic[256] = {
/*00*/  0x00, 0x01, 0x02, 0x03, 0x37, 0x2d, 0x2e, 0x2f,
		0x16, 0x05, 0x15, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,  /*................*/
/*10*/  0x10, 0x11, 0x12, 0x13, 0x3c, 0x3d, 0x32, 0x26,
		0x18, 0x19, 0x3f, 0x27, 0x1c, 0x1d, 0x1e, 0x1f,  /*................*/
/*20*/  0x40, 0x5a, 0x7f, 0x7b, 0x5b, 0x6c, 0x50, 0x7d,
		0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,  /* !"#$%&'()*+,-./ */
/*30*/  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
		0xf8, 0xf9, 0x7a, 0x5e, 0x4c, 0x7e, 0x6e, 0x6f,  /*0123456789:;<=>?*/
/*40*/  0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,  /*@ABCDEFGHIJKLMNO*/
/*50*/  0xd7, 0xd8, 0xd9, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6,
		0xe7, 0xe8, 0xe9, 0xbb, 0xbc, 0xbd, 0x6a, 0x6d,  /*PQRSTUVWXYZ[\]^_*/
/*60*/  0x4a, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,  /*`abcdefghijklmno*/
/*70*/  0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
		0xa7, 0xa8, 0xa9, 0xfb, 0x4f, 0xfd, 0xff, 0x07,  /*pqrstuvwxyz{|}~.*/
/*80*/  0x20, 0x21, 0x22, 0x23, 0x24, 0x04, 0x06, 0x08,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x09, 0x0a, 0x14,  /*................*/
/*90*/  0x30, 0x31, 0x25, 0x33, 0x34, 0x35, 0x36, 0x17,
		0x38, 0x39, 0x3a, 0x3b, 0x1a, 0x1b, 0x3e, 0x5f,  /*................*/
/*a0*/  0x41, 0xaa, 0xb0, 0xb1, 0x9f, 0xb2, 0xd0, 0xb5,
		0x79, 0xb4, 0x9a, 0x8a, 0xba, 0xca, 0xaf, 0xa1,  /*................*/
/*b0*/  0x90, 0x8f, 0xea, 0xfa, 0xbe, 0xa0, 0xb6, 0xb3,
		0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,  /*................*/
/*c0*/  0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68,
		0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77,  /*................*/
/*d0*/  0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf,
		0x80, 0xe0, 0xfe, 0xdd, 0xfc, 0xad, 0xae, 0x59,  /*................*/
/*e0*/  0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9c, 0x48,
		0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,  /*................*/
/*f0*/  0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1,
		0x70, 0xc0, 0xde, 0xdb, 0xdc, 0x8d, 0x8e, 0xdf   /*................*/
};
#endif // CHARSET_EBCDIC

static char pct_to_char( const util::string& str,
		  const std::size_t idx )
{
	char value;
	char c;

	c = str[ idx ];
	if (isupper(c))
			c = static_cast<char>(tolower(c));
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = str[ idx + 1 ];
	if (isupper(c))
			c = static_cast<char>(tolower(c));
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return value;
}

util::string
decode_pct_path(const util::string& encoded_path)
{
	const std::size_t encoded_size = encoded_path.size();
	util::string decoded_path;

	for( std::size_t i{ 0 } ; i < encoded_size ; ++i) {
	if ( encoded_path[ i ] == '%' &&
		 i + 2 < encoded_size &&
		 isxdigit( encoded_path[ i + 1 ] ) &&
		 isxdigit( encoded_path[ i + 2 ] ) ) {
#ifndef CHARSET_EBCDIC
			decoded_path.push_back( pct_to_char( encoded_path , i + 1 ) );
#else
			decoded_path.push_back( os_toebcdic[ pct_to_char( encoded_path, i + 1 ) ] );
#endif
		i += 2;
	} else {
		decoded_path.push_back( encoded_path[ i ] );
	}
	}
	return decoded_path;
}

bool operator==(const google::protobuf::Message& msg_a,
				const google::protobuf::Message& msg_b) {
  return (msg_a.GetTypeName() == msg_b.GetTypeName()) &&
	  (msg_a.SerializeAsString() == msg_b.SerializeAsString());
}

} // namespace drv

} // namespace mysqlx
