--TEST--
mysqlx warnings
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

    $session = mysql_xdevapi\getSession($connection_uri);
	$session->sql("create database $db")->execute();
	$session->sql("create table $db.test_table(x int)")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert(['x'])->values([1])->values([2])->values([3])->execute();
	$res = $table->select(['x/0 as bad_x'])->execute();

	expect_eq($res->getWarningsCount(), 3);

	$warn = $res->getWarnings();
	for( $i = 0 ; $i < 3; $i++ ) {
	    //expect_eq($warn[0]->message,'');
	    expect_eq($warn[0]->level,2);
	    expect_eq($warn[0]->code,1365);
	}

    $schema->createCollection($test_collection_name);
	$coll = $schema->getCollection($test_collection_name);
	$res = $coll->add('{"name": "Marco",      "age": 19, "job": "Programmatore"}',
	    '{"name": "Lonardo",    "age": 59, "job": "Paninaro"}',
		'{"name": "Riccardo",   "age": 27, "job": "Cantante"}',
		'{"name": "Carlotta",   "age": 23, "job": "Programmatrice"}')->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
