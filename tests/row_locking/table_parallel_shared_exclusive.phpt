--TEST--
mysqlx table row locking parallel shared read before exclusive write
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	$session = mysql_xdevapi\getSession($connection_uri);
	$tab = create_test_table($session);

	$worker_process = run_worker(__FILE__);
	if (is_resource($worker_process))
	{
		recv_worker_started();

		$session->startTransaction();
		check_select_lock_all($tab, ['1', '2', '3'], [1, 2, 3], $Lock_shared);

		send_let_worker_modify();

		check_select_lock_one($tab, '4', 4, $Lock_shared);

		send_let_worker_block();

		$session->commit();

		check_select_lock_one($tab, '4', 4, $Lock_shared);

		send_let_worker_commit();
		recv_worker_committed();

		check_select_lock_all($tab, ['4', '5', '6'], [4, 55, 66], $Lock_shared);
		check_select_lock_all($tab, ['1', '2', '3'], [11, 22, 3], $Lock_shared);

		recv_msg_from_worker("ok");
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
worker cmd-line:%s
worker started
let worker modify
let worker block
let worker commit
worker committed
ok
let worker end
done!%A
