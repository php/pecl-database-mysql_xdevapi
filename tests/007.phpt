--TEST--
mysqlx basic collection operations
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$nodeSession = mysql_xdevapi\getSession($connection_uri);

	function verify_doc($doc, $name, $job, $age) {
		$result = ($doc[0] = $name);
		$result = ($result && ($doc[1] = $job));
		$result = ($result && ($doc[2] = $age));
		return $result;
	}

	$nodeSession->createSchema($test_schema_name);
	$schema = $nodeSession->getSchema($test_schema_name);

	$schema->createCollection($test_collection_name);
	$coll = $schema->getCollection($test_collection_name);

	$coll->add('{"name": "Sakila", "age": 15, "job": "Programmer"}')->execute();

	$sakila = ["name" => "Sakila", "age" => 17, "job" => "Singer"];
	$coll->add($sakila)->execute();
	$coll->add(["name" => "Sakila", "age" => 18, "job" => "Student"])->execute();
	$coll->add('{"name": "Susanne", "age": 24, "job": "Plumber"}')->execute();
	$coll->add(["name" => "Mike", "age" => 39, "job" => "Manager"])->execute();

	$res = $coll->find('name like "Sakila"')->execute();
	$data = $res->fetchAll();

	$test = "0000";
	for ($i = 0; $i < count($data); $i++) {
		if (verify_doc($data[$i]["doc"], "Sakila", "Programmer", "15"))
			$test[0] = "1";
		if (verify_doc($data[$i]["doc"], "Sakila", "Singer", "17"))
			$test[1] = "1";
		if (verify_doc($data[$i]["doc"], "Sakila", "Student", "18"))
			$test[2] = "1";
	}

	$coll->remove('name like "Sakila"')->execute();
	$res = $coll->find('name like "Sakila"')->execute();
	$data = $res->fetchAll();
	if (is_bool($data) && $data == false)
		$test[3] = "1";

	$schema->dropCollection($test_collection_name);
	$nodeSession->dropSchema($test_schema_name);

	var_dump($test);
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db($test_schema_name);
?>
--EXPECTF--
%s(4) "1111"
done!%A
