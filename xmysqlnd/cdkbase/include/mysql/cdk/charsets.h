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
*/

/*
  Lists of character sets known to CDK string codec (see codec.h).
  This is generally kept in sync with character sets known to the MySQL Server.
  See: http://dev.mysql.com/doc/refman/en/charset-charsets.html

  Note: currently the codec can only convert utf8 strings.
*/


#ifndef MYSQL_CDK_CHARSETS_H
#define MYSQL_CDK_CHARSETS_H

#define CDK_CS_LIST(X) \
  X(big5)  \
  X(dec8)  \
  X(cp850)  \
  X(hp8)  \
  X(koi8r)  \
  X(latin1)  \
  X(latin2)  \
  X(swe7)  \
  X(ascii)  \
  X(ujis)  \
  X(sjis)  \
  X(hebrew)  \
  X(tis620)  \
  X(euckr)  \
  X(koi8u)  \
  X(gb2312)  \
  X(greek)  \
  X(cp1250)  \
  X(gbk)  \
  X(latin5)  \
  X(armscii8)  \
  X(utf8)  \
  X(ucs2)  \
  X(cp866)  \
  X(keybcs2)  \
  X(macce)  \
  X(macroman)  \
  X(cp852)  \
  X(latin7)  \
  X(utf8mb4)  \
  X(cp1251)  \
  X(utf16)  \
  X(utf16le)  \
  X(cp1256)  \
  X(cp1257)  \
  X(utf32)  \
  X(binary)  \
  X(geostd8)  \
  X(cp932)  \
  X(eucjpms)  \
  X(gb18030)  \

#endif
