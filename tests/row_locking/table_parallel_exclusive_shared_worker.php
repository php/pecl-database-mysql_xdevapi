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
	$tab = $schema->getTable($test_table_name);

	$session->startTransaction();

	$res2 = select_lock_one($tab, '3', $Lock_shared);
	$res1 = select_lock_one($tab, '4', $Lock_shared);
	send_current_state($res1, $res2);

	recv_let_worker_block();

	$res1 = select_lock_one($tab, '5', $Lock_shared);
	$res2 = select_lock_one($tab, '2', $Lock_shared);
	send_current_state($res1, $res2);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();

	$res1 = select_lock_one($tab, '1', $Lock_shared);
	$res2 = select_lock_one($tab, '2', $Lock_shared);
	send_current_state($res1, $res2);

	$res1 = select_lock_one($tab, '5', $Lock_shared);
	$res2 = select_lock_one($tab, '6', $Lock_shared);
	send_current_state($res1, $res2);
?>
