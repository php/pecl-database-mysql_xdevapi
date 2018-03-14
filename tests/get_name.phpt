--TEST--
mysqlx getName for schema, collection, table and view
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$table = $schema->getTable($test_table_name);
	$collection = $schema->getCollection($test_collection_name);
	$view = create_test_view($session);

	var_dump($schema->getName());
	var_dump($table->getName());
	var_dump($collection->getName());
	var_dump($view->getName());

	$table = $schema->getTable("non_found_table");
	$collection = $schema->getCollection("non_found_collection");

	var_dump($table->getName());
	var_dump($collection->getName());

	$schema = $session->getSchema("non_existing_schema");
	$table = $schema->getTable("non_existing_table");
	$collection = $schema->getCollection("non_existing_collection");

	var_dump($schema->getName());
	var_dump($table->getName());
	var_dump($collection->getName());
	var_dump($view->getName());

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
string(%d) "%s"
string(10) "test_table"
string(15) "test_collection"
string(9) "test_view"
string(15) "non_found_table"
string(20) "non_found_collection"
string(19) "non_existing_schema"
string(18) "non_existing_table"
string(23) "non_existing_collection"
string(9) "test_view"
done!%A
