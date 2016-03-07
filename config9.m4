dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_connection.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_crud.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_datatypes.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_expect.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_expr.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_notice.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_resultset.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_session.proto
dnl protoc --cpp_out proto_gen/ --proto_path proto_def/ proto_def/mysqlx_sql.proto
dnl
dnl g++ -c proto_gen/*.cc -lprotobuf

PHP_ARG_ENABLE(xmysqlnd, whether to enable xmysqlnd,
  [  --enable-xmysqlnd       Enable xmysqlnd], no, yes)

PHP_ARG_ENABLE(xmysqlnd_experimental_features, whether to disable experimental features in xmysqlnd,
  [  --disable-xmysqlnd-experimental-features
                          Disable support for the experimental features in xmysqlnd], yes, no)

dnl If some extension uses xmysqlnd it will get compiled in PHP core
if test "$PHP_XMYSQLND" != "no" || test "$PHP_XMYSQLND_ENABLED" = "yes"; then
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
      XMYSQLND_BOOST_FUNCTION=$i/$SEARCH_FOR
      AC_DEFINE([XMYSQLND_BOOST_FUNCTION], $XMYSQLND_BOOST_FUNCTION, [Enable experimental features])
      AC_MSG_RESULT(Header found in $XMYSQLND_BOOST_FUNCTION)
    fi
  done

  if test "$PHP_XMYSQLND_EXPERIMENTAL_FEATURES" != "no"; then
    AC_DEFINE([XMYSQLND_EXPERIMENTAL_FEATURES], 1, [Enable experimental features])
  fi

  xmysqlnd_protobuf_sources="proto_gen/mysqlx_connection.pb.cc \
							 proto_gen/mysqlx_crud.pb.cc \
							 proto_gen/mysqlx_datatypes.pb.cc \
							 proto_gen/mysqlx_expect.pb.cc \
							 proto_gen/mysqlx_expr.pb.cc \
							 proto_gen/mysqlx_notice.pb.cc \
							 proto_gen/mysqlx.pb.cc \
							 proto_gen/mysqlx_resultset.pb.cc \
							 proto_gen/mysqlx_session.pb.cc \
							 proto_gen/mysqlx_sql.pb.cc \
					"

  xmysqlnd_expr_parser="     xmysqlnd/crud_parsers/expression_parser.cc \
							 xmysqlnd/crud_parsers/orderby_parser.cc \
							 xmysqlnd/crud_parsers/projection_parser.cc \
					"


  xmysqlnd_sources="     php_xmysqlnd.c \
						 xmysqlnd/xmysqlnd_driver.c \
						 xmysqlnd/xmysqlnd_extension_plugin.c \
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
						 xmysqlnd/xmysqlnd_warning_list.c \
						 xmysqlnd/xmysqlnd_wireprotocol.cc \
						 xmysqlnd/xmysqlnd_zval2any.cc \
					"

  AC_DEFINE([XMYSQLND_SSL_SUPPORTED], 1, [Enable core xmysqlnd SSL code])

  test -z "$PHP_OPENSSL" && PHP_OPENSSL=no

  if test "$PHP_OPENSSL" != "no" || test "$PHP_OPENSSL_DIR" != "no"; then
    AC_CHECK_LIB(ssl, DSA_get_default_method, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))
    AC_CHECK_LIB(crypto, X509_free, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))

    PHP_SETUP_OPENSSL(XMYSQLND_SHARED_LIBADD, [AC_DEFINE(XMYSQLND_HAVE_SSL,1,[Enable xmysqlnd code that uses OpenSSL directly])])
  fi

  PHP_ADD_LIBRARY(protobuf)

  PHP_SUBST(XMYSQLND_SHARED_LIBADD)

  PHP_ADD_BUILD_DIR($ext_builddir/messages)
  PHP_ADD_BUILD_DIR($ext_builddir/proto_gen)

  this_ext_sources="$xmysqlnd_protobuf_sources $xmysqlnd_expr_parser $xmysqlnd_sources"
  PHP_NEW_EXTENSION(xmysqlnd, $this_ext_sources, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR([ext/xmysqlnd], 1)
  PHP_INSTALL_HEADERS([ext/xmysqlnd/])
fi

if test "$PHP_XMYSQLND" != "no" || test "$PHP_XMYSQLND_ENABLED" = "yes" || test "$PHP_MYSQLI" != "no"; then
  PHP_ADD_BUILD_DIR([ext/xmysqlnd], 1)
fi
