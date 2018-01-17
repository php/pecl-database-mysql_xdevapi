<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	$session = mysql_xdevapi\getSession($connection_uri);

	$coll = create_test_collection($session);

	notify_worker_started();

	$session->startTransaction();

	recv_let_worker_modify();

	find_lock_one($coll, '3', $Lock_exclusive);
	modify_row($coll, '3', 333);

	find_lock_one($coll, '4', $Lock_exclusive);
	modify_row($coll, '4', 444);

	recv_let_worker_block();

	check_find_lock_one($coll, '1', 11, $Lock_exclusive);
	check_find_lock_one($coll, '2', 22, $Lock_exclusive);

	find_lock_one($coll, '5', $Lock_exclusive);
	modify_row($coll, '5', 55);

	find_lock_one($coll, '1', $Lock_exclusive);
	modify_row($coll, '1', 111);

	find_lock_one($coll, '2', $Lock_exclusive);
	modify_row($coll, '2', 222);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();

	recv_let_worker_end();
?>
