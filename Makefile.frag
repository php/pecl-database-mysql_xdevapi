# TODO: We're writing into srcdir, not builddir!
protofiles: $(srcdir)/xmysqlnd/proto_def/mysqlx_connection.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_datatypes.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_expr.proto $(srcdir)/xmysqlnd/proto_def/mysqlx.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_session.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_crud.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_expect.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_notice.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_resultset.proto $(srcdir)/xmysqlnd/proto_def/mysqlx_sql.proto
	$(protoc) --cpp_out $(srcdir)/xmysqlnd/proto_gen/ --proto_path $(srcdir)/xmysqlnd/proto_def/ $(srcdir)/xmysqlnd/proto_def/*.proto

$(srcdir)/xmysqlnd/proto_gen/mysqlx.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_connection.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_crud.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_datatyes.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_expect.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_expr.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_notice.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_esultset.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_session.cc: protofiles
$(srcdir)/xmysqlnd/proto_gen/mysqlx_sql.cc: protofiles

