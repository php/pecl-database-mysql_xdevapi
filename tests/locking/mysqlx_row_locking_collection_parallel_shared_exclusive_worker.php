<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/mysqlx_row_locking.inc");

	function send_current_state($res1, $res2) {
		$result_msg = strval($res1['n']) . " " . strval($res2['n']);
		echo $result_msg, "\n";
	}

	notify_worker_started();

	$session = mysql_xdevapi\getSession($connection_uri);

	$schema = $session->getSchema($test_schema_name);
	$coll = $schema->getCollection($test_collection_name);

	$session->startTransaction();

	recv_let_worker_modify();

	// should return immediately
	check_find_lock_one($coll, '2', 2, $Lock_exclusive);
	modify_row($coll, '2', 22);
	check_find_lock_one($coll, '2', 22, $Lock_exclusive);

	// should return immediately
	check_find_lock_one($coll, '3', 3, $Lock_exclusive);

	// $session2 should block
	check_find_lock_one($coll, '1', 1, $Lock_exclusive);
	modify_row($coll, '1', 11);
	check_find_lock_one($coll, '1', 11, $Lock_exclusive);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();
?>
