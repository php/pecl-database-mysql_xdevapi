--TEST--
mysqlx table row locking misc
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
	$res1 = $tab1->select('_id', 'n')->where("_id like '1'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res1, '1', 1);

	$res1 = $tab1->select('_id', 'n')->where("_id like '3'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res1, '3', 3);

	$session2->startTransaction();

	$res2 = $tab2->select('_id', 'n')->where("_id like '3'")
		->lockExclusive()->lockShared()->execute();
	check_one($res2, '3', 3);

	$res2 = $tab2->select('_id', 'n')->where("_id like '2'")
		->lockShared()->lockExclusive()->lockShared()->execute();
	check_one($res2, '2', 2);

	$res2 = $tab2->select('_id', 'n')->where("_id like '1'")
		->lockExclusive()->lockShared()->execute();
	check_one($res2, '1', 1);

	$res2 = $tab2->select('_id', 'n')->where("_id like '4'")
		->lockExclusive()->lockShared()->execute();
	check_one($res2, '4', 4);

	$res1 = $tab1->select('_id', 'n')->where("_id like '4'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res1, '4', 4);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive
	$session1->startTransaction();
	$res1 = $tab1->select('_id', 'n')->where("_id like '1'")
		->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	$res2 = $tab2->select('_id', 'n')->where("_id like '2'")
		->lockExclusive()->lockShared()->lockShared()->lockShared()->execute();
	check_one($res2, '2', 2);

	$res2 = $tab2->select('_id', 'n')->where("_id like '3'")
		->lockShared()->lockExclusive()->lockShared()->execute();
	check_one($res2, '3', 3);

	$res1 = $tab1->select('_id', 'n')->where("_id like '4'")
		->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res1, '4', 4);

	$session1->rollback();

	$res2 = $tab2->select('_id', 'n')->where("_id like '1'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res2, '1', 1);

	$session2->rollback();


	// test3: Exclusive after Shared
	$session1->startTransaction();

	$res1 = $tab1->select('_id', 'n')->where("_id like '1'")
		->lockExclusive()->lockExclusive()->lockShared()->execute();
	check_one($res1, '1', 1);

	$res1 = $tab1->select('_id', 'n')->where("_id like '3'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res1, '3', 3);

	$session2->startTransaction();

	$res2 = $tab2->select('_id', 'n')->where("_id like '2'")
		->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res2, '2', 2);

	$res2 = $tab2->select('_id', 'n')->where("_id like '3'")
		->lockExclusive()->lockExclusive()->lockShared()->execute();
	check_one($res2, '3', 3);

	$res2 = $tab2->select('_id', 'n')->where("_id like '4'")
		->lockShared()->lockExclusive()->lockExclusive()->execute();
	check_one($res2, '4', 4);

	$session1->rollback();

	$res2 = $tab2->select('_id', 'n')->where("_id like '1'")
		->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session2->rollback();


	// test4: Exclusive after Exclusive
	$session1->startTransaction();
	$res1 = $tab1->select('_id', 'n')->where("_id like '1'")
		->lockShared()->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	$res2 = $tab2->select('_id', 'n')->where("_id like '2'")
		->lockExclusive()->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res2, '2', 2);

	$res2 = $tab2->select('_id', 'n')->where("_id like '3'")
		->lockShared()->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res2, '3', 3);

	$res1 = $tab1->select('_id', 'n')->where("_id like '4'")
		->lockShared()->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res1, '4', 4);

	$session1->rollback();

	$res2 = $tab2->select('_id', 'n')->where("_id like '1'")
		->lockShared()->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session2->rollback();


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
