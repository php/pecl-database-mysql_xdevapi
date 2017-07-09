--TEST--
mysqlx collection row locking parallel exclusive write before exclusive write
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/mysqlx_row_locking.inc");

	$session = mysql_xdevapi\getSession($connection_uri);
	$coll = createTestCollection($session);

	$worker_process = run_worker(__FILE__);
	if (is_resource($worker_process))
	{
		recv_worker_started();

		$session->startTransaction();
		check_find_lock_all($coll, ['1', '2'], [1, 2], $Lock_exclusive);
		modify_row($coll, '1', 11);
		modify_row($coll, '2', 22);
		check_find_lock_all($coll, ['1', '2'], [11, 22], $Lock_exclusive);

		send_let_worker_modify();

		$session->commit(); // worker should unblock now

		check_find_lock_one($coll, '1', 11, $Lock_exclusive);
		check_find_lock_one($coll, '2', 22, $Lock_exclusive);

		send_let_worker_commit();
		recv_worker_committed();

		check_find_lock_one($coll, '1', 111, $Lock_exclusive);
		check_find_lock_one($coll, '2', 222, $Lock_exclusive);

		send_let_worker_end();
	}

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
worker started
let worker modify
let worker commit
worker committed
let worker end
done!%A
