--TEST--
existsInDatabase for schema, collection and table
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");
	$collection = $schema->getCollection("test_collection");

        expect_true($schema->existsInDatabase());
	expect_true($table->existsInDatabase());
	expect_true($collection->existsInDatabase());

	$table = $schema->getTable("non_found_table");
	$collection = $schema->getCollection("non_found_collection");

        expect_false($table->existsInDatabase());
	expect_false($collection->existsInDatabase());

	$schema = $nodeSession->getSchema("non_existing_schema");
	$table = $schema->getTable("non_existing_table");
	$collection = $schema->getCollection("non_existing_collection");

        expect_false($schema->existsInDatabase());
	expect_false($table->existsInDatabase());
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
