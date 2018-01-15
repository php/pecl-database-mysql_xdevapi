--TEST--
mysqlx collection row locking parallel exclusive write before exclusive write
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	assert_mysql_xdevapi_loaded();

	$worker_process = run_worker(__FILE__);
	if (is_resource($worker_process)) {
		recv_worker_started();

		$session = mysql_xdevapi\getSession($connection_uri);
		$schema = $session->getSchema($test_schema_name);
		$coll = $schema->getCollection($test_collection_name);

		$session->startTransaction();
		check_find_lock_all($coll, ['1', '2'], [1, 2], $Lock_exclusive);
		modify_row($coll, '1', 11);
		modify_row($coll, '2', 22);
		check_find_lock_all($coll, ['1', '2'], [11, 22], $Lock_exclusive);

		send_let_worker_modify();

		check_find_lock_one($coll, '5', 5, $Lock_exclusive);

		send_let_worker_block();

		find_lock_one($coll, '6', $Lock_exclusive);
		modify_row($coll, '6', 66);

		$session->commit();

		send_let_worker_commit();
		recv_worker_committed();

		check_find_lock_all($coll, ['1', '2'], [111, 222], $Lock_exclusive);
		check_find_lock_all($coll, ['3', '4'], [333, 444], $Lock_exclusive);
		check_find_lock_all($coll, ['5', '6'], [55, 66], $Lock_exclusive);

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
let worker end
done!%A
