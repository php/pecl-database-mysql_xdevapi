<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	notify_worker_started();

	$session = mysql_xdevapi\getSession($connection_uri);

	$schema = $session->getSchema($test_schema_name);
	$tab = $schema->getTable($test_table_name);

	$session->startTransaction();

	recv_let_worker_modify();

	check_select_lock_one($tab, '5', 5, $Lock_exclusive);
	update_row($tab, '5', 55);
	check_select_lock_one($tab, '5', 55, $Lock_exclusive);

	check_select_lock_one($tab, '6', 6, $Lock_exclusive);
	update_row($tab, '6', 66);
	check_select_lock_one($tab, '6', 66, $Lock_exclusive);

	recv_let_worker_block();

	check_select_lock_one($tab, '2', 2, $Lock_exclusive);
	update_row($tab, '2', 22);
	check_select_lock_one($tab, '2', 22, $Lock_exclusive);

	check_select_lock_one($tab, '3', 3, $Lock_exclusive);

	check_select_lock_one($tab, '1', 1, $Lock_exclusive);
	update_row($tab, '1', 11);
	check_select_lock_one($tab, '1', 11, $Lock_exclusive);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();

	send_verification_status();
	recv_let_worker_end();
?>
