<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	function send_current_state($res1, $res2) {
		$result_msg = strval($res1['n']) . " " . strval($res2['n']);
		echo $result_msg, "\n";
	}

	notify_worker_started();

	$session = mysql_xdevapi\getSession($connection_uri);

	$schema = $session->getSchema($test_schema_name);
	$coll = $schema->getCollection($test_collection_name);

	$session->startTransaction();

	$res2 = find_lock_one($coll, '4', $Lock_shared);

	$res1 = find_lock_one($coll, '3', $Lock_shared);

	send_current_state($res1, $res2);

 	recv_let_worker_block();

	$res2 = find_lock_one($coll, '5', $Lock_shared);
	$res1 = find_lock_one($coll, '6', $Lock_shared);
	send_current_state($res1, $res2);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();

	$res1 = find_lock_one($coll, '1', $Lock_shared);
	$res2 = find_lock_one($coll, '2', $Lock_shared);
	send_current_state($res1, $res2);

	$res1 = find_lock_one($coll, '5', $Lock_shared);
	$res2 = find_lock_one($coll, '6', $Lock_shared);
	send_current_state($res1, $res2);
?>
