--TEST--
mysqlx add with empty [] is noop
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
		require_once("connect.inc");

        $session = mysql_xdevapi\getSession($connection_uri);
		$session->createSchema($db);
		$schema = $session->getSchema($db);

		$schema->createCollection("test_collection");
		$coll = $schema->getCollection("test_collection");

		$result = $coll->add([])->execute();//Single noop

		expect_null($result);

		$res = $coll->find()->execute()->fetchAll();
		expect_empty_array($res);

		$result = $coll->add(
				[],
				["_id" => "1", "name" => "Sakila", "age" => 18, "job" => "Student"],
				[],
				["_id" => "2", "name" => "Mike", "age" => 39, "job" => "Manager"]
				)->execute(); //Mixed noop with valid docs
		expect_eq($result->getAffectedItemsCount(), 2);

		$result = $coll->add([])->execute();//Again noop
		expect_null($result);

		$res = $coll->find()->execute()->fetchAll();
		expect_eq(count($res),2);

		$result = $coll->add([],[],[])->execute(); //Perverted case...
		expect_null($result);
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
