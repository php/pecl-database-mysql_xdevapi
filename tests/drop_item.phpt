--TEST--
mysqlx drop schema, table, collection, collection index, view
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	fill_db_table();
	fill_test_collection();
	$view = create_test_view($session);

	$schema = $session->getSchema($db);
	$collection = $schema->getCollection($test_collection_name);

	$indexName = "name_index";
	$collection->createIndex($indexName, '{"fields": [{"field": "$.name", "type": "TEXT(25)", "required": true}], "unique": false}');
	expect_true($collection->dropIndex($indexName));
	expect_false($collection->dropIndex($indexName));

	expect_true($schema->dropCollection($test_collection_name));
	expect_false($schema->dropCollection($test_collection_name));

	expect_true($session->dropSchema($test_schema_name));
	expect_false($session->dropSchema($test_schema_name));

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
Warning: mysql_xdevapi\Collection::dropIndex(): [1091][42000] Can't DROP 'name_index'; check that column/key exists in%A

Warning: mysql_xdevapi\Schema::dropCollection(): [1051][42S02] Unknown table '%s.test_collection' in%A

Warning: mysql_xdevapi\Session::dropSchema(): cannot drop schema '%s' in%A
done!%A
