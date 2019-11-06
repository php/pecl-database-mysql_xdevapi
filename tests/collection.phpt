--TEST--
mysqlx Collection
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection($test_collection_name);

	fill_db_collection($coll);

	expect_eq($coll->getName(),$test_collection_name);
	expect_eq($coll->name, $test_collection_name);
	expect_true($coll->existsInDatabase());
	expect_eq($coll->count(), 16);

	try {
		$schema = $coll->getSchema();
		expect_eq($schema->getName(),$db);
		expect_true($schema->existsInDatabase());
		$coll2 = $schema->getCollection($test_collection_name);
		expect_eq($coll2->getName(),$test_collection_name);
		expect_true($coll2->existsInDatabase());
	} catch(Exception $ex) {
		test_step_failed();
	}

	try {
		$session2 = $coll->getSession();
	} catch(Exception $ex) {
		test_step_failed();
	}

	$coll = $schema->getCollection("not_existing_collection");
	expect_eq($coll->getName(), 'not_existing_collection');
	expect_eq($coll->name, 'not_existing_collection');
	expect_false($coll->existsInDatabase());

	try {
		$schema = $coll->count();
		test_step_failed();
	} catch(Exception $ex) {
		test_step_ok();
	}

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
