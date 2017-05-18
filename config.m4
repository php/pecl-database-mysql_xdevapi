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


	xmysqlnd_sources="php_xmysqlnd.cc \
		xmysqlnd/xmysqlnd_any2expr.cc \
		xmysqlnd/xmysqlnd_crud_collection_commands.cc \
		xmysqlnd/xmysqlnd_crud_table_commands.cc \
		xmysqlnd/xmysqlnd_ddl_table_defs.cc \
		xmysqlnd/xmysqlnd_ddl_view_commands.cc \
		xmysqlnd/xmysqlnd_driver.cc \
		xmysqlnd/xmysqlnd_environment.cc \
		xmysqlnd/xmysqlnd_extension_plugin.cc \
		xmysqlnd/xmysqlnd_index_collection_commands.cc \
		xmysqlnd/xmysqlnd_node_collection.cc \
		xmysqlnd/xmysqlnd_node_schema.cc \
		xmysqlnd/xmysqlnd_node_session.cc \
		xmysqlnd/xmysqlnd_node_stmt.cc \
		xmysqlnd/xmysqlnd_node_stmt_result.cc \
		xmysqlnd/xmysqlnd_node_stmt_result_meta.cc \
		xmysqlnd/xmysqlnd_node_table.cc \
		xmysqlnd/xmysqlnd_object_factory.cc \
		xmysqlnd/xmysqlnd_protocol_frame_codec.cc \
		xmysqlnd/xmysqlnd_protocol_dumper.cc \
		xmysqlnd/xmysqlnd_rowset.cc \
		xmysqlnd/xmysqlnd_rowset_buffered.cc \
		xmysqlnd/xmysqlnd_rowset_fwd.cc \
		xmysqlnd/xmysqlnd_session_config.cc \
		xmysqlnd/xmysqlnd_statistics.cc \
		xmysqlnd/xmysqlnd_stmt_execution_state.cc \
		xmysqlnd/xmysqlnd_table_create.cc \
		xmysqlnd/xmysqlnd_utils.cc \
		xmysqlnd/xmysqlnd_view.cc \
		xmysqlnd/xmysqlnd_warning_list.cc \
		xmysqlnd/xmysqlnd_wireprotocol.cc \
		xmysqlnd/xmysqlnd_zval2any.cc \
		"

	mysqlx_messages=""
	if test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES" != "no" || test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES_ENABLED" = "yes"; then
		AC_DEFINE([MYSQL_XDEVAPI_MESSAGE_CLASSES], 1, [Enable message classes])

		mysqlx_messages="messages/mysqlx_node_connection.cc \
			messages/mysqlx_node_pfc.cc \
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
			messages/mysqlx_message__capability.cc \
			messages/mysqlx_message__stmt_execute.cc \
			messages/mysqlx_message__stmt_execute_ok.cc \
			messages/mysqlx_message__data_fetch_done.cc \
			"
	fi

	mysqlx_phputils="phputils/allocator.cc \
		phputils/exceptions.cc \
		phputils/object.cc \
		phputils/string_utils.cc \
		phputils/strings.cc \
		"

	mysqlx_base_sources="php_mysqlx.cc \
		php_mysqlx_ex.cc \
		mysqlx_base_session.cc \
		mysqlx_class_properties.cc \
		mysqlx_crud_operation_bindable.cc \
		mysqlx_crud_operation_limitable.cc \
		mysqlx_crud_operation_skippable.cc \
		mysqlx_crud_operation_sortable.cc \
		mysqlx_database_object.cc \
		mysqlx_driver.cc \
		mysqlx_exception.cc \
		mysqlx_executable.cc \
		mysqlx_execution_status.cc \
		mysqlx_expression.cc \
		mysqlx_field_metadata.cc \
		mysqlx_node_schema.cc \
		mysqlx_node_session.cc \
		mysqlx_node_session_configuration.cc \
		mysqlx_node_collection.cc \
		mysqlx_node_collection__add.cc \
		mysqlx_node_collection__create_index.cc \
		mysqlx_node_collection__drop_index.cc \
		mysqlx_node_collection__find.cc \
		mysqlx_node_collection__modify.cc \
		mysqlx_node_collection__remove.cc \
		mysqlx_node_table.cc \
		mysqlx_node_table__delete.cc \
		mysqlx_node_table__insert.cc \
		mysqlx_node_table__select.cc \
		mysqlx_node_table__update.cc \
		mysqlx_node_sql_statement.cc \
		mysqlx_node_base_result.cc \
		mysqlx_node_base_result_iterator.cc \
		mysqlx_node_doc_result.cc \
		mysqlx_node_doc_result_iterator.cc \
		mysqlx_node_result.cc \
		mysqlx_node_result_iterator.cc \
		mysqlx_node_row_result.cc \
		mysqlx_node_row_result_iterator.cc \
		mysqlx_node_sql_statement_result.cc \
		mysqlx_node_sql_statement_result_iterator.cc \
		mysqlx_node_column_result.cc \
		mysqlx_object.cc \
		mysqlx_schema_object.cc \
		mysqlx_session.cc \
		mysqlx_table_create.cc \
		mysqlx_table_create_column_def_base.cc \
		mysqlx_table_create_column_def.cc \
		mysqlx_table_create_generated_column_def.cc \
		mysqlx_table_create_foreign_key_def.cc \
		mysqlx_view_create.cc \
		mysqlx_view_alter.cc \
		mysqlx_view_drop.cc \
		mysqlx_warning.cc \
		mysqlx_x_session.cc \
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

	this_ext_sources="$xmysqlnd_protobuf_sources $mysqlx_base_sources $xmysqlnd_expr_parser $xmysqlnd_sources $mysqlx_messages $mysqlx_phputils"
	PHP_NEW_EXTENSION(mysql_xdevapi, $this_ext_sources, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1, true)
	PHP_ADD_BUILD_DIR([ext/mysql_xdevapi], 1)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, json)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, mysqlnd)

	dnl TODO: we should search for a proper protoc matchig the one who's heades we use and which we link above
	PROTOC=protoc
	PHP_SUBST(PROTOC)
	PHP_ADD_MAKEFILE_FRAGMENT()

	dnl TODO: we should ONLY do this for OUR files, and this is NOT portable!
	CXXFLAGS="$CXXFLAGS -std=c++11"
fi
