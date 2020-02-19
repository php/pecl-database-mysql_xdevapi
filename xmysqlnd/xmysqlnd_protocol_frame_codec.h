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
#ifndef XMYSQLND_PROTOCOL_FRAME_CODEC_H
#define XMYSQLND_PROTOCOL_FRAME_CODEC_H

#include "xmysqlnd_enum_n_def.h"
#include "xmysqlnd_driver.h"
#include "util/allocator.h"

namespace mysqlx {

namespace drv {

typedef struct st_xmysqlnd_protocol_frame_codec			XMYSQLND_PFC;
typedef struct st_xmysqlnd_protocol_frame_codec_data	XMYSQLND_PFC_DATA;

typedef enum_func_status	(*func_xmysqlnd_pfc__init)(XMYSQLND_PFC * const pfc, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const factory, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_pfc__reset)(XMYSQLND_PFC * const pfc, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_pfc__set_client_option)(XMYSQLND_PFC * const pfc, enum_xmysqlnd_client_option option, const char * const value);
typedef enum_func_status	(*func_xmysqlnd_pfc__send)(XMYSQLND_PFC * const pfc, MYSQLND_VIO * const vio, zend_uchar packet_type, const zend_uchar * const buffer, const size_t count, size_t * bytes_sent, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef enum_func_status	(*func_xmysqlnd_pfc__receive)(XMYSQLND_PFC * const pfc, MYSQLND_VIO * const vio, zend_uchar * prealloc_buffer, const size_t prealloc_buffer_len, zend_uchar * packet_type, zend_uchar ** buffer, size_t * count, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);
typedef void				(*func_xmysqlnd_pfc__free_contents)(XMYSQLND_PFC * pfc);
typedef void				(*func_xmysqlnd_pfc__dtor)(XMYSQLND_PFC * const pfc, MYSQLND_STATS * const stats, MYSQLND_ERROR_INFO * const error_info);


MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec)
{
	func_xmysqlnd_pfc__init init;
	func_xmysqlnd_pfc__reset reset;
	func_xmysqlnd_pfc__set_client_option set_client_option;
	func_xmysqlnd_pfc__send send;
	func_xmysqlnd_pfc__receive receive;

	func_xmysqlnd_pfc__free_contents free_contents;
	func_xmysqlnd_pfc__dtor dtor;
};

struct st_xmysqlnd_protocol_frame_codec_data : public util::custom_allocable
{
	php_stream		*stream;
	size_t			max_packet_size;
	zend_bool		ssl;

	zend_bool		persistent;
	MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_protocol_packet_frame_codec) m;
};


struct st_xmysqlnd_protocol_frame_codec : public util::custom_allocable
{
	MYSQLND_CMD_BUFFER	cmd_buffer;

	st_xmysqlnd_protocol_frame_codec_data* data;

	zend_bool 		persistent;
};


PHP_MYSQL_XDEVAPI_API MYSQLND_CLASS_METHODS_INSTANCE_DECLARE(xmysqlnd_protocol_packet_frame_codec);
PHP_MYSQL_XDEVAPI_API XMYSQLND_PFC * xmysqlnd_pfc_create(const zend_bool persistent, const MYSQLND_CLASS_METHODS_TYPE(xmysqlnd_object_factory) * const object_factory,  MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);
PHP_MYSQL_XDEVAPI_API void xmysqlnd_pfc_free(XMYSQLND_PFC * const net, MYSQLND_STATS * stats, MYSQLND_ERROR_INFO * error_info);

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_PROTOCOL_FRAME_CODEC_H */
