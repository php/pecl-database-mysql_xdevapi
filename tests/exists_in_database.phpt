--TEST--
mysqlx existsInDatabase for schema, collection, table and view
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$table = $schema->getTable($test_table_name);
	$collection = $schema->getCollection($test_collection_name);
	$view = create_test_view($session);

	expect_true($schema->existsInDatabase());
	expect_true($table->existsInDatabase());
	expect_true($collection->existsInDatabase());
	expect_true($view->existsInDatabase());
	expect_true($view->isView());

	$table = $schema->getTable("non_found_table");
	$collection = $schema->getCollection("non_found_collection");

	expect_false($table->existsInDatabase());
	expect_false($table->isView());
	expect_false($collection->existsInDatabase());

	$schema = $session->getSchema("non_existing_schema");
	$table = $schema->getTable("non_existing_table");
	$collection = $schema->getCollection("non_existing_collection");

	expect_false($schema->existsInDatabase());
	expect_false($table->existsInDatabase());
	expect_false($table->isView());
	expect_false($collection->existsInDatabase());

	verify_expectations();
	print "done!".PHP_EOL;
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
