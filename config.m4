PHP_ARG_ENABLE(mysqlx, whether to enable mysqlx,
  [  --enable-mysqlx       Enable myxsqlx], no, yes)

PHP_ARG_ENABLE(mysqlx_experimental_features, whether to disable experimental features in mysqlx,
  [  --disable-mysqlx-experimental-features
                          Disable support for the experimental features in mysqlx], yes, no)

PHP_ARG_ENABLE(mysqlx_message_classes, whether to enable the experimental message classes in mysqlx,
  [  --enable-mysqlx-message-classes
                          Enable support for the experimental message classes in mysqlx], yes, no)


if test "$PHP_MYSQLX" != "no" || test "$PHP_MYSQLX_ENABLED" = "yes"; then
  PHP_REQUIRE_CXX

  if test "$PHP_MYSQLX_EXPERIMENTAL_FEATURES" != "no"; then
    AC_DEFINE([MYSQLX_EXPERIMENTAL_FEATURES], 1, [Enable experimental features])
  fi

  mysqlx_messages=""
  if test "$PHP_MYSQLX_MESSAGE_CLASSES" != "no" || test "$PHP_MYSQLX_MESSAGE_CLASSES_ENABLED" = "yes"; then
    AC_DEFINE([MYSQLX_MESSAGE_CLASSES], 1, [Enable message classes])

	mysqlx_messages="   messages/mysqlx_node_connection.c \
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

  mysqlx_base_sources="	 php_mysqlx.c \
						 php_mysqlx_ex.c \
						 mysqlx_class_properties.c \
						 mysqlx_database_object.c \
						 mysqlx_driver.c \
						 mysqlx_executable.c \
						 mysqlx_exception.c \
						 mysqlx_execution_status.c \
						 mysqlx_field_metadata.c \
						 mysqlx_node_schema.c \
						 mysqlx_node_session.c \
						 mysqlx_node_collection.c \
						 mysqlx_node_collection__add.c \
						 mysqlx_node_collection__find.c \
						 mysqlx_node_collection__modify.c \
						 mysqlx_node_collection__remove.c \
						 mysqlx_node_table.c \
						 mysqlx_node_table__delete.c \
						 mysqlx_node_table__insert.c \
						 mysqlx_node_table__select.c \
						 mysqlx_node_table__update.c \
						 mysqlx_node_sql_statement.c \
						 mysqlx_node_sql_statement_result.c \
						 mysqlx_node_sql_statement_result_iterator.c \
						 mysqlx_object.c \
						 mysqlx_session.c \
						 mysqlx_schema_object.c \
						 mysqlx_warning.c \
					"


  test -z "$PHP_OPENSSL" && PHP_OPENSSL=no

  if test "$PHP_OPENSSL" != "no" || test "$PHP_OPENSSL_DIR" != "no"; then
    AC_CHECK_LIB(ssl, DSA_get_default_method, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))
    AC_CHECK_LIB(crypto, X509_free, AC_DEFINE(HAVE_DSA_DEFAULT_METHOD, 1, [OpenSSL 0.9.7 or later]))

    PHP_SETUP_OPENSSL(MYSQLX_SHARED_LIBADD, [AC_DEFINE(MYSQLX_HAVE_SSL,1,[Enable mysqlx code that uses OpenSSL directly])])
  fi

  PHP_ADD_LIBRARY(protobuf)

  PHP_SUBST(MYSQLX_SHARED_LIBADD)

  PHP_ADD_BUILD_DIR($ext_builddir/messages)
  PHP_ADD_BUILD_DIR($ext_builddir/proto_gen)

  this_ext_sources="$mysqlx_base_sources $mysqlx_messages"
  PHP_NEW_EXTENSION(mysqlx, $this_ext_sources, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR([ext/xmysqlnd], 1)

  PHP_INSTALL_HEADERS([ext/xmysqlnd/])
fi
