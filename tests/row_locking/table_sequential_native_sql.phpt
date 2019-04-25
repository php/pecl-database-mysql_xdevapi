--TEST--
mysqlx table row locking sequential with native SQL queries
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

	// shared after shared / native SQL queries
	$session1->startTransaction();
	$session2->startTransaction();

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '1' lock in share mode")->execute();
	check_one($res1, '1', 1);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '2' lock in share mode")->execute();
	check_one($res2, '2', 2);

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '3' lock in share mode")->execute();
	check_one($res1, '3', 3);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '4' lock in share mode")->execute();
	check_one($res2, '4', 4);

	$session1->rollback();
	$session2->rollback();


	// shared after exclusive / native SQL queries
	$session1->startTransaction();
	$session2->startTransaction();

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '1' for update")->execute();
	check_one($res1, '1', 1);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '3' lock in share mode")->execute();
	check_one($res2, '3', 3);

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '2' for update")->execute();
	check_one($res1, '2', 2);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '4' lock in share mode")->execute();
	check_one($res2, '4', 4);

	$session1->rollback();
	$session2->rollback();


	// exclusive after shared / native SQL queries
	$session1->startTransaction();
	$session2->startTransaction();

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '2' lock in share mode")->execute();
	check_one($res1, '2', 2);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '3' for update")->execute();
	check_one($res2, '3', 3);

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '5' lock in share mode")->execute();
	check_one($res1, '5', 5);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '6' for update")->execute();
	check_one($res2, '6', 6);

	$session1->rollback();
	$session2->rollback();


	// exclusive after exclusive / native SQL queries
	$session1->startTransaction();
	$session2->startTransaction();

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '1' for update")->execute();
	check_one($res1, '1', 1);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '2' for update")->execute();
	check_one($res2, '2', 2);

	$res1 = $session1->sql("select * from $db.$test_table_name where _id like '5' for update")->execute();
	check_one($res1, '5', 5);

	$res2 = $session2->sql("select * from $db.$test_table_name where _id like '6' for update")->execute();
	check_one($res2, '6', 6);

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
