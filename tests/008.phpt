--TEST--
mysqlx collection modify/set/unset
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$test = "000000000";

	$nodeSession = mysql_xdevapi\getNodeSession($connection_uri);

	function verify_doc($doc, $name, $job, $age) {
		$result = ($doc[0] = $name);
		$result = ($result && ($doc[1] = $job));
		$result = ($result && ($doc[2] = $age));
		return $result;
	}

	$nodeSession->createSchema("test_schema");
	$schema = $nodeSession->getSchema("test_schema");

	$schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");

	$coll->add('{"name": "Sakila", "age": 15, "job": "Programmer"}')->execute();
	$coll->add('{"name": "Sakila", "age": 17, "job": "Singer"}')->execute();
	$coll->add('{"name": "Sakila", "age": 18, "job": "Student"}')->execute();
	$coll->add('{"name": "Susanne", "age": 24, "job": "Plumber"}')->execute();
	$coll->add('{"name": "Mike", "age": 39, "job": "Manager"}')->execute();

	$coll->modify('name like :param')->set("job", "Unemployed")->bind(['param' => 'Sakila'])->execute();

	$res = $coll->find('name like "Sakila"')->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		if (verify_doc($data[$i]['doc'], 'Sakila', 'Unemployed', '15'))
			$test[0] ='1';
		if (verify_doc($data[$i]["doc"], 'Sakila', 'Unemployed', '17'))
			$test[1] ='1';
		if (verify_doc($data[$i]['doc'], 'Sakila', 'Unemployed', '18'))
			$test[2] ='1';
	}


	$coll->modify('job like :job_name')->unset(["age", "name"])->bind(['job_name' => 'Plumber'])->execute();
	$coll->modify('job like :job_name')->set("name", "Artur")->set("age",49)->bind(['job_name' => 'Plumber'])->execute();

	$res = $coll->find('job like "Plumber"')->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		if (verify_doc($data[$i]["doc"], "Artur", "Plumber", "49"))
			$test[3] = "1";
	}

	$coll->modify('job like :job_param')->set("name", "Martin")->bind(['job_param' => 'nojob'])->execute();
	$coll->modify('name like :name_param')->unset(["crap1", "crap2"])->bind(['name_param' => 'Sakila'])->execute();

	$res = $coll->find()->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		if (verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "15"))
			$test[4] = "1";
		if (verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "17"))
			$test[5] = "1";
		if (verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "18"))
			$test[6] = "1";
		if (verify_doc($data[$i]["doc"], "Artur", "Plumber", "49"))
			$test[7] = "1";
		if (verify_doc($data[$i]["doc"], "Mike", "Manager", "39"))
			$test[8] = "1";
	}

	$nodeSession->dropSchema("test_schema");

	var_dump($test);
		print "done!\n";
?>
--EXPECTF--
%s(9) "111111111"
done!%A
