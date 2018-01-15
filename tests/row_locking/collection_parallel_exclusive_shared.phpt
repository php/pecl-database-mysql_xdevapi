--TEST--
mysqlx collection row locking parallel exclusive write before shared read
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	$session = mysql_xdevapi\getSession($connection_uri);
	$coll = create_test_collection($session);

	$worker_process = run_worker(__FILE__);
	if (is_resource($worker_process))
	{
		recv_worker_started();

		$session->startTransaction();
		check_find_lock_all($coll, ['1', '2'], [1, 2], $Lock_exclusive);
		modify_row($coll, '1', 11);
		modify_row($coll, '2', 22);
		check_find_lock_all($coll, ['1', '2'], [11, 22], $Lock_exclusive);

		check_find_lock_one($coll, '3', 3, $Lock_shared);

		$expected_result = "3 4";
		recv_msg_from_worker($expected_result);

		check_find_lock_all($coll, ['5', '6'], [5, 6], $Lock_exclusive);
		send_let_worker_block();

		modify_row($coll, '5', 55);
		modify_row($coll, '6', 66);
		check_find_lock_all($coll, ['5', '6'], [55, 66], $Lock_exclusive);

		$session->commit();

		send_let_worker_commit();

		$expected_result = "66 55";
		recv_msg_from_worker($expected_result);

		recv_worker_committed();

		$expected_result = "11 22";
		recv_msg_from_worker($expected_result);

		check_find_lock_one($coll, '1', 11, $Lock_exclusive);
		check_find_lock_one($coll, '2', 22, $Lock_exclusive);

		$expected_result = "55 66";
		recv_msg_from_worker($expected_result);
		check_find_lock_all($coll, ['5', '6'], [55, 66], $Lock_exclusive);
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
3 4
let worker block
let worker commit
66 55
worker committed
11 22
55 66
done!%A
