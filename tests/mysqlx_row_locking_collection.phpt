--TEST--
mysqlx collection row locking
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

	$col1 = $schema1->createCollection($test_collection_name);

	$col1->add('{_id:"1", "a": 1}');
	$col1->add('{_id:"2", "a": 1}');
	$col1->add('{_id:"3", "a": 1}');

	$schema2 = $session2->getSchema($test_schema_name);
	$col2 = $schema2->getCollection($test_collection_name);

	// test1: Shared Lock

	$session1->startTransaction();
	$col1->find("_id = '1'")->lockShared()->execute();

	$session2->startTransaction();
	$col2->find("_id = '2'")->lockShared()->execute(); // should return immediately
	$col2->find("_id = '1'")->lockShared()->execute(); // should return immediately

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive

	$session1->startTransaction();
	$col1->find("_id = '1'")->lockExclusive()->execute();

	$session2->startTransaction();
	$col2->find("_id = '2'")->lockShared()->execute(); // should return immediately
	$col2->find("_id = '1'")->lockShared()->execute(); // $session2 blocks

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test3: Exclusive after Shared

	$session1->startTransaction();
	$col1->find("_id in ('1', '3')")->lockShared()->execute();

	$session2->startTransaction();
	$col2->find("_id = '2'")->lockExclusive()->execute();  // should return immediately
	$col2->find("_id = '3'")->lockShared()->execute();     // should return immediately
	$col2->find("_id = '1'")->lockExclusive()->execute();  // $session2 should block

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test4: Exclusive after Exclusive

	$session1->startTransaction();
	$col1->find("_id = '1'")->lockExclusive()->execute();

	$session2->startTransaction();
	$col2->find("_id = '2'")->lockExclusive()->execute(); // should return immediately
	$col2->find("_id = '1'")->lockExclusive()->execute(); // $session2 should block

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
