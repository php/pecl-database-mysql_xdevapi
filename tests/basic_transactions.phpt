--TEST--
mysqlx basic transactions
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	$session->startTransaction();

	try{
		fill_db_collection($coll);
		$session->commit();
	} catch( Exception $e ) {
		test_step_failed();
	}

	$res = $coll->find()->execute();
	expect_eq( count($res->fetchAll()), 16);

	$session->startTransaction();

	try{
		$coll->add('{"_id":17, "name": "Massimo",    "age": 32, "job": "Cavia"}')->execute();
		$coll->add('{"_id":18, "name": "Carlo",      "age": 47, "job": "Cavia"}')->execute();
		$coll->add('{"_id":17, "name": "Leonardo",   "age": 53, "job": "Cavia"}')->execute();
		$session->commit();
		test_step_failed(); //commit shall raise an exception!
	} catch( Exception $e) {
		expect_eq($e->getMessage(),
			"[HY000] Couldn't fetch data");
		expect_eq($e->getCode(), 10000);
		$session->rollback();
	}

	$res = $coll->find()->execute();
	expect_eq( count($res->fetchAll()), 16);

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
