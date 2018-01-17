--TEST--
mysqlx table row locking sequential
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/row_locking_utils.inc");

	$session1 = mysql_xdevapi\getSession($connection_uri);
	$session2 = mysql_xdevapi\getSession($connection_uri);

	$tab1 = create_test_table($session1);

	$schema2 = $session2->getSchema($test_schema_name);
	$tab2 = $schema2->getTable($test_table_name);

	// test1: Shared Lock
	$session1->startTransaction();
	check_select_lock_one($tab1, '1', 1, $Lock_shared);
	check_select_lock_one($tab1, '3', 3, $Lock_shared);

	$session2->startTransaction();

	check_select_lock_one($tab2, '2', 2, $Lock_shared);

	check_select_lock_one($tab1, '4', 4, $Lock_shared);

	check_select_lock_one($tab2, '1', 1, $Lock_shared);
	check_select_lock_one($tab2, '4', 4, $Lock_shared);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive
	$session1->startTransaction();
	check_select_lock_one($tab1, '1', 1, $Lock_exclusive);
	check_select_lock_one($tab1, '3', 3, $Lock_exclusive);

	$session2->startTransaction();

	check_select_lock_one($tab2, '2', 2, $Lock_shared);

	check_select_lock_one($tab1, '5', 5, $Lock_exclusive);

	check_select_lock_one($tab2, '4', 4, $Lock_shared);

	$session1->rollback();

	check_select_lock_one($tab2, '1', 1, $Lock_shared);
	check_select_lock_one($tab2, '5', 5, $Lock_shared);

	$session2->rollback();


	// test3: Exclusive after Shared
	$session1->startTransaction();
	check_select_lock_all($tab1, ['1', '3'], [1, 3], $Lock_shared);

	$session2->startTransaction();
	check_select_lock_all($tab2, ['2', '4'], [2, 4], $Lock_exclusive);
	check_select_lock_all($tab1, ['5', '6'], [5, 6], $Lock_shared);

	$session1->rollback();

	check_select_lock_all($tab2, ['1', '3'], [1, 3], $Lock_exclusive);

	$session2->rollback();


	// test4: Exclusive after Exclusive
	$session1->startTransaction();
	check_select_lock_all($tab1, ['1', '2'], [1, 2], $Lock_exclusive);

	$session2->startTransaction();
	check_select_lock_all($tab2, ['3', '4'], [3, 4], $Lock_exclusive);

	check_select_lock_one($tab1, '5', 5, $Lock_exclusive);
	check_select_lock_one($tab2, '6', 6, $Lock_exclusive);

	$session1->rollback();

	check_select_lock_all($tab2, ['1', '2'], [1, 2], $Lock_exclusive);

	$session2->rollback();


	// test5: Shared Lock read after Exclusive write
	$session1->startTransaction();

	check_select_lock_all($tab1, ['1', '2'], [1, 2], $Lock_exclusive);
	update_row($tab1, '1', 11);
	update_row($tab1, '2', 22);
	check_select_lock_all($tab1, ['1', '2'], [11, 22], $Lock_exclusive);

	$session2->startTransaction();

	check_select_lock_all($tab2, ['3', '4', '5'], [3, 4, 5], $Lock_shared);
	check_select_lock_one($tab1, '6', 6, $Lock_exclusive);

	$session1->commit();

	check_select_lock_all($tab2, ['1', '2', '6'], [11, 22, 6], $Lock_shared);
	$session2->commit();


	// test6: Exclusive write after Shared read
	$session1->startTransaction();
	check_select_lock_all($tab1, ['1', '3'], [11, 3], $Lock_shared);

	$session2->startTransaction();

	check_select_lock_one($tab2, '2', 22, $Lock_exclusive);
	update_row($tab2, '2', 222);
	check_select_lock_one($tab2, '2', 222, $Lock_exclusive);

	check_select_lock_one($tab2, '3', 3, $Lock_shared);

	$session1->commit();

	check_select_lock_one($tab2, '1', 11, $Lock_exclusive);
	update_row($tab2, '1', 111);
	check_select_lock_one($tab2, '1', 111, $Lock_exclusive);

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
