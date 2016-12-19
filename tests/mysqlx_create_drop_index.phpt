--TEST--
mysqlx create/drop index
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function assert_index($indexName) {
		global $db;
		global $nodeSession;

		$query = "SHOW INDEX FROM test_collection FROM $db WHERE Key_name='$indexName'";
		$res = $nodeSession->executeSql($query);
		expect_true($res->hasData());

		global $coll;
		$coll->dropIndex($indexName)->execute();

		$res = $nodeSession->executeSql($query);
		expect_false($res->hasData());
	}

	$nodeSession = create_test_db();
	fill_test_collection();

	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	$coll->createIndex("name_index", false)->field("$.name", "TEXT(15)", true)->execute();
	assert_index("name_index");

	$coll->createIndex("age_index", false)->field("$.age", "INTEGER", false)->execute();
	assert_index("age_index");

	$coll->createIndex("name_age_index", true)->field("$.name", "TEXT(20)", true)->field("$.age", "INTEGER", false)->execute();
	assert_index("name_age_index");

	$coll->createIndex("age_job_index", false)->field("$.age", "INTEGER", true)->field("$.job", "TEXT(30)", false)->execute();
	assert_index("age_job_index");

	$coll->createIndex("name_age_job_index", true)->field("$.name", "TEXT(20)", true)->field("$.age", "INTEGER", true)->field("$.job", "TEXT(30)", false)->execute();
	assert_index("name_age_job_index");

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
