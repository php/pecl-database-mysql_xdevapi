--TEST--
mysqlx collection find
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");
	$session = create_test_db();

	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$res = $coll->find('job like :job and age > :age')->fields('age');
	$res = $res->bind(['job' => 'Programmatore', 'age' => 20])->sort('age desc')->limit(2);
	$res = $res->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	$data = $res->fetchAll();

	expect_eq(count($data),2);
	expect_eq($data[0]['age'],27);
	expect_eq($data[1]['age'],25);

	$res = $coll->find("job like 'Programmatore'")->limit(1)->offset(3)->sort('age asc')->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);

	$data = $res->fetchAll();

	expect_eq(count($data),1);
	expect_eq_id($data[0]['_id'],5);
	expect_eq($data[0]['age'],25);
	expect_eq($data[0]['job'],'Programmatore');
	expect_eq($data[0]['name'],'Carlo');

	try { //Expected to fail
		$data = $coll->find("job like 'Programmatore'")->limit(1)->offset(-1)->sort('age asc')->execute();
		test_step_failed();
	} catch(Exception $ex) {
	}

	//For the purpose of testing groupBy(...) I need some special elements in the collection
	$coll->add('{"_id":50, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":51, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":52, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":53, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":54, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();

	$res = $coll->find('job like :job and age = :age')->fields(['age', 'job'])->groupBy('age', 'job');
	$res = $res->bind(['job' => 'Studioso', 'age' => 10])->sort('age desc')->limit(4);
	$res = $res->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	$data = $res->fetchAll();

	expect_eq(count($data),1);
	expect_eq($data[0]['age'],10);
	expect_eq($data[0]['job'],'Studioso');

	//For the purpose of testing sort([...]) I need some special elements in the collection
	$coll->add('{"_id":99, "name": "Ugo",                 "job": "Cavia"}')->execute();
	$coll->add('{"_id":98, "name": "Simone",              "job": "Cavia"}')->execute();
	$coll->add('{"_id":97, "name": "Matteo",              "job": "Cavia"}')->execute();
	$coll->add('{"_id":96, "name": "Alfonso",  "age": 35, "job": "Cavia"}')->execute();
	$coll->add('{"_id":17, "name": "Luca",     "age": 99, "job": "Cavia"}')->execute();

function verify_composed_sort( $data ) {
	$expected = [
		[17,99,'Cavia','Luca'],
		[96,35,'Cavia','Alfonso'],
		[99,null,'Cavia','Ugo'],
		[98,null,'Cavia','Simone'],
		[97,null,'Cavia','Matteo']
	];

	expect_eq(count($data),5);
	if( count($data) == 5 ) {
		for($i = 0 ; $i < 5 ; $i++ ) {
			expect_eq($data[$i]['_id'],$expected[$i][0]);
		if( $expected[$i][1] == null ) {
			expect_false(array_key_exists('age',$data[$i]));
		} else {
			expect_eq($data[$i]['age'],$expected[$i][1]);
		}
		expect_eq($data[$i]['job'],$expected[$i][2]);
		expect_eq($data[$i]['name'],$expected[$i][3]);
	   }
	}
}
	$res = $coll->find("job like 'Cavia'")->sort('age desc', '_id desc')->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	$data = $res->fetchAll();

	verify_composed_sort($data);

	$res = $coll->find("job like 'Cavia'")->sort(['age desc', '_id desc'])->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	$data = $res->fetchAll();

	verify_composed_sort($data);

	$res = $coll->find()
		->fields(['name','age'])
		->limit(3)->sort('age desc')
		->having('age > :ageParam')
		->bind(['ageParam' => 40])
		->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	$data = $res->fetchAll();

	$expected = [
		[99,'Luca'],
		[59,'Lonardo'],
		[47,'Lucia']
	];

	expect_eq(count($data),3);
	if( count($data) == 3 ) {
		for($i = 0 ; $i < 3 ; $i++ ) {
			expect_eq($data[$i]['age'],$expected[$i][0]);
		expect_eq($data[$i]['name'],$expected[$i][1]);
		}
	}

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
