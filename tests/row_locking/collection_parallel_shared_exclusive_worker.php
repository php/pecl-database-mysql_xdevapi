<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	notify_worker_started();

	$session = mysql_xdevapi\getSession($connection_uri);

	$schema = $session->getSchema($test_schema_name);
	$coll = $schema->getCollection($test_collection_name);

	$session->startTransaction();

	recv_let_worker_modify();

	check_find_lock_all($coll, ['4', '5'], [4, 5], $Lock_exclusive);
	modify_row($coll, '4', 44);
	modify_row($coll, '5', 55);
	check_find_lock_all($coll, ['4', '5'], [44, 55], $Lock_exclusive);

	recv_let_worker_block();

	check_find_lock_one($coll, '3', 3, $Lock_exclusive);

	check_find_lock_one($coll, '2', 2, $Lock_exclusive);
	modify_row($coll, '2', 22);
	check_find_lock_one($coll, '2', 22, $Lock_exclusive);

	check_find_lock_one($coll, '1', 1, $Lock_exclusive);
	modify_row($coll, '1', 11);
	check_find_lock_one($coll, '1', 11, $Lock_exclusive);

	recv_let_worker_commit();
	$session->commit();
	check_find_lock_all($coll, ['4', '5'], [44, 55], $Lock_shared);
	notify_worker_committed();

	send_verification_status();
	recv_let_worker_end();
?>
