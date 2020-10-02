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

PHP_ARG_WITH(
	lz4,
	for lz4 install dir,
	[  --with-lz4[=DIR]       Point out lz4 library],
	no,
	no)

PHP_ARG_WITH(
	zlib,
	for zlib install dir,
	[  --with-zlib[=DIR]       Point out zlib library],
	no,
	no)

PHP_ARG_WITH(
	zstd,
	for zstd install dir,
	[  --with-zstd[=DIR]       Point out zstd library],
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


AC_DEFUN([MYSQL_XDEVAPI_ADD_CXXFLAGS], [
	NEW_CXXFLAG=$1
	MYSQL_XDEVAPI_CXXFLAGS="$MYSQL_XDEVAPI_CXXFLAGS $NEW_CXXFLAG"
])

AC_DEFUN([MYSQL_XDEVAPI_DEFINE], [
	CPP_DEFINE=$1
	MYSQL_XDEVAPI_ADD_CXXFLAGS([-D$CPP_DEFINE])
])

AC_DEFUN([MYSQL_XDEVAPI_NOTICE], [
	MESSAGE=$1
	if test "$PHP_DEV_MODE" == "yes" || test "$PHP_DEV_MODE_ENABLED" == "yes"; then
		AC_MSG_ERROR([$MESSAGE])
	else
		AC_MSG_WARN([$MESSAGE])
	fi
])

dnl $1 dirs to be searched (e.g. $PROTOBUF_SEARCH_DIRS)
dnl $2 looked for path (e.g. "google/protobuf/any.h")
dnl $3 subdirs to be also searched (e.g. "include")
dnl $4 variable to store the result - resolved path (e.g. MYSQL_XDEVAPI_PROTOBUF_INCLUDES)
AC_DEFUN([MYSQL_XDEVAPI_RESOLVE_PATH], [
	SEARCH_DIRS=$1
	SEARCH_FOR=$2
	SUBDIRS=$3

	RESOLVED_PATH=""
	for dir in $SEARCH_DIRS; do
		if test -r "$dir/$SEARCH_FOR"; then
			RESOLVED_PATH=$dir
			break
		fi

		for subdir in $SUBDIRS; do
			if test -r "$dir/$subdir/$SEARCH_FOR"; then
				RESOLVED_PATH=$dir/$subdir
				break
			fi
		done
	done

	$4=$RESOLVED_PATH
])

AC_DEFUN([MYSQL_XDEVAPI_PREPARE_BINARY_SEARCH_PATH], [
	SEARCH_DIRS=$1
	BINARY_SEARCH_PATH=""
	for i in $SEARCH_DIRS; do
		if test -d $i; then
			BINARY_SEARCH_PATH="$BINARY_SEARCH_PATH:$i"
		fi
		if test -d $i/bin; then
			BINARY_SEARCH_PATH="$BINARY_SEARCH_PATH:$i/bin"
		fi
	done
	$2=[$BINARY_SEARCH_PATH:$PATH]
])


AC_DEFUN([MYSQL_XDEVAPI_APPEND_INCLUDE_SEARCH_DIR], [
	NEW_INCLUDE_SEARCH_DIR=$1
	if test -d ${NEW_INCLUDE_SEARCH_DIR}; then
		dnl needed also for AC_CHECK_HEADER and AC_EGREP_CPP
		CPPFLAGS="${CPPFLAGS} -I${NEW_INCLUDE_SEARCH_DIR}"
	fi
])

AC_DEFUN([MYSQL_XDEVAPI_ADD_DIR_TO_INCLUDE_SEARCH_PATH], [
	NEW_INCLUDE_SEARCH_DIR=$1
	if test ${NEW_INCLUDE_SEARCH_DIR}; then
		MYSQL_XDEVAPI_APPEND_INCLUDE_SEARCH_DIR("$NEW_INCLUDE_SEARCH_DIR")
		MYSQL_XDEVAPI_APPEND_INCLUDE_SEARCH_DIR("$NEW_INCLUDE_SEARCH_DIR/include")
	fi
])

AC_DEFUN([MYSQL_XDEVAPI_ADD_DIRS_TO_INCLUDE_SEARCH_PATH], [
	NEW_INCLUDE_SEARCH_DIRS=$1
	for i in $NEW_INCLUDE_SEARCH_DIRS; do
		MYSQL_XDEVAPI_ADD_DIR_TO_INCLUDE_SEARCH_PATH($i)
	done
])


AC_DEFUN([MYSQL_XDEVAPI_APPEND_LIB_SEARCH_DIR], [
	NEW_LIB_SEARCH_DIR=$1
	if test -d ${NEW_LIB_SEARCH_DIR}; then
		dnl needed also for AC_CHECK_LIB
		LDFLAGS="${LDFLAGS} -L${NEW_LIB_SEARCH_DIR}"
	fi
])

AC_DEFUN([MYSQL_XDEVAPI_ADD_DIR_TO_LIB_SEARCH_PATH], [
	NEW_LIB_SEARCH_DIR=$1
	if test ${NEW_LIB_SEARCH_DIR}; then
		MYSQL_XDEVAPI_APPEND_LIB_SEARCH_DIR("$NEW_LIB_SEARCH_DIR")
		MYSQL_XDEVAPI_APPEND_LIB_SEARCH_DIR("$NEW_LIB_SEARCH_DIR/lib")
	fi
])

AC_DEFUN([MYSQL_XDEVAPI_ADD_DIRS_TO_LIB_SEARCH_PATH], [
	NEW_LIB_SEARCH_DIRS=$1
	for i in $NEW_LIB_SEARCH_DIRS; do
		MYSQL_XDEVAPI_ADD_DIR_TO_LIB_SEARCH_PATH($i)
	done
])


dnl $1 default compressor root (e.g. $PHP_ZLIB)
dnl $2 lib label (e.g. "zlib")
dnl $3 lib name (e.g. "z")
dnl $4 function name (e.g. "LZ4F_createCompressionContext")
dnl $5 header name (e.g. "zlib.h")
dnl $6 cpp #define (e.g. "MYSQL_XDEVAPI_HAVE_ZLIB")
dnl $7 variable to store the result - whether compressor was resolved (e.g. IS_ZLIB_ENABLED)
AC_DEFUN([MYSQL_XDEVAPI_RESOLVE_COMPRESSOR], [
	DEFAULT_ROOT=$1
	LIB_LABEL=$2
	LIB_NAME=$3
	FUNC_NAME=$4
	HEADER_NAME=$5
	CPP_DEFINE=$6
	$7="no"
	if test "$DEFAULT_ROOT" != "no"; then
		if test "$DEFAULT_ROOT" != "yes"; then
			MYSQL_XDEVAPI_ADD_DIR_TO_INCLUDE_SEARCH_PATH("$DEFAULT_ROOT")
			MYSQL_XDEVAPI_ADD_DIR_TO_LIB_SEARCH_PATH("$DEFAULT_ROOT")
		fi

		AC_CHECK_HEADER([$HEADER_NAME], [], [COMPRESSOR_INCLUDES_NOT_FOUND=1])
		AC_CHECK_LIB([$LIB_NAME], [$FUNC_NAME], [], [COMPRESSOR_LIB_NOT_FOUND=1])
		if test -z $COMPRESSOR_INCLUDES_NOT_FOUND && test -z $COMPRESSOR_LIB_NOT_FOUND; then
			PHP_ADD_LIBRARY_WITH_PATH($LIB_NAME, [], MYSQL_XDEVAPI_SHARED_LIBADD)
			MYSQL_XDEVAPI_DEFINE($CPP_DEFINE)
			$7="yes"
		else
			MYSQL_XDEVAPI_NOTICE("could not resolve compressor $LIB_LABEL")
		fi
 	fi
])


dnl If some extension uses mysql-xdevapi it will get compiled in PHP core
if test "$PHP_MYSQL_XDEVAPI" != "no" || test "$PHP_MYSQL_XDEVAPI_ENABLED" == "yes"; then
	mysqlx_devapi_sources=" \
		mysqlx_base_result.cc \
		mysqlx_class_properties.cc \
		mysqlx_client.cc \
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
		mysqlx_exception.cc \
		mysqlx_executable.cc \
		mysqlx_execution_status.cc \
		mysqlx_expression.cc \
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
		php_mysqlx.cc \
		php_mysqlx_ex.cc \
		"

	mysqlx_util=" \
		util/allocator.cc \
		util/arguments.cc \
		util/exceptions.cc \
		util/functions.cc \
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
		xmysqlnd/xmysqlnd_compression.cc \
		xmysqlnd/xmysqlnd_compression_setup.cc \
		xmysqlnd/xmysqlnd_compression_types.cc \
		xmysqlnd/xmysqlnd_compressor.cc \
		xmysqlnd/xmysqlnd_compressor_lz4.cc \
		xmysqlnd/xmysqlnd_compressor_zlib.cc \
		xmysqlnd/xmysqlnd_compressor_zstd.cc \
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
		xmysqlnd/xmysqlnd_wireprotocol_types.cc \
		xmysqlnd/xmysqlnd_zval2any.cc \
		"

	xmysqlnd_cdkbase_parser=" \
		xmysqlnd/cdkbase/core/codec.cc \
		xmysqlnd/cdkbase/foundation/error.cc \
		xmysqlnd/cdkbase/foundation/stream.cc \
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
		xmysqlnd/proto_gen/mysqlx.pb.cc \
		xmysqlnd/proto_gen/mysqlx_connection.pb.cc \
		xmysqlnd/proto_gen/mysqlx_crud.pb.cc \
		xmysqlnd/proto_gen/mysqlx_cursor.pb.cc \
		xmysqlnd/proto_gen/mysqlx_datatypes.pb.cc \
		xmysqlnd/proto_gen/mysqlx_expect.pb.cc \
		xmysqlnd/proto_gen/mysqlx_expr.pb.cc \
		xmysqlnd/proto_gen/mysqlx_notice.pb.cc \
		xmysqlnd/proto_gen/mysqlx_prepare.pb.cc \
		xmysqlnd/proto_gen/mysqlx_resultset.pb.cc \
		xmysqlnd/proto_gen/mysqlx_session.pb.cc \
		xmysqlnd/proto_gen/mysqlx_sql.pb.cc \
		"

	MYSQL_XDEVAPI_SOURCES=" \
		$xmysqlnd_protobuf_sources \
		$mysqlx_devapi_sources \
		$mysqlx_util \
		$xmysqlnd_sources \
		$xmysqlnd_cdkbase_parser \
		$xmysqlnd_crud_parsers \
		"

	PHP_REQUIRE_CXX
	AC_LANG([C++])

	MYSQL_XDEVAPI_CXXFLAGS="-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++17 \
		-Wall -Wno-unused-function -Wformat-security -Wformat-extra-args"

	if test "$PHP_DEV_MODE" == "yes" || test "$PHP_DEV_MODE_ENABLED" == "yes"; then
		MYSQL_XDEVAPI_ADD_CXXFLAGS([-Werror])
		MYSQL_XDEVAPI_DEFINE(MYSQL_XDEVAPI_DEV_MODE)
	fi

	case $host_os in
		*freebsd*)
			dnl due to unknown reason AC_CHECK_HEADER and AC_CHECK_LIB don't search
			dnl /usr/local/* locations on FreeBSD, at least on our local testing farm (pb2)
			dnl adding them explicitly is patch to fix that case
			MYSQL_XDEVAPI_ADD_DIRS_TO_INCLUDE_SEARCH_PATH("/usr/local/include")
			MYSQL_XDEVAPI_ADD_DIRS_TO_LIB_SEARCH_PATH("/usr/local/lib")
			;;

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
			MYSQL_XDEVAPI_ADD_CXXFLAGS([-Wno-deprecated-declarations])
			;;
	esac

	dnl boost
	MINIMAL_BOOST_VER=105300
	MINIMAL_BOOST_VER_LABEL="1.53.00"

	PREFERRED_BOOST_VER_SUBDIR="boost_1_72_0"
	PREFERRED_BOOST_VER_LABEL="1.72.00"

	REQUIRED_BOOST_VER_MSG="required at least $MINIMAL_BOOST_VER_LABEL (preferred is $PREFERRED_BOOST_VER_LABEL)"

	if test -d "$WITH_BOOST"; then
		PREFERRED_BOOST_LOCATION=[$WITH_BOOST/$PREFERRED_BOOST_VER_SUBDIR]
	else
		PREFERRED_BOOST_LOCATION=""
	fi

	BOOST_SEARCH_DIRS="$PHP_BOOST $PREFERRED_BOOST_LOCATION $MYSQL_XDEVAPI_BOOST_ROOT $BOOST_ROOT $BOOST_PATH"
	AC_MSG_NOTICE([checking for boost $BOOST_SEARCH_DIRS])
	MYSQL_XDEVAPI_ADD_DIRS_TO_INCLUDE_SEARCH_PATH($BOOST_SEARCH_DIRS)
	AC_CHECK_HEADER("boost/version.hpp", [], [BOOST_INCLUDES_NOT_FOUND=1])

	if test $BOOST_INCLUDES_NOT_FOUND; then
		AC_MSG_ERROR([boost not found, please install it in system, consider use of --with-boost or setting MYSQL_XDEVAPI_BOOST_ROOT; $REQUIRED_BOOST_VER_MSG])
	fi

	AC_MSG_CHECKING([whether boost version is valid])
	AC_EGREP_CPP(
		boost_version_ok,
		[
			#include <boost/version.hpp>
			#if BOOST_VERSION >= $MINIMAL_BOOST_VER
				boost_version_ok
			#endif
		],
		[AC_MSG_RESULT([ok])],
		[AC_MSG_ERROR([boost version is too old, $REQUIRED_BOOST_VER_MSG])]
	)


	dnl protobuf
	PROTOBUF_SEARCH_DIRS="$PHP_PROTOBUF $MYSQL_XDEVAPI_PROTOBUF_ROOT $PROTOBUF_ROOT $PROTOBUF_PATH"
	AC_MSG_NOTICE([checking for protobuf $PROTOBUF_SEARCH_DIRS])

	MYSQL_XDEVAPI_ADD_DIRS_TO_INCLUDE_SEARCH_PATH($PROTOBUF_SEARCH_DIRS)
	AC_CHECK_HEADER("google/protobuf/any.h", [], [PROTOBUF_INCLUDES_NOT_FOUND=1])

	MYSQL_XDEVAPI_ADD_DIRS_TO_LIB_SEARCH_PATH($PROTOBUF_SEARCH_DIRS)
	AC_CHECK_LIB(protobuf, main, [], [PROTOBUF_LIB_NOT_FOUND=1])

	MYSQL_XDEVAPI_PREPARE_BINARY_SEARCH_PATH($PROTOBUF_SEARCH_DIRS, PROTOBUF_BINARY_SEARCH_PATH)
	AC_PATH_PROG(MYSQL_XDEVAPI_PROTOC, protoc, [PROTOBUF_PROTOC_NOT_FOUND=1], [$PROTOBUF_BINARY_SEARCH_PATH])

	if test -z $PROTOBUF_INCLUDES_NOT_FOUND && test -z $PROTOBUF_LIB_NOT_FOUND && test -z $PROTOBUF_PROTOC_NOT_FOUND; then
		PHP_SUBST(MYSQL_XDEVAPI_PROTOC)
		PHP_ADD_LIBRARY_WITH_PATH(protobuf, [], MYSQL_XDEVAPI_SHARED_LIBADD)
	else
		AC_MSG_ERROR([protobuf not found, please install it in system, consider use of --with-protobuf or setting MYSQL_XDEVAPI_PROTOBUF_ROOT])
	fi

	dnl exact path needed at generation of *.proto by protoc - see Makefile.frag, if not resolved then
	dnl protobuf lib is supposed to be in default location, and not necessary to pass explicitly to protoc
	AC_MSG_CHECKING([protobuf includes dir])
	MYSQL_XDEVAPI_RESOLVE_PATH($PROTOBUF_SEARCH_DIRS, "google/protobuf/any.h", "include", MYSQL_XDEVAPI_PROTOBUF_INCLUDES)
	if test -d $MYSQL_XDEVAPI_PROTOBUF_INCLUDES; then
		PHP_SUBST(MYSQL_XDEVAPI_PROTOBUF_INCLUDES)
	else
		MYSQL_XDEVAPI_PROTOBUF_INCLUDES="default system location"
	fi
	AC_MSG_RESULT($MYSQL_XDEVAPI_PROTOBUF_INCLUDES)


	dnl compressors
	MYSQL_XDEVAPI_RESOLVE_COMPRESSOR("$PHP_LZ4", "lz4", "lz4", "LZ4F_createCompressionContext", "lz4.h", "MYSQL_XDEVAPI_HAVE_LZ4", IS_LZ4_ENABLED)
	MYSQL_XDEVAPI_RESOLVE_COMPRESSOR("$PHP_ZLIB", "zlib", "z", "inflateEnd", "zlib.h", "MYSQL_XDEVAPI_HAVE_ZLIB", IS_ZLIB_ENABLED)
	MYSQL_XDEVAPI_RESOLVE_COMPRESSOR("$PHP_ZSTD", "zstd", "zstd", "ZSTD_createCStream", "zstd.h", "MYSQL_XDEVAPI_HAVE_ZSTD", IS_ZSTD_ENABLED)


	dnl CAUTION! PHP_NEW_EXTENSION defines variables like $ext_srcdir, $ext_builddir or
	dnl $PHP_PECL_EXTENSION. Should be called before they are used.
	PHP_NEW_EXTENSION(mysql_xdevapi, $MYSQL_XDEVAPI_SOURCES, $ext_shared,, $MYSQL_XDEVAPI_CXXFLAGS, true)
	PHP_SUBST(MYSQL_XDEVAPI_SHARED_LIBADD)

	PHP_ADD_INCLUDE([$ext_srcdir/xmysqlnd/cdkbase])
	PHP_ADD_INCLUDE([$ext_srcdir/xmysqlnd/cdkbase/include])
	PHP_ADD_INCLUDE([$ext_srcdir/xmysqlnd/cdkbase/extra/rapidjson/include])

	PHP_ADD_BUILD_DIR([$ext_srcdir])
	PHP_ADD_BUILD_DIR([$ext_srcdir/util])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/crud_parsers])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/proto_gen])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/core])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/foundation])
	PHP_ADD_BUILD_DIR([$ext_srcdir/xmysqlnd/cdkbase/parser])

	PHP_ADD_MAKEFILE_FRAGMENT()

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

	PHP_DEF_HAVE([MYSQL_XDEVAPI], 1, [mysql-xdevapi support enabled])

	dnl expose metadata
	dnl expose sources metadata
	INFO_SRC_PATH=[$ext_builddir/INFO_SRC]

	MYSQL_XDEVAPI_VERSION=`$EGREP "define PHP_MYSQL_XDEVAPI_VERSION" $ext_srcdir/php_mysql_xdevapi.h | $SED -e 's/[[^0-9\.]]//g'`
	echo [MySQL X DevAPI for PHP ${MYSQL_XDEVAPI_VERSION}] > $INFO_SRC_PATH
	echo [version: ${MYSQL_XDEVAPI_VERSION}] >> $INFO_SRC_PATH

 	AC_PATH_PROG(GIT_EXEC_PATH, 'git')
	if test -x "${GIT_EXEC_PATH}"; then
		IS_GIT_REPO=`git rev-parse --is-inside-work-tree`
	fi

	if test "${IS_GIT_REPO}"; then
		BRANCH_NAME=`git symbolic-ref --short HEAD`
		echo [branch: $BRANCH_NAME] >> $INFO_SRC_PATH

		COMMIT_INFO=`git log -1 --pretty=format:"commit: %H%ndate: %aD%nshort: %h"`
		echo "${COMMIT_INFO}" >> $INFO_SRC_PATH
	else
		# internal use, below envars available only on pb2 hosts without git
		if test "${BRANCH_SOURCE}"; then
			# e.g. export BRANCH_SOURCE='http://myrepo.no.oracle.com/git/connector-php-devapi.git wl-12276-expose-metadata'
			BRANCH_NAME=`echo ${BRANCH_SOURCE} | cut -d' ' -f2`
			echo [branch: ${BRANCH_NAME}] >> $INFO_SRC_PATH
		fi

		if test "${PUSH_REVISION}"; then
			echo [commit: ${PUSH_REVISION}] >> $INFO_SRC_PATH
		fi
	fi

	# expose binaries metadata
	INFO_BIN_PATH=[$ext_builddir/INFO_BIN]

	echo [===== Information about the build process: =====]] > $INFO_BIN_PATH
	CURRENT_TIME=`date -u`
	HOSTNAME=`hostname`
	echo [Build was run at ${CURRENT_TIME} on host ${HOSTNAME}] >> $INFO_BIN_PATH
	HOST_OS=`uname -a`
	echo [Build was done on ${HOST_OS}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

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

	echo [] >> $INFO_BIN_PATH

	echo [===== Feature flags used: =====] >> $INFO_BIN_PATH
	echo [php-config: ${PHP_CONFIG}] >> $INFO_BIN_PATH
    if test "$enable_maintainer_zts" == "yes"; then
      THREAD_SAFETY="yes"
    else
      THREAD_SAFETY="no"
    fi
	echo [thread-safety: ${THREAD_SAFETY}] >> $INFO_BIN_PATH
	echo [debug: ${ZEND_DEBUG}] >> $INFO_BIN_PATH
	echo [developer-mode: ${PHP_DEV_MODE}] >> $INFO_BIN_PATH

	echo [] >> $INFO_BIN_PATH

	echo [===== Compiler flags used: =====] >> $INFO_BIN_PATH
	echo [CC: ${CC}] >> $INFO_BIN_PATH
	echo [CFLAGS: ${CFLAGS}] >> $INFO_BIN_PATH
	echo [CXX: ${CXX}] >> $INFO_BIN_PATH
	echo [CXXFLAGS: ${CXXFLAGS}] >> $INFO_BIN_PATH
	echo [CPPFLAGS: ${CPPFLAGS}] >> $INFO_BIN_PATH
	echo [MYSQL_XDEVAPI_CXXFLAGS: ${MYSQL_XDEVAPI_CXXFLAGS}] >> $INFO_BIN_PATH
	echo [LDFLAGS: ${LDFLAGS}] >> $INFO_BIN_PATH
	echo [PHP_LDFLAGS: ${PHP_LDFLAGS}] >> $INFO_BIN_PATH

	echo [] >> $INFO_BIN_PATH

	echo [===== Libraries: =====] >> $INFO_BIN_PATH
	echo [--with-boost: ${PHP_BOOST}] >> $INFO_BIN_PATH
	echo [MYSQL_XDEVAPI_BOOST_ROOT: ${MYSQL_XDEVAPI_BOOST_ROOT}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	PROTOC_VERSION=`${MYSQL_XDEVAPI_PROTOC} --version`
	echo [--with-protobuf: ${PHP_PROTOBUF}] >> $INFO_BIN_PATH
	echo [MYSQL_XDEVAPI_PROTOBUF_ROOT: ${MYSQL_XDEVAPI_PROTOBUF_ROOT}] >> $INFO_BIN_PATH
	echo [version: ${PROTOC_VERSION}] >> $INFO_BIN_PATH
	echo [protoc path: ${MYSQL_XDEVAPI_PROTOC}] >> $INFO_BIN_PATH
	echo [includes path: ${MYSQL_XDEVAPI_PROTOBUF_INCLUDES}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [--with-lz4: ${PHP_LZ4}] >> $INFO_BIN_PATH
	echo [lz4 enabled: ${IS_LZ4_ENABLED}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [--with-zlib: ${PHP_ZLIB}] >> $INFO_BIN_PATH
	echo [zlib enabled: ${IS_ZLIB_ENABLED}] >> $INFO_BIN_PATH
	echo [] >> $INFO_BIN_PATH

	echo [--with-zstd: ${PHP_ZSTD}] >> $INFO_BIN_PATH
	echo [zstd enabled: ${IS_ZSTD_ENABLED}] >> $INFO_BIN_PATH
	echo [===== EOF =====] >> $INFO_BIN_PATH
fi
