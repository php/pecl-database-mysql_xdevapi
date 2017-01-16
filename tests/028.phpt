--TEST--
mysqlx add with empty [] is noop
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
		require_once("connect.inc");

		$nodeSession = mysql_xdevapi\getNodeSession($connection_uri);

		$nodeSession->createSchema($db);
		$schema = $nodeSession->getSchema($db);

		$schema->createCollection("test_collection");
		$coll = $schema->getCollection("test_collection");

		$node_result = $coll->add([])->execute();//Single noop
		expect_false($node_result);

		$res = $coll->find()->execute()->fetchAll();
		expect_false($res);

		$node_result = $coll->add(
				[],
				["name" => "Sakila", "age" => 18, "job" => "Student"],
				[],
				["name" => "Mike", "age" => 39, "job" => "Manager"]
				)->execute(); //Mixed noop with valid docs
		expect_eq($node_result->getAffectedItemsCount(), 2);

		$node_result = $coll->add([])->execute();//Again noop
		expect_false($node_result);

		$res = $coll->find()->execute()->fetchAll();
		expect_eq(count($res),2);

		$node_result = $coll->add([],[],[])->execute(); //Perverted case...
		expect_false($node_result);
		$res = $coll->find()->execute()->fetchAll();
		expect_eq(count($res),2);

		expect_eq($res[0]['name'],'Sakila');
		expect_eq($res[0]['age'],18);
		expect_eq($res[0]['job'],'Student');

		expect_eq($res[1]['name'],'Mike');
		expect_eq($res[1]['age'],39);
		expect_eq($res[1]['job'],'Manager');

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
