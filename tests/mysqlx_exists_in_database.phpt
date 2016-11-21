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

	var_dump($schema->existsInDatabase());
	var_dump($table->existsInDatabase());
	var_dump($collection->existsInDatabase());

	$table = $schema->getTable("non_found_table");
	$collection = $schema->getCollection("non_found_collection");

	var_dump($table->existsInDatabase());
	var_dump($collection->existsInDatabase());

	$schema = $nodeSession->getSchema("non_existing_schema");
	$table = $schema->getTable("non_existing_table");
	$collection = $schema->getCollection("non_existing_collection");

	var_dump($schema->existsInDatabase());
	var_dump($table->existsInDatabase());
	var_dump($collection->existsInDatabase());

	print "done!
";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
done!%A
