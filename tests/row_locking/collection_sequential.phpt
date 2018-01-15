--TEST--
mysqlx collection row locking sequential
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	$session1 = mysql_xdevapi\getSession($connection_uri);
	$session2 = mysql_xdevapi\getSession($connection_uri);

	$col1 = create_test_collection($session1);

	$schema2 = $session2->getSchema($test_schema_name);
	$col2 = $schema2->getCollection($test_collection_name);

	// test1: Shared Lock
	$session1->startTransaction();
	check_find_lock_one($col1, '1', 1, $Lock_shared);
	check_find_lock_one($col1, '5', 5, $Lock_shared);

	$session2->startTransaction();

	check_find_lock_one($col2, '2', 2, $Lock_shared);
	check_find_lock_one($col2, '1', 1, $Lock_shared);
	check_find_lock_one($col2, '6', 6, $Lock_shared);

	check_find_lock_one($col1, '6', 6, $Lock_shared);

	check_find_lock_one($col2, '5', 5, $Lock_shared);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive
	$session1->startTransaction();
	check_find_lock_one($col1, '1', 1, $Lock_exclusive);
	check_find_lock_one($col1, '3', 3, $Lock_exclusive);

	$session2->startTransaction();

	check_find_lock_one($col2, '2', 2, $Lock_shared);

	check_find_lock_one($col1, '5', 5, $Lock_exclusive);

	check_find_lock_one($col2, '4', 4, $Lock_shared);

	check_find_lock_one($col1, '6', 6, $Lock_exclusive);

	$session1->rollback();

	check_find_lock_one($col2, '1', 1, $Lock_shared);
	check_find_lock_one($col2, '3', 3, $Lock_shared);
	check_find_lock_one($col2, '6', 6, $Lock_shared);

	$session2->rollback();


	// test3: Exclusive after Shared
	$session1->startTransaction();
	check_find_lock_all($col1, ['1', '3'], [1, 3], $Lock_shared);

	$session2->startTransaction();

	check_find_lock_one($col2, '2', 2, $Lock_exclusive);
	check_find_lock_one($col1, '5', 5, $Lock_shared);
	check_find_lock_one($col2, '4', 4, $Lock_exclusive);

	$session1->rollback();
	check_find_lock_all($col2, ['1', '3'], [1, 3], $Lock_exclusive);
	check_find_lock_one($col2, '5', 5, $Lock_exclusive);

	$session2->rollback();


	// test4: Exclusive after Exclusive
	$session1->startTransaction();
	check_find_lock_one($col1, '1', 1, $Lock_exclusive);
	check_find_lock_one($col1, '3', 3, $Lock_exclusive);

	$session2->startTransaction();
	check_find_lock_one($col2, '2', 2, $Lock_exclusive);
	check_find_lock_one($col1, '4', 4, $Lock_exclusive);
	check_find_lock_one($col2, '5', 5, $Lock_exclusive);
	check_find_lock_one($col1, '6', 6, $Lock_exclusive);

	$session1->rollback();

	check_find_lock_one($col2, '1', 1, $Lock_exclusive);
	check_find_lock_one($col2, '4', 4, $Lock_exclusive);

	$session2->rollback();


	// test5: Shared Lock read after Exclusive write
	$session1->startTransaction();

	check_find_lock_all($col1, ['1', '2'], [1, 2], $Lock_exclusive);
	modify_row($col1, '1', 11);
	modify_row($col1, '2', 22);
	check_find_lock_all($col1, ['1', '2'], [11, 22], $Lock_exclusive);

	$session2->startTransaction();

	check_find_lock_all($col2, ['3', '4'], [3, 4], $Lock_exclusive);
	check_find_lock_all($col1, ['5', '6'], [5, 6], $Lock_exclusive);

	$session1->commit();

	check_find_lock_all($col2, ['5', '6'], [5, 6], $Lock_exclusive);

	check_find_lock_one($col2, '2', 22, $Lock_shared);
	check_find_lock_one($col2, '1', 11, $Lock_shared);

	$session2->commit();


	// test6: Exclusive write after Shared read
	$session1->startTransaction();
	check_find_lock_all($col1, ['1', '3'], [11, 3], $Lock_shared);

	$session2->startTransaction();

	check_find_lock_one($col2, '2', 22, $Lock_exclusive);
	modify_row($col2, '2', 222);
	check_find_lock_one($col2, '2', 222, $Lock_exclusive);

	check_find_lock_one($col2, '3', 3, $Lock_shared);

	check_find_lock_all($col1, ['5', '6'], [5, 6], $Lock_shared);

	$session1->commit();

	check_find_lock_all($col2, ['5', '6'], [5, 6], $Lock_exclusive);

	check_find_lock_one($col2, '1', 11, $Lock_exclusive);
	modify_row($col2, '1', 111);
	check_find_lock_one($col2, '1', 111, $Lock_exclusive);

	$session2->commit();

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
