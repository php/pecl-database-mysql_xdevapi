dnl Note: see README for build details

PHP_ARG_WITH(
	boost,
	for boost install dir,
	[  --with-boost[=DIR]          Point out boost library],
	no,
	no)

PHP_ARG_WITH(
	protobuf,
	for protobuf install dir,
	[  --with-protobuf[=DIR]       Point out protobuf library],
	no,
	no)

PHP_ARG_ENABLE(
	dev-mode,
	whether to enable developer mode,
	[  --enable-dev-mode           Enable internal developer mode],
	no,
	no)

PHP_ARG_ENABLE(
	mysql-xdevapi,
	whether to enable mysql-xdevapi,
	[  --enable-mysql-xdevapi      Enable mysql-xdevapi],
	no,
	yes)


dnl If some extension uses mysql-xdevapi it will get compiled in PHP core
if test "$PHP_MYSQL_XDEVAPI" != "no" || test "$PHP_MYSQL_XDEVAPI_ENABLED" = "yes"; then
	PHP_REQUIRE_CXX

	mysqlx_devapi_sources=" \
		mysqlx_base_result.cc \
		mysqlx_class_properties.cc \
		mysqlx_collection.cc \
		mysqlx_collection__add.cc \
		mysqlx_collection__find.cc \
		mysqlx_collection__modify.cc \
		mysqlx_collection__remove.cc \
		mysqlx_collection_index.cc \
		mysqlx_column_result.cc \
		mysqlx_crud_operation_bindable.cc \
		mysqlx_crud_operation_limitable.cc \
		mysqlx_crud_operation_skippable.cc \
		mysqlx_crud_operation_sortable.cc \
		mysqlx_database_object.cc \
		mysqlx_doc_result.cc \
		mysqlx_doc_result_iterator.cc \
		mysqlx_driver.cc \
		mysqlx_exception.cc \
		mysqlx_executable.cc \
		mysqlx_execution_status.cc \
		mysqlx_expression.cc \
		mysqlx_field_metadata.cc \
		mysqlx_object.cc \
		mysqlx_result.cc \
		mysqlx_result_iterator.cc \
		mysqlx_row_result.cc \
		mysqlx_row_result_iterator.cc \
		mysqlx_schema.cc \
		mysqlx_schema_object.cc \
		mysqlx_session.cc \
		mysqlx_sql_statement.cc \
		mysqlx_sql_statement_result.cc \
		mysqlx_sql_statement_result_iterator.cc \
		mysqlx_table.cc \
		mysqlx_table__delete.cc \
		mysqlx_table__insert.cc \
		mysqlx_table__select.cc \
		mysqlx_table__update.cc \
		mysqlx_warning.cc \
		mysqlx_x_session.cc \
		php_mysqlx.cc \
		php_mysqlx_ex.cc \
		"

	mysqlx_messages=" \
		messages/mysqlx_connection.cc \
		messages/mysqlx_message__auth_ok.cc \
		messages/mysqlx_message__auth_start.cc \
		messages/mysqlx_message__capabilities.cc \
		messages/mysqlx_message__capabilities_get.cc \
		messages/mysqlx_message__capabilities_set.cc \
		messages/mysqlx_message__capability.cc \
		messages/mysqlx_message__data_fetch_done.cc \
		messages/mysqlx_message__error.cc \
		messages/mysqlx_message__ok.cc \
		messages/mysqlx_message__stmt_execute.cc \
		messages/mysqlx_message__stmt_execute_ok.cc \
		messages/mysqlx_pfc.cc \
		messages/mysqlx_resultset__column_metadata.cc \
		messages/mysqlx_resultset__data_row.cc \
		messages/mysqlx_resultset__resultset_metadata.cc \
		"

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
		util/zend_utils.cc \
		"

	xmysqlnd_sources=" \
		xmysqlnd/xmysqlnd_any2expr.cc \
		xmysqlnd/xmysqlnd_collection.cc \
		xmysqlnd/xmysqlnd_crud_collection_commands.cc \
		xmysqlnd/xmysqlnd_crud_table_commands.cc \
		xmysqlnd/xmysqlnd_driver.cc \
		xmysqlnd/xmysqlnd_environment.cc \
		xmysqlnd/xmysqlnd_extension_plugin.cc \
		xmysqlnd/xmysqlnd_index_collection_commands.cc \
		xmysqlnd/xmysqlnd_object_factory.cc \
		xmysqlnd/xmysqlnd_protocol_dumper.cc \
		xmysqlnd/xmysqlnd_protocol_frame_codec.cc \
		xmysqlnd/xmysqlnd_rowset.cc \
		xmysqlnd/xmysqlnd_rowset_buffered.cc \
		xmysqlnd/xmysqlnd_rowset_fwd.cc \
		xmysqlnd/xmysqlnd_schema.cc \
		xmysqlnd/xmysqlnd_session.cc \
		xmysqlnd/xmysqlnd_statistics.cc \
		xmysqlnd/xmysqlnd_stmt.cc \
		xmysqlnd/xmysqlnd_stmt_execution_state.cc \
		xmysqlnd/xmysqlnd_stmt_result.cc \
		xmysqlnd/xmysqlnd_stmt_result_meta.cc \
		xmysqlnd/xmysqlnd_table.cc \
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


	if test "$PHP_DEV_MODE" = "yes" || test "$PHP_DEV_MODE_ENABLED" = "yes"; then
		AC_DEFINE([MYSQL_XDEVAPI_DEV_MODE], 1, [Enable developer mode])
		DEV_MODE_CXXFLAGS="-Werror"
	else
		DEV_MODE_CXXFLAGS=""
	fi

	MYSQL_XDEVAPI_CXXFLAGS="-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++14 \
		-Wall -Wno-unused-function -Wformat-security -Wformat-extra-args \
		$DEV_MODE_CXXFLAGS"

	case $host_os in
		*darwin*)
			dnl On macOS there is problem with older protobuf libs which call deprecated
			dnl atomic functions. It generates warnings, but causes compilation errors in
			dnl case of option 'treat warnings as error' enabled (-Werror), e.g.:
			dnl protobuf/src/google/protobuf/stubs/atomicops_internals_macosx.h:47:9: error:
			dnl 'OSAtomicCompareAndSwap32' is deprecated: first deprecated in macOS 10.12 -
			dnl Use std::atomic_compare_exchange_strong_explicit(std::memory_order_relaxed)
			dnl from <atomic> instead [-Werror,-Wdeprecated-declarations]
			dnl https://github.com/google/protobuf/issues/2182
			dnl https://github.com/grpc/grpc/issues/8466
			dnl it is already fixed
			dnl https://github.com/google/protobuf/pull/2337
			dnl but we still use older version on pb2, so have to temporarily suppress
			dnl warnings regarding deprecated functions
			dnl btw I tried to find a way to pass includes with switch "-isystem" instead
			dnl of "-I", but so far didn't find one (PHP_ADD_INCLUDE adds common "-I")
			AC_MSG_NOTICE([-Wno-deprecated-declarations added])
			MYSQL_XDEVAPI_CXXFLAGS="$MYSQL_XDEVAPI_CXXFLAGS -Wno-deprecated-declarations"
			;;
	esac

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
		AC_MSG_NOTICE([phpize/pecl build mode])

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
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, hash)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, json)
	PHP_ADD_EXTENSION_DEP(mysql_xdevapi, mysqlnd)


	dnl boost
	AC_MSG_CHECKING([for boost])
	SEARCH_PATH="$PHP_BOOST $MYSQL_XDEVAPI_BOOST_ROOT $BOOST_ROOT $BOOST_PATH /usr/local/include /usr/include"
	SEARCH_FOR="boost/version.hpp"
	for i in $SEARCH_PATH ; do
		if test -r "$i/$SEARCH_FOR"; then
			BOOST_RESOLVED_ROOT=$i
			break
		fi
	done

	if test -d "$BOOST_RESOLVED_ROOT"; then
		PHP_ADD_INCLUDE([$BOOST_RESOLVED_ROOT])
		AC_MSG_RESULT(found in $BOOST_RESOLVED_ROOT)
	else
		AC_MSG_ERROR([not found, consider use of --with-boost or setting MYSQL_XDEVAPI_BOOST_ROOT])
	fi

	AC_MSG_CHECKING([if boost version is valid])
	MINIMAL_BOOST_VER=105300
	MINIMAL_BOOST_VER_LABEL="1.53.00"
	AC_EGREP_CPP(
		boost_version_ok,
		[
			#include "$BOOST_RESOLVED_ROOT/boost/version.hpp"
			#if BOOST_VERSION >= $MINIMAL_BOOST_VER
				boost_version_ok
			#endif
		],
		[AC_MSG_RESULT([ok])],
		[AC_MSG_ERROR([boost version is too old, required at least $MINIMAL_BOOST_VER_LABEL])]
	)


	dnl protobuf
	AC_MSG_CHECKING([for protobuf])
	SEARCH_PATH="$PHP_PROTOBUF $MYSQL_XDEVAPI_PROTOBUF_ROOT $PROTOBUF_ROOT $PROTOBUF_PATH /usr/local /usr"
	SEARCH_FOR="bin/protoc"
	for i in $SEARCH_PATH ; do
		if test -r "$i/$SEARCH_FOR"; then
			PROTOBUF_RESOLVED_ROOT=$i
			break
		fi
	done

	if test -z "$PROTOBUF_RESOLVED_ROOT"; then
		AC_PATH_PROG(PROTOC_PATH_RESOLVED, protoc)
		if test -x "$PROTOC_PATH_RESOLVED"; then
			PROTOBUF_BIN_DIR=$(dirname "$PROTOC_PATH_RESOLVED")
			PROTOBUF_RESOLVED_ROOT=$(dirname "$PROTOBUF_BIN_DIR")
		fi
	fi

	if test -d "$PROTOBUF_RESOLVED_ROOT"; then
		MYSQL_XDEVAPI_PROTOC=$PROTOBUF_RESOLVED_ROOT/bin/protoc
		PHP_SUBST(MYSQL_XDEVAPI_PROTOC)

		MYSQL_XDEVAPI_PROTOBUF_INCLUDES=$PROTOBUF_RESOLVED_ROOT/include
		PHP_SUBST(MYSQL_XDEVAPI_PROTOBUF_INCLUDES)

		PHP_ADD_INCLUDE([$MYSQL_XDEVAPI_PROTOBUF_INCLUDES])
		PHP_ADD_LIBRARY_WITH_PATH(protobuf, [$PROTOBUF_RESOLVED_ROOT/lib], MYSQL_XDEVAPI_SHARED_LIBADD)

		AC_MSG_RESULT([found in $PROTOBUF_RESOLVED_ROOT])
	else
		AC_MSG_ERROR([not found, consider use of --with-protobuf or setting MYSQL_XDEVAPI_PROTOBUF_ROOT])
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

	dnl expose metadata
	dnl expose sources metadata
	INFO_SRC_PATH=[$ext_builddir/INFO_SRC]
 	AC_PATH_PROG(GIT_PATH, 'git')

	# var xdevapi_version = grep_xdevapi_version(] >> $INFO_BIN_PATH
	MYSQL_XDEVAPI_VERSION=`$EGREP "define PHP_MYSQL_XDEVAPI_VERSION" $ext_srcdir/php_mysql_xdevapi.h | $SED -e 's/[[^0-9\.]]//g'`
	echo [MySQL X DevAPI for PHP ${MYSQL_XDEVAPI_VERSION}] > $INFO_SRC_PATH
	echo [version: ${MYSQL_XDEVAPI_VERSION}] >> $INFO_SRC_PATH

	if test -x "$GIT_PATH"; then
		BRANCH_NAME=`git symbolic-ref --short HEAD`
		echo [branch: $BRANCH_NAME] >> $INFO_SRC_PATH

		COMMIT_INFO=`git log -1 --pretty=format:"commit: %H%ndate: %aD%nshort: %h"`
		echo "${COMMIT_INFO}" >> $INFO_SRC_PATH
	else
		if [ test "$BRANCH_SOURCE" ]; then
			echo [branch: ${BRANCH_SOURCE}] >> $INFO_SRC_PATH
		fi

		if [ test "$PUSH_REVISION" ]; then
			echo [commit: ${PUSH_REVISION}] >> $INFO_SRC_PATH
		fi
	fi

	# expose binaries metadata
	INFO_BIN_PATH=[$ext_builddir/INFO_BIN]

	echo [===== Information about the build process: =====]] > $INFO_BIN_PATH
	HOSTNAME=`hostname -a`
	echo [Build was run at ${CURRENT_TIME} on host ${HOSTNAME}] >> $INFO_BIN_PATH
	HOST_OS=`uname -a`
	echo [Build was done on ${HOST_OS}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	CURRENT_TIME=`date -u`
	echo [build-date: ${CURRENT_TIME}] >> $INFO_BIN_PATH
	echo [os-info: ${HOST_OS}] >> $INFO_BIN_PATH
	if test -z "$PHP_DEBUG"; then
		BUILD_TYPE="Release"
	else
		BUILD_TYPE="Debug"
	fi
	echo [build-type: ${BUILD_TYPE}] >> $INFO_BIN_PATH
	echo [mysql-version: ${MYSQL_XDEVAPI_VERSION}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [host-cpu: ${host_cpu}] >> $INFO_BIN_PATH
	echo [host-vendor: ${host_os}] >> $INFO_BIN_PATH
	echo [host-os: ${host_os}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [===== Compiler / generator used: =====] >> $INFO_BIN_PATH

	COMPILER_VERSION=`${CXX} --version | head -1`
	echo [compiler: "${COMPILER_VERSION}"] >> $INFO_BIN_PATH

	PROTOC_VERSION=`${MYSQL_XDEVAPI_PROTOC} --version`
	echo [protbuf: ${PROTOC_VERSION}] >> $INFO_BIN_PATH

	BOOST_VERSION=`$EGREP "define BOOST_VERSION" $BOOST_RESOLVED_ROOT/boost/version.hpp | $SED -e 's/[[^0-9]]//g'`
	echo [boost: ${BOOST_VERSION}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [===== Feature flags used: =====] >> $INFO_BIN_PATH
	echo [php-version: ${PHP_MAJOR_VERSION}] >> $INFO_BIN_PATH
	echo [architecture: ${AT}] >> $INFO_BIN_PATH
	echo [thread-safety: ${ZEND_ZTS}] >> $INFO_BIN_PATH
	echo [debug: ${PHP_DEBUG}] >> $INFO_BIN_PATH
	echo [developer-mode: ${PHP_DEV_MODE}] >> $INFO_BIN_PATH
	echo [--with-boost: ${PHP_BOOST}] >> $INFO_BIN_PATH
	echo [--with-protobuf: ${PHP_PROTOBUF}] >> $INFO_BIN_PATH

	echo [] >> $INFO_BIN_PATH

	echo [===== Compiler flags used: =====] >> $INFO_BIN_PATH
	echo [CC: ${CC}] >> $INFO_BIN_PATH
	echo [CFLAGS: ${CFLAGS}] >> $INFO_BIN_PATH
	echo [CXX: ${CXX}] >> $INFO_BIN_PATH
	echo [CXXFLAGS: ${CXXFLAGS}] >> $INFO_BIN_PATH
	echo [MYSQL_XDEVAPI_CXXFLAGS: ${MYSQL_XDEVAPI_CXXFLAGS}] >> $INFO_BIN_PATH
	echo [LDFLAGS: ${LDFLAGS}] >> $INFO_BIN_PATH
	echo [PHP_LDFLAGS: ${PHP_LDFLAGS}] >> $INFO_BIN_PATH

	echo [===== EOF =====] >> $INFO_BIN_PATH

fi
