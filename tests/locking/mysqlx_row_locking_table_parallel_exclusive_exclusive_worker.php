<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/mysqlx_row_locking.inc");

	assert_mysql_xdevapi_loaded();
	notify_worker_started();

	$session = mysql_xdevapi\getSession($connection_uri);

	$schema = $session->getSchema($test_schema_name);
	$tab = $schema->getTable($test_table_name);

	$session->startTransaction();

	recv_let_worker_modify();

	select_lock_one($tab, '1', $Lock_exclusive);
	update_row($tab, '1', 111);

	select_lock_one($tab, '2', $Lock_exclusive);
	update_row($tab, '2', 222);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();
?>
