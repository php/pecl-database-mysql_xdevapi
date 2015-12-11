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
dnl
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_connection.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_crud.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_datatypes.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_expect.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_expr.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_notice.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_resultset.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_session.proto
dnl protoc-c --c_out proto_gen_c --proto_path proto_def/ proto_def/mysqlx_sql.proto
dnl
dnl gcc -c proto_gen_c/*.c -lprotobuf-c


PHP_ARG_ENABLE(xmysqlnd, whether to enable xmysqlnd,
  [  --enable-xmysqlnd       Enable xmysqlnd], no, yes)

PHP_ARG_WITH(protobufc, protobuf compiler,
  [  --with-protobuf-c[=DIR]       xmysqlnd: protobuf installation directory], no, no)

dnl If some extension uses xmysqlnd it will get compiled in PHP core
if test "$PHP_XMYSQLND" != "no" || test "$PHP_XMYSQLND_ENABLED" = "yes"; then
  PHP_REQUIRE_CXX

  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="include/google/protobuf-c/protobuf-c.h"
  if test -r $PHP_MYSQLPP/$SEARCH_FOR; then # path given as parameter
     PROTOBUFC_DIR=$PHP_PROTOBUFC
  else # search default path list
     AC_MSG_CHECKING([for protobuf-c files in default path])
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         PROTOBUFC_DIR=$i/lib/google/protobuf-c/
         AC_MSG_RESULT(Header found in $i/include/google/protobuf-c/)
       fi
     done
  fi

  xmysqlnd_base_sources="php_xmysqlnd.c \
                         xmysqlnd_driver.c \
						 xmysqlnd_extension_plugin.c \
						 xmysqlnd_node_session.c \
						 xmysqlnd_protocol_frame_codec.c \
                         xmysqlnd_statistics.c \
						 xmysqlnd_wireprotocol.cc \
						 xmysqlnd_zval2any.cc \
						 php_mysqlx.c \
						 mysqlx_class_properties.c \
						 mysqlx_driver.c \
						 mysqlx_exception.c \
						 mysqlx_message__error.cc \
						 mysqlx_message__ok.cc \
						 mysqlx_message__auth_start.cc \
						 mysqlx_message__auth_continue.cc \
						 mysqlx_message__auth_ok.cc \
						 mysqlx_message__capabilities_get.cc \
						 mysqlx_message__capabilities_set.cc \
						 mysqlx_message__capabilities.cc \
						 mysqlx_message__capability.c \
						 mysqlx_message__stmt_execute.cc \
						 mysqlx_message__stmt_execute_ok.cc \
						 mysqlx_resultset__column_metadata.cc \
						 mysqlx_resultset__resultset_metadata.cc \
						 mysqlx_resultset__data_row.cc \
						 mysqlx_message__data_fetch_done.cc \
						 mysqlx_node_connection.c \
						 mysqlx_node_pfc.c \
						 mysqlx_node_session.c \
						 mysqlx_object.c \
						 "
  xmysqlnd_protobufc_sources="proto_gen_c/mysqlx_connection.pb-c.c \
  							 proto_gen_c/mysqlx_crud.pb-c.c \
							 proto_gen_c/mysqlx_datatypes.pb-c.c \
							 proto_gen_c/mysqlx_expect.pb-c.c \
							 proto_gen_c/mysqlx_expr.pb-c.c \
							 proto_gen_c/mysqlx_notice.pb-c.c \
							 proto_gen_c/mysqlx.pb-c.c \
							 proto_gen_c/mysqlx_resultset.pb-c.c \
							 proto_gen_c/mysqlx_session.pb-c.c \
							 proto_gen_c/mysqlx_sql.pb-c.c \
						 "

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


  AC_DEFINE([XMYSQLND_SSL_SUPPORTED], 1, [Enable core xmysqlnd SSL code])

  test -z "$PHP_OPENSSL" && PHP_OPENSSL=no

  if test "$PHP_OPENSSL" != "no" || test "$PHP_OPENSSL_DIR" != "no"; then
    AC_CHECK_LIB(ssl, DSA_get_default_method, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))
    AC_CHECK_LIB(crypto, X509_free, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))

    PHP_SETUP_OPENSSL(XMYSQLND_SHARED_LIBADD, [AC_DEFINE(XMYSQLND_HAVE_SSL,1,[Enable xmysqlnd code that uses OpenSSL directly])])
  fi

  LIBNAME=protobuf-c
  LIBSYMBOL=protobuf_c_service_destroy

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
      PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, PROTOBUFC_DIR, XMYSQLND_SHARED_LIBADD)
      AC_DEFINE(HAVE_PROTOBUFC,1,[ ])
  ],[
      AC_MSG_ERROR([wrong protobuf-c library])
  ],[
     -L$PROTOBUFC_DIR
  ])


  PHP_ADD_LIBRARY(protobuf)

  PHP_SUBST(XMYSQLND_SHARED_LIBADD)

  PHP_ADD_BUILD_DIR($ext_builddir/proto_gen_c)

  xmysqlnd_sources="$xmysqlnd_base_sources $xmysqlnd_protobuf_sources"
  PHP_NEW_EXTENSION(xmysqlnd, $xmysqlnd_sources, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR([ext/xmysqlnd], 1)
  PHP_INSTALL_HEADERS([ext/xmysqlnd/])
fi

if test "$PHP_XMYSQLND" != "no" || test "$PHP_XMYSQLND_ENABLED" = "yes" || test "$PHP_MYSQLI" != "no"; then
  PHP_ADD_BUILD_DIR([ext/xmysqlnd], 1)
fi
