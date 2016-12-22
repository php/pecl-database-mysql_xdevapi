dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_connection.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_crud.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_datatypes.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_expect.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_expr.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_notice.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_resultset.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_session.proto
dnl protoc --cpp_out xmysqlnd/proto_gen/ --proto_path xmysqlnd/proto_def/ xmysqlnd/proto_def/mysqlx_sql.proto
dnl
dnl g++ -c proto_gen/*.cc -lprotobuf

PHP_ARG_ENABLE(mysql-xdevapi, whether to enable mysql-xdevapi,
	[  --enable-mysql-xdevapi       Enable mysql-xdevapi], no, yes)

PHP_ARG_ENABLE(mysql-xdevapi-experimental-features, whether to disable experimental features in mysql-xdevapi,
	[  --disable-mysql-xdevapi-experimental-features
						Disable support for the experimental features in mysql-xdevapi], yes, no)

PHP_ARG_ENABLE(mysql-xdevapi-message-classes, whether to enable the experimental message classes in mysql-xdevapi,
	[  --enable-mysql-xdevapi-message-classes
						Enable support for the experimental message classes in mysql-xdevapi], yes, no)

dnl If some extension uses mysql-xdevapi it will get compiled in PHP core
if test "$PHP_MYSQL_XDEVAPI" != "no" || test "$PHP_MYSQL_XDEVAPI_ENABLED" = "yes"; then
	PHP_REQUIRE_CXX

	SEARCH_PATH="/usr/local /usr"
	SEARCH_FOR="include/google/protobuf-c/protobuf-c.h"
	AC_MSG_CHECKING([for protobuf-c files in default path])
	for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
			PROTOBUFC_DIR=$i/lib/google/protobuf-c/
			AC_MSG_RESULT(Header found in $i/include/google/protobuf-c/)
		fi
	done

	SEARCH_PATH="/usr/local /usr"
	SEARCH_FOR="include/boost/function.hpp"
	AC_MSG_CHECKING([for boost::function in default path])
	for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
			MYSQL_XDEVAPI_BOOST_FUNCTION=$i/$SEARCH_FOR
			AC_DEFINE([MYSQL_XDEVAPI_BOOST_FUNCTION], $MYSQL_XDEVAPI_BOOST_FUNCTION, [Enable experimental features])
			AC_MSG_RESULT(Header found in $MYSQL_XDEVAPI_BOOST_FUNCTION)
		fi
	done

	if test "$PHP_MYSQL_XDEVAPI_EXPERIMENTAL_FEATURES" != "no"; then
		AC_DEFINE([MYSQL_XDEVAPI_EXPERIMENTAL_FEATURES], 1, [Enable experimental features])
	fi

	xmysqlnd_protobuf_sources="xmysqlnd/proto_gen/mysqlx_connection.pb.cc \
		xmysqlnd/proto_gen/mysqlx_crud.pb.cc \
		xmysqlnd/proto_gen/mysqlx_datatypes.pb.cc \
		xmysqlnd/proto_gen/mysqlx_expect.pb.cc \
		xmysqlnd/proto_gen/mysqlx_expr.pb.cc \
		xmysqlnd/proto_gen/mysqlx_notice.pb.cc \
		xmysqlnd/proto_gen/mysqlx.pb.cc \
		xmysqlnd/proto_gen/mysqlx_resultset.pb.cc \
		xmysqlnd/proto_gen/mysqlx_session.pb.cc \
		xmysqlnd/proto_gen/mysqlx_sql.pb.cc \
		"

	xmysqlnd_expr_parser="xmysqlnd/crud_parsers/expression_parser.cc \
		xmysqlnd/crud_parsers/orderby_parser.cc \
		xmysqlnd/crud_parsers/projection_parser.cc \
		xmysqlnd/crud_parsers/tokenizer.cc \
		"


	xmysqlnd_sources="php_xmysqlnd.c \
		xmysqlnd/xmysqlnd_any2expr.cc \
		xmysqlnd/xmysqlnd_crud_collection_commands.cc \
		xmysqlnd/xmysqlnd_crud_table_commands.cc \
		xmysqlnd/xmysqlnd_driver.c \
		xmysqlnd/xmysqlnd_extension_plugin.c \
		xmysqlnd/xmysqlnd_index_collection_commands.cc \
		xmysqlnd/xmysqlnd_node_collection.c \
		xmysqlnd/xmysqlnd_node_schema.c \
		xmysqlnd/xmysqlnd_node_session.c \
		xmysqlnd/xmysqlnd_node_stmt.c \
		xmysqlnd/xmysqlnd_node_stmt_result.c \
		xmysqlnd/xmysqlnd_node_stmt_result_meta.c \
		xmysqlnd/xmysqlnd_node_table.c \
		xmysqlnd/xmysqlnd_object_factory.c \
		xmysqlnd/xmysqlnd_protocol_frame_codec.c \
		xmysqlnd/xmysqlnd_protocol_dumper.cc \
		xmysqlnd/xmysqlnd_rowset.c \
		xmysqlnd/xmysqlnd_rowset_buffered.c \
		xmysqlnd/xmysqlnd_rowset_fwd.c \
		xmysqlnd/xmysqlnd_statistics.c \
		xmysqlnd/xmysqlnd_stmt_execution_state.c \
		xmysqlnd/xmysqlnd_utils.c \
		xmysqlnd/xmysqlnd_warning_list.c \
		xmysqlnd/xmysqlnd_wireprotocol.cc \
		xmysqlnd/xmysqlnd_zval2any.cc \
		"

	mysqlx_messages=""
	if test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES" != "no" || test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES_ENABLED" = "yes"; then
		AC_DEFINE([MYSQL_XDEVAPI_MESSAGE_CLASSES], 1, [Enable message classes])

		mysqlx_messages="messages/mysqlx_node_connection.c \
			messages/mysqlx_node_pfc.c \
										\
			messages/mysqlx_resultset__column_metadata.cc \
			messages/mysqlx_resultset__resultset_metadata.cc \
			messages/mysqlx_resultset__data_row.cc \
													\
			messages/mysqlx_message__error.cc \
			messages/mysqlx_message__ok.cc \
			messages/mysqlx_message__auth_start.cc \
			messages/mysqlx_message__auth_continue.cc \
			messages/mysqlx_message__auth_ok.cc \
			messages/mysqlx_message__capabilities_get.cc \
			messages/mysqlx_message__capabilities_set.cc \
			messages/mysqlx_message__capabilities.cc \
			messages/mysqlx_message__capability.c \
			messages/mysqlx_message__stmt_execute.cc \
			messages/mysqlx_message__stmt_execute_ok.cc \
			messages/mysqlx_message__data_fetch_done.cc \
			"
	fi

	mysqlx_php="php/allocator.cc \
		"

	mysqlx_base_sources="php_mysqlx.c \
		php_mysqlx_ex.c \
		mysqlx_base_session.c \
		mysqlx_class_properties.c \
		mysqlx_crud_operation_bindable.c \
		mysqlx_crud_operation_limitable.c \
		mysqlx_crud_operation_skippable.c \
		mysqlx_crud_operation_sortable.c \
		mysqlx_database_object.c \
		mysqlx_driver.c \
		mysqlx_exception.c \
		mysqlx_executable.c \
		mysqlx_execution_status.c \
		mysqlx_expression.c \
		mysqlx_field_metadata.c \
		mysqlx_node_schema.c \
		mysqlx_node_session.c \
		mysqlx_node_collection.c \
		mysqlx_node_collection__add.c \
		mysqlx_node_collection__create_index.cc \
		mysqlx_node_collection__drop_index.cc \
		mysqlx_node_collection__find.c \
		mysqlx_node_collection__modify.c \
		mysqlx_node_collection__remove.c \
		mysqlx_node_table.c \
		mysqlx_node_table__delete.c \
		mysqlx_node_table__insert.c \
		mysqlx_node_table__select.c \
		mysqlx_node_table__update.c \
		mysqlx_node_sql_statement.c \
		mysqlx_node_base_result.c \
		mysqlx_node_base_result_iterator.c \
		mysqlx_node_doc_result.c \
		mysqlx_node_doc_result_iterator.c \
		mysqlx_node_result.c \
		mysqlx_node_result_iterator.c \
		mysqlx_node_row_result.c \
		mysqlx_node_row_result_iterator.c \
		mysqlx_node_sql_statement_result.c \
		mysqlx_node_sql_statement_result_iterator.c \
		mysqlx_node_column_result.c \
		mysqlx_x_session.c \
		mysqlx_object.c \
		mysqlx_session.c \
		mysqlx_schema_object.c \
		mysqlx_warning.c \
		"

	AC_DEFINE([MYSQL_XDEVAPI_SSL_SUPPORTED], 1, [Enable core xmysqlnd SSL code])

	test -z "$PHP_OPENSSL" && PHP_OPENSSL=no

	if test "$PHP_OPENSSL" != "no" || test "$PHP_OPENSSL_DIR" != "no"; then
		AC_CHECK_LIB(ssl, DSA_get_default_method, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))
		AC_CHECK_LIB(crypto, X509_free, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))

		PHP_SETUP_OPENSSL(MYSQL_XDEVAPI_SHARED_LIBADD, [AC_DEFINE(MYSQL_XDEVAPI_HAVE_SSL,1,[Enable mysql_xdevapi code that uses OpenSSL directly])])
	fi

	if test "$PHP_MYSQLND" != "yes" && test "$PHP_MYSQLND_ENABLED" != "yes" && test "$PHP_MYSQLI" != "yes" && test "$PHP_MYSQLI" != "mysqlnd"; then
		dnl Enable mysqlnd build in case it wasn't passed explicitly in cmd-line
		PHP_ADD_BUILD_DIR([ext/mysqlnd], 1)

		dnl This needs to be set in any extension which wishes to use mysqlnd
		PHP_MYSQLND_ENABLED=yes

		AC_MSG_NOTICE(mysql-xdevapi depends on ext/mysqlnd; it has been added to build)
	fi

	PHP_ADD_LIBRARY(protobuf,, MYSQL_XDEVAPI_SHARED_LIBADD)

	PHP_SUBST(MYSQL_XDEVAPI_SHARED_LIBADD)

	PHP_ADD_BUILD_DIR($ext_builddir/messages)
	PHP_ADD_BUILD_DIR($ext_builddir/proto_gen)

	this_ext_sources="$xmysqlnd_protobuf_sources $mysqlx_base_sources $xmysqlnd_expr_parser $xmysqlnd_sources $mysqlx_messages $mysqlx_php"
	PHP_NEW_EXTENSION(mysql_xdevapi, $this_ext_sources, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1, true)
	PHP_ADD_BUILD_DIR([ext/mysql_xdevapi], 1)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, json)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, mysqlnd)
	PHP_INSTALL_HEADERS([ext/mysql_xdevapi/])

	dnl TODO: we should search for a proper protoc matchig the one who's heades we use and which we link above
	PROTOC=protoc
	PHP_SUBST(PROTOC)
	PHP_ADD_MAKEFILE_FRAGMENT()

	dnl TODO: we should ONLY do this for OUR files, and this is NOT portable!
	CXXFLAGS="$CXXFLAGS -std=c++11"
fi
