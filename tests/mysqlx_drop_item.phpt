--TEST--
mysqlx drop schema, table, collection, collection index, view
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	fill_db_table();
	fill_test_collection();
	$view = create_test_view($nodeSession);

	$schema = $nodeSession->getSchema($db);
	$collection = $schema->getCollection($test_collection_name);

	expect_true($schema->dropView($test_view_name));
	expect_false($schema->dropView($test_view_name));

	expect_true($schema->dropTable($test_table_name));
	expect_false($schema->dropTable($test_table_name));

	$indexName = "name_index";
	$collection->createIndex($indexName, false)->field("$.name", "TEXT(15)", true)->execute();
	expect_true($collection->dropIndex($indexName));
	expect_false($collection->dropIndex($indexName));

	expect_true($schema->dropCollection($test_collection_name));
	expect_false($schema->dropCollection($test_collection_name));

	expect_true($nodeSession->dropSchema($test_schema_name));
	expect_false($nodeSession->dropSchema($test_schema_name));

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
Warning: mysql_xdevapi\NodeSchema::dropView(): cannot drop view 'test_view' in%A

Warning: mysql_xdevapi\NodeSchema::dropTable(): [HY000] Unknown table '%s.test_table' in%A

Warning: mysql_xdevapi\NodeCollection::dropIndex(): [HY000] Can't DROP 'name_index'; check that column/key exists in%A

Warning: mysql_xdevapi\NodeSchema::dropCollection(): [HY000] Unknown table '%s.test_collection' in%A

Warning: mysql_xdevapi\BaseSession::dropSchema(): cannot drop schema '%s' in%A
done!%A
