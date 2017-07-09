--TEST--
mysqlx collection row locking misc
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once(__DIR__."/../connect.inc");
	require_once(__DIR__."/mysqlx_row_locking.inc");

	$session1 = mysql_xdevapi\getSession($connection_uri);
	$session2 = mysql_xdevapi\getSession($connection_uri);

	$col1 = createTestCollection($session1);

	$schema2 = $session2->getSchema($test_schema_name);
	$col2 = $schema2->getCollection($test_collection_name);


	// test1: Shared Lock
	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")
		->lockShared()->lockExclusive()->lockShared()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	$res2 = $col2->find("id like '2'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res2, '2', 2);

	$res2 = $col2->find("id like '1'")
		->lockExclusive()->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive
	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")
		->lockShared()->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	$res2 = $col2->find("id like '2'")
		->lockShared()->lockShared()->lockShared()->execute();
	check_one($res2, '2', 2);

	$res2 = $col2->find("id like '1'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
	$session2->rollback();


	// test3: Exclusive after Shared
	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")
		->lockExclusive()->lockShared()->lockShared()->execute();
	check_one($res1, '1', 1);
	$res1 = $col1->find("id like '3'")
		->lockShared()->lockExclusive()->lockShared()->execute();
	check_one($res1, '3', 3);

	$session2->startTransaction();

	$res2 = $col2->find("id like '2'")
		->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res2, '2', 2);

	$res2 = $col2->find("id like '3'")
		->lockShared()->lockExclusive()->lockShared()->execute();
	check_one($res2, '3', 3);

	$res2 = $col2->find("id like '1'")
		->lockExclusive()->lockShared()->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
	$session2->rollback();


	// test4: Exclusive after Exclusive
	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")
		->lockShared()->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	$res2 = $col2->find("id like '2'")
		->lockShared()->lockShared()->lockExclusive()->lockExclusive()->execute();
	check_one($res2, '2', 2);

	$res2 = $col2->find("id like '1'")
		->lockExclusive()->lockShared()->lockShared()->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
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
