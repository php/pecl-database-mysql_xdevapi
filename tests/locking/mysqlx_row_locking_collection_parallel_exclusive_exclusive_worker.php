<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/mysqlx_row_locking.inc");

	assert_mysql_xdevapi_loaded();

	$session = mysql_xdevapi\getSession($connection_uri);

	$coll = createTestCollection($session);

	notify_worker_started();

	$session->startTransaction();

	recv_let_worker_modify();

	find_lock_one($coll, '1', $Lock_exclusive);
	modify_row($coll, '1', 111);

	find_lock_one($coll, '2', $Lock_exclusive);
	modify_row($coll, '2', 222);

	recv_let_worker_commit();
	$session->commit();
	notify_worker_committed();

	recv_let_worker_end();
?>
