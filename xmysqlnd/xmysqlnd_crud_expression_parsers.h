/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2016 The PHP Group                                |
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
#ifndef XMYSQLND_CRUD_EXPRESSION_PARSERS_H
#define XMYSQLND_CRUD_EXPRESSION_PARSERS_H

#include "xmysqlnd/proto_gen/mysqlx_datatypes.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_expr.pb.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

typedef struct st_xmysqlnd_crud_collection_filter
{
	void * expr;
	zend_bool valid;
} XMYSQLND_CRUD_COLLECTION_FILTER;


typedef struct st_xmysqlnd_crud_document_path
{
	void * expr;
	zend_bool valid;
} XMYSQLND_CRUD_DOCUMENT_PATH;


typedef struct st_xmysqlnd_crud_column_identifier
{
	void * expr;
	zend_bool valid;
} XMYSQLND_CRUD_COLUMN_IDENTIFIER;


typedef struct st_xmysqlnd_crud_table_filter
{
	void * expr;
	zend_bool valid;
} XMYSQLND_CRUD_TABLE_FILTER;


PHPAPI XMYSQLND_CRUD_COLLECTION_FILTER xmysqlnd_crud_parse_collection_filter(const std::string &source, std::vector<std::string>* placeholders);

PHPAPI XMYSQLND_CRUD_DOCUMENT_PATH xmysqlnd_crud_parse_document_path(const std::string& source, Mysqlx::Expr::ColumnIdentifier& col_identifier);

PHPAPI XMYSQLND_CRUD_COLUMN_IDENTIFIER xmysqlnd_crud_parse_column_identifier(const std::string& source);

PHPAPI XMYSQLND_CRUD_TABLE_FILTER xmysqlnd_crud_parse_table_filter(const std::string &source, std::vector<std::string>* placeholders);

#endif /* XMYSQLND_CRUD_EXPRESSION_PARSERS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
