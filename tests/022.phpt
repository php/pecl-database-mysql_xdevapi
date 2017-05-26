--TEST--
mysqlx NodeSchema
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	expect_true($schema->existsInDatabase());
	$coll_as_table = $schema->getCollectionAsTable('test_collection');

	expect_eq($coll_as_table->getName(), 'test_collection');
	expect_eq($coll_as_table->name, 'test_collection');
	expect_eq($coll_as_table->count(), 16);
/*
	try {
		//This is not implemented yet
		$session = $schema->getSession();
	} catch(Exception $e) {
		test_step_failed();
	}
*/
	$res = $coll_as_table->select(['doc','_id'])->execute()->fetchAll();
	expect_eq(count($res), 16);

	$schema->createCollection('test_collection_2');
	$schema->createCollection('test_collection_3');

	$collections = $schema->getCollections();
	expect_eq(count($collections), 3);
	expect_true($collections['test_collection']->existsInDatabase());
	expect_eq($collections['test_collection']->name, 'test_collection');
	expect_true($collections['test_collection_2']->existsInDatabase());
	expect_eq($collections['test_collection_2']->name, 'test_collection_2');
	expect_true($collections['test_collection_3']->existsInDatabase());
	expect_eq($collections['test_collection_3']->name, 'test_collection_3');

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
Warning: mysql_xdevapi\BaseSession::dropSchema(): cannot drop schema 'testx' in %a
done!%A
