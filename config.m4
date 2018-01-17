dnl  Note: The extension name is "mysql-xdevapi", you enable it with
dnl  "--enable-mysql-xdevapi", for the moment only phpize/pecl build mode
dnl  is officially supported
dnl
dnl  required 3rdParty libs may be also configured with below environment variables:
dnl  - MYSQL_XDEVAPI_PROTOBUF_ROOT to point out google protobuf root
dnl  - MYSQL_XDEVAPI_BOOST_ROOT to point out boost libraries


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

	if test "$PHP_MYSQL_XDEVAPI_EXPERIMENTAL_FEATURES" != "no"; then
		AC_DEFINE([MYSQL_XDEVAPI_EXPERIMENTAL_FEATURES], 1, [Enable experimental features])
	fi

	mysqlx_devapi_sources=" \
		php_mysqlx.cc \
		php_mysqlx_ex.cc \
		php_xmysqlnd.cc \
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
		mysqlx_node_collection.cc \
		mysqlx_node_collection__add.cc \
		mysqlx_node_collection__find.cc \
		mysqlx_node_collection__modify.cc \
		mysqlx_node_collection__remove.cc \
		mysqlx_node_collection_index.cc \
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
		mysqlx_warning.cc \
		mysqlx_x_session.cc \
		"

	mysqlx_messages=""
	if test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES" != "no" || test "$PHP_MYSQL_XDEVAPI_MESSAGE_CLASSES_ENABLED" = "yes"; then
		AC_DEFINE([MYSQL_XDEVAPI_MESSAGE_CLASSES], 1, [Enable message classes])

		mysqlx_messages=" \
			messages/mysqlx_node_connection.cc \
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

	mysqlx_util=" \
		util/allocator.cc \
		util/exceptions.cc \
		util/hash_table.cc \
		util/json_utils.cc \
		util/object.cc \
		util/pb_utils.cc \
		util/string_utils.cc \
		util/strings.cc \
		util/url_utils.cc \
		util/value.cc \
		"

	xmysqlnd_sources=" \
		xmysqlnd/xmysqlnd_any2expr.cc \
		xmysqlnd/xmysqlnd_crud_collection_commands.cc \
		xmysqlnd/xmysqlnd_crud_table_commands.cc \
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
		xmysqlnd/xmysqlnd_statistics.cc \
		xmysqlnd/xmysqlnd_stmt_execution_state.cc \
		xmysqlnd/xmysqlnd_utils.cc \
		xmysqlnd/xmysqlnd_warning_list.cc \
		xmysqlnd/xmysqlnd_wireprotocol.cc \
		xmysqlnd/xmysqlnd_zval2any.cc \
		"

	xmysqlnd_cdkbase_parser=" \
		xmysqlnd/cdkbase/core/codec.cc \
		xmysqlnd/cdkbase/foundation/error.cc \
		xmysqlnd/cdkbase/foundation/string.cc \
		xmysqlnd/cdkbase/parser/expr_parser.cc \
		xmysqlnd/cdkbase/parser/json_parser.cc \
		xmysqlnd/cdkbase/parser/tokenizer.cc \
		"

	xmysqlnd_crud_parsers=" \
		xmysqlnd/crud_parsers/expression_parser.cc \
		xmysqlnd/crud_parsers/legacy_tokenizer.cc \
		xmysqlnd/crud_parsers/mysqlx_crud_parser.cc \
		"

	xmysqlnd_protobuf_sources=" \
		xmysqlnd/proto_gen/mysqlx_connection.pb.cc \
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

	MYSQL_XDEVAPI_SOURCES=" \
		$xmysqlnd_protobuf_sources \
		$mysqlx_devapi_sources \
		$mysqlx_messages \
		$mysqlx_util \
		$xmysqlnd_sources \
		$xmysqlnd_cdkbase_parser \
		$xmysqlnd_crud_parsers \
		"

	MYSQL_XDEVAPI_CXXFLAGS="-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++14"

	dnl CAUTION! PHP_NEW_EXTENSION defines variables like $ext_srcdir, $ext_builddir or
	dnl $PHP_PECL_EXTENSION. Should be called before they are used.
	PHP_NEW_EXTENSION(mysql_xdevapi, $MYSQL_XDEVAPI_SOURCES, $ext_shared,, $MYSQL_XDEVAPI_CXXFLAGS, true)
	PHP_SUBST(MYSQL_XDEVAPI_SHARED_LIBADD)

	PHP_ADD_INCLUDE([$ext_srcdir/xmysqlnd/cdkbase])
	PHP_ADD_INCLUDE([$ext_srcdir/xmysqlnd/cdkbase/include])

	PHP_ADD_BUILD_DIR([$ext_srcdir])
	PHP_ADD_BUILD_DIR([$ext_srcdir/messages])
	PHP_ADD_BUILD_DIR([$ext_srcdir/phputils])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/crud_parsers])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/proto_gen])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/core])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/foundation])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/parser])

	dnl phpize/pecl build
	if test "$PHP_PECL_EXTENSION"; then
		AC_MSG_NOTICE(phpize/pecl build mode)

		case $host_os in
			*solaris*)
				dnl On Solaris there is problem with C++ exceptions while
				dnl building with gcc/g++ due to conflict between Solaris libc
				dnl vs GNU libgcc_s. They both contain _Unwind_RaiseException
				dnl function, so we have to ensure libgcc_s comes in front of
				dnl libc at link stage, else crashes may occur
				dnl It may be problematic as autoconf may eat duplicates, e.g.
				dnl -lgcc_s -lc -lgcc_s  becomes =>  -lc -lgcc_s
				dnl see also libtool --preserve-dup-deps
				if test -n "$GCC"; then
					AC_MSG_NOTICE(patch applied for libc vs libgcc_s _Unwind_RaiseException conflict)
					LDFLAGS="$LDFLAGS -lgcc_s"
				fi
				;;
		esac
	fi


	dnl dependencies
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, json)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, mysqlnd)


	dnl boost
	AC_MSG_CHECKING([for boost])
	SEARCH_PATH="$MYSQL_XDEVAPI_BOOST_ROOT $BOOST_ROOT $BOOST_PATH /usr/local/include /usr/include"
	SEARCH_FOR="boost/version.hpp"
	for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
			BOOST_ROOT_FOUND=$i
			break
		fi
	done

	if test $BOOST_ROOT_FOUND; then
		PHP_ADD_INCLUDE([$BOOST_ROOT_FOUND])
		AC_MSG_RESULT(found in $BOOST_ROOT_FOUND)
	else
		AC_MSG_RESULT([not found, defaults applied (consider setting MYSQL_XDEVAPI_BOOST_ROOT)])
	fi


	dnl protobuf
	AC_MSG_CHECKING([for protobuf])
	SEARCH_PATH="$MYSQL_XDEVAPI_PROTOBUF_ROOT $PROTOBUF_ROOT $PROTOBUF_PATH /usr/local /usr"
	SEARCH_FOR="bin/protoc"
	for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
			PROTOBUF_ROOT_FOUND=$i
			break
		fi
	done

	if test $PROTOBUF_ROOT_FOUND; then
		MYSQL_XDEVAPI_PROTOC=$PROTOBUF_ROOT_FOUND/bin/protoc
		PHP_SUBST(MYSQL_XDEVAPI_PROTOC)

		MYSQL_XDEVAPI_PROTOBUF_INCLUDES=$PROTOBUF_ROOT_FOUND/include
		PHP_SUBST(MYSQL_XDEVAPI_PROTOBUF_INCLUDES)

		PHP_ADD_INCLUDE([$MYSQL_XDEVAPI_PROTOBUF_INCLUDES])
		PHP_ADD_LIBRARY_WITH_PATH(protobuf, [$PROTOBUF_ROOT_FOUND/lib], MYSQL_XDEVAPI_SHARED_LIBADD)

		AC_MSG_RESULT([found in $PROTOBUF_ROOT_FOUND])
	else
		MYSQL_XDEVAPI_PROTOC=protoc
		PHP_SUBST(MYSQL_XDEVAPI_PROTOC)

		PHP_ADD_LIBRARY(protobuf,, MYSQL_XDEVAPI_SHARED_LIBADD)

		AC_MSG_RESULT([not found, defaults applied (consider setting MYSQL_XDEVAPI_PROTOBUF_ROOT)])
	fi

	PHP_ADD_MAKEFILE_FRAGMENT()

	dnl Enable mysqlnd build in case it wasn't passed explicitly in cmd-line
	if test -z "$PHP_PECL_EXTENSION"; then
		dnl only in case it is NOT phpize/pecl building mode
		if test "$PHP_MYSQLND" != "yes" && test "$PHP_MYSQLND_ENABLED" != "yes" && test "$PHP_MYSQLI" != "yes" && test "$PHP_MYSQLI" != "mysqlnd"; then
			PHP_ADD_BUILD_DIR(ext/mysqlnd, 1)

			dnl This needs to be set in any extension which wishes to use mysqlnd
			PHP_MYSQLND_ENABLED=yes

			AC_MSG_NOTICE([mysql-xdevapi depends on ext/mysqlnd, it has been added to build])
		fi
	fi

	AC_DEFINE(HAVE_MYSQL_XDEVAPI, 1, [mysql-xdevapi support enabled])
fi
