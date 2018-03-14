--TEST--
mysqlx coll. multiple add / Affected item counts
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

	$res = $coll->add('{"name": "Marco","age": 19, "job": "Programmatore"}',
			'{"name": "Marco2","age": 192, "job": "Programmatore"}',
			["name" => "Mario", "age" => 44, "job" => "Tifoso"],
			'{"name": "Marco3","age": 92, "job": "Programmatore2"}')->execute();
	expect_eq($res->getAffectedItemsCount(), 4);

	$res = $coll->add(
		'{"name": "Lonardo","_id": "  2  "  , "age": 59, "job": "Paninaro"}',
		'{"name": "Riccardo",   "age": 27, "job": "Cantante","_id":34}',
		'{"name": "Carlotta",   "age": 23, "_id":     33.33    , "job": "Programmatrice"}'
		)->execute();
	expect_eq($res->getAffectedItemsCount(), 3);

	$res = $coll->add(
		["name" => "Carlotta", "age" => 34, "_id" => 11, "job" => "Dentista"],
		["name" => "Antonello", "age" => 45, "job" => "Scavatore" , "_id" => 344],
		["name" => "Mariangela", "age" => 32, "job" => "Disoccupata"],
		["name" => "Antonio", "age" => 42, "job" => "Architetto"]
		)->execute();
	expect_eq($res->getAffectedItemsCount(), 4);


	$res = $coll->find()->sort('name desc')->execute()->fetchAll();
	expect_eq(count($res), 11);
	expect_eq($res[0]["name"], "Riccardo");
	expect_eq($res[0]["age"], 27);
	expect_eq($res[0]["job"], "Cantante");
	expect_eq($res[0]["_id"], 34);

	expect_eq($res[1]["name"], "Mario");
	expect_eq($res[1]["age"], 44);
	expect_eq($res[1]["job"], "Tifoso");

	expect_eq($res[9]["name"], "Antonio");
	expect_eq($res[9]["age"], 42);
	expect_eq($res[9]["job"], "Architetto");

	expect_eq($res[10]["name"], "Antonello");
	expect_eq($res[10]["age"], 45);
	expect_eq($res[10]["job"], "Scavatore");
	expect_eq($res[10]["_id"], 344);


	$res = $coll->add(["name" => "Marco", "age" => 84, "job" => "Spavatore", "_id" => 123])->execute();
	expect_eq($res->getAffectedItemsCount(), 1);

	$res = $coll->add('{"name": "Roberto","age": 39, "job": "Animatore", "_id" : "cool"}')->execute();
	expect_eq($res->getAffectedItemsCount(), 1);

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
