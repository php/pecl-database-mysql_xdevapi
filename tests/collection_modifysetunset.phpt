--TEST--
mysqlx collection modify/set/unset
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$session = mysql_xdevapi\getSession($connection_uri);

	function verify_doc($doc, $name, $job, $age) {
		$result = ($doc[0] = $name);
		$result = ($result && ($doc[1] = $job));
		$result = ($result && ($doc[2] = $age));
		return $result;
	}

	$session->createSchema($test_schema_name);
	$schema = $session->getSchema($test_schema_name);

	$schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");

	$coll->add('{"name": "Sakila", "age": 15, "job": "Programmer"}')->execute();
	$coll->add('{"name": "Sakila", "age": 17, "job": "Singer"}')->execute();
	$coll->add('{"name": "Sakila", "age": 18, "job": "Student"}')->execute();
	$coll->add('{"name": "Susanne", "age": 24, "job": "Plumber"}')->execute();
	$coll->add('{"name": "Mike", "age": 39, "job": "Manager"}')->execute();

	$coll->modify('name like :param')
		->set("job", "Unemployed")
		->bind(['param' => 'Sakila'])
		->execute();

	$res = $coll->find('name like "Sakila"')->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		expect_true(verify_doc($data[$i]['doc'], 'Sakila', 'Unemployed', '15'));
		expect_true(verify_doc($data[$i]["doc"], 'Sakila', 'Unemployed', '17'));
		expect_true(verify_doc($data[$i]['doc'], 'Sakila', 'Unemployed', '18'));
	}


	$coll->modify('job like :job_name')->unset(["age", "name"])->bind(['job_name' => 'Plumber'])->execute();
	$coll->modify('job like :job_name')
		->set("name", "Artur")
		->set("age", "49")
		->bind(['job_name' => 'Plumber'])->execute();

	$res = $coll->find('job like "Plumber"')->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		expect_true(verify_doc($data[$i]["doc"], "Artur", "Plumber", "49"));
	}

	$coll->modify('job like :job_param')->set("name", "Martin")->bind(['job_param' => 'nojob'])->execute();
	$coll->modify('name like :name_param')->unset(["crap1", "crap2"])->bind(['name_param' => 'Sakila'])->execute();

	try{
		$coll->modify()->set("name","test")->execute();
		test_step_failed();
	} catch( Exception $ex ) {
		test_step_ok();
	}

	$res = $coll->find()->execute();
	$data = $res->fetchAll();

	for ($i = 0; $i < count($data); $i++) {
		expect_true(verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "15"));
		expect_true(verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "17"));
		expect_true(verify_doc($data[$i]["doc"], "Sakila", "Unemployed", "18"));
		expect_true(verify_doc($data[$i]["doc"], "Artur", "Plumber", "49"));
		expect_true(verify_doc($data[$i]["doc"], "Mike", "Manager", "39"));
	}

	// fails expected due to empty or incorrect search-condition
	function check_incorrect_condition($condition) {
		global $coll;
		try {
			$coll->modify($condition);
			test_step_failed();
		} catch(Exception $e) {
			test_step_ok();
		}
	}

	check_incorrect_condition('');
	check_incorrect_condition(' ');
	check_incorrect_condition('@ incorrect $ condition &');

	$session->dropSchema($test_schema_name);

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db($test_schema_name);
?>
--EXPECTF--
done!%A
