--TEST--
mysqlx table row locking parallel exclusive write before shared read
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
		check_select_lock_all($tab, ['1', '2'], [1, 2], $Lock_exclusive);
		update_row($tab, '1', 11);
		update_row($tab, '2', 22);
		check_select_lock_all($tab, ['1', '2'], [11, 22], $Lock_exclusive);

		$expected_result = "4 3";
		recv_msg_from_worker($expected_result);

		check_select_lock_all($tab, ['5', '6'], [5, 6], $Lock_exclusive);
		update_row($tab, '5', 55);
		update_row($tab, '6', 66);
		check_select_lock_all($tab, ['5', '6'], [55, 66], $Lock_exclusive);

		send_let_worker_block();

		$session->commit();

		$expected_result = "55 22";
		recv_msg_from_worker($expected_result);

		send_let_worker_commit();
		recv_worker_committed();

		$expected_result = "11 22";
		recv_msg_from_worker($expected_result);

		check_select_lock_one($tab, '1', 11, $Lock_exclusive);
		check_select_lock_one($tab, '2', 22, $Lock_exclusive);

		$expected_result = "55 66";
		recv_msg_from_worker($expected_result);

		check_select_lock_one($tab, '5', 55, $Lock_exclusive);
		check_select_lock_one($tab, '6', 66, $Lock_exclusive);
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
4 3
let worker block
55 22
let worker commit
worker committed
11 22
55 66
done!%A
