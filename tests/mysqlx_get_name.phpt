--TEST--
mysqlx getName for schema, collection and table
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");
	$collection = $schema->getCollection("test_collection");

	var_dump($schema->getName());
	var_dump($table->getName());
	var_dump($collection->getName());

	$table = $schema->getTable("non_found_table");
	$collection = $schema->getCollection("non_found_collection");

	var_dump($table->getName());
	var_dump($collection->getName());

	$schema = $nodeSession->getSchema("non_existing_schema");
	$table = $schema->getTable("non_existing_table");
	$collection = $schema->getCollection("non_existing_collection");

	var_dump($schema->getName());
	var_dump($table->getName());
	var_dump($collection->getName());

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
string(5) "testx"
string(10) "test_table"
string(15) "test_collection"
string(15) "non_found_table"
string(20) "non_found_collection"
string(19) "non_existing_schema"
string(18) "non_existing_table"
string(23) "non_existing_collection"
done!%A