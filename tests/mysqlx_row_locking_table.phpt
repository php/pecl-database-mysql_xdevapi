--TEST--
mysqlx table row locking
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$session1 = mysql_xdevapi\getSession($connection_uri);
	$session2 = mysql_xdevapi\getSession($connection_uri);

	$session1->createSchema($test_schema_name);
	$schema1 = $session1->getSchema($test_schema_name);

	$schema1->createTable($test_table_name)
		->addColumn(new mysql_xdevapi\ColumnDef('_id', 'varchar', 1024))
		->addColumn(new mysql_xdevapi\ColumnDef('a', 'int'))
		->execute();

	$tab1 = $schema1->getTable($test_table_name);
	
	$tab1->insert(["_id", "a"])->values(["1", 1])->execute();
	$tab1->insert(["_id", "a"])->values(["2", 1])->execute();
	$tab1->insert(["_id", "a"])->values(["3", 1])->execute();

	$schema2 = $session2->getSchema($test_schema_name);
	$tab2 = $schema2->getTable($test_table_name);

	// test1: Shared Lock

	$session1->startTransaction();
	$tab1->select('_id', 'a')->where("_id like '1'")->lockShared()->execute();

	$session2->startTransaction();
	$tab2->select('_id', 'a')->where("_id like '2'")->lockShared()->execute(); // should return immediately
	$tab2->select('_id', 'a')->where("_id like '1'")->lockShared()->execute(); // should return immediately

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive

	$session1->startTransaction();
	$tab1->select('_id', 'a')->where("_id like '1'")->lockExclusive()->execute();

	$session2->startTransaction();
	$tab2->select('_id', 'a')->where("_id like '2'")->lockShared()->execute(); // should return immediately
	$tab2->select('_id', 'a')->where("_id like '1'")->lockShared()->execute(); // $session2 blocks

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test3: Exclusive after Shared

	$session1->startTransaction();
	$tab1->select("_id in ('1', '3')")->lockShared()->execute();

	$session2->startTransaction();
	$tab2->select('_id', 'a')->where("_id like '2'")->lockExclusive()->execute();  // should return immediately
	$tab2->select('_id', 'a')->where("_id like '3'")->lockShared()->execute();     // should return immediately
	$tab2->select('_id', 'a')->where("_id like '1'")->lockExclusive()->execute();  // $session2 should block

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test4: Exclusive after Exclusive

	$session1->startTransaction();
	$tab1->select('_id', 'a')->where("_id like '1'")->lockExclusive()->execute();

	$session2->startTransaction();
	$tab2->select('_id', 'a')->where("_id like '2'")->lockExclusive()->execute(); // should return immediately
	$tab2->select('_id', 'a')->where("_id like '1'")->lockExclusive()->execute(); // $session2 should block

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db($test_schema_name);
?>
--EXPECTF--
done!%A
