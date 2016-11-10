--TEST--
mysqlx collection find
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function dump_all_row($table){
		$res = $table->select(['age', 'name'])->execute();
		$all_row = $res->fetchAll();
		var_dump($all_row);
	}

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	//TODO, fields do not work for non-array types. Is this ok?
	$res = $coll->find('job like :job and age > :age')->fields(['age']);
	$res = $res->bind(['job' => 'Programmatore', 'age' => 20])->sort('age desc')->limit(2);
	$data = $res->execute();

	var_dump($data->fetchAll());

	$data = $coll->find('job like \'Programmatore\'')->limit(1)->skip(3)->sort('age asc')->execute();
	var_dump($data->fetchAll());

	try { //Expected to fail
		$data = $coll->find('job like \'Programmatore\'')->limit(1)->skip(-1)->sort('age asc')->execute();
	} catch(Exception $ex) {
		print "Exception!\n";
	}

	//For the purpose of testing groupBy(...) I need some special elements in the collection
	//TODO: How does this groupBy Work??
	$coll->add('{"_id":50, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":51, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":52, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":53, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();
	$coll->add('{"_id":54, "name": "Ugo", "age": 10, "job": "Studioso"}')->execute();

	$res = $coll->find('job like :job and age = :age')->fields(['age', 'job'])->groupBy(['age', 'job']);
	$res = $res->bind(['job' => 'Studioso', 'age' => 10])->sort('age desc')->limit(4);
	$data = $res->execute();

	var_dump($data->fetchAll());

	//For the purpose of testing sort([...]) I need some special elements in the collection
	$coll->add('{"_id":99, "name": "Ugp",                 "job": "Cavia"}')->execute();
	$coll->add('{"_id":98, "name": "Simone",              "job": "Cavia"}')->execute();
	$coll->add('{"_id":97, "name": "Matteo",              "job": "Cavia"}')->execute();
	$coll->add('{"_id":96, "name": "Alfonso",  "age": 35, "job": "Cavia"}')->execute();
	$coll->add('{"_id":17, "name": "Luca",     "age": 99, "job": "Cavia"}')->execute();

	$data = $coll->find('job like \'Cavia\'')->sort(['age desc', '_id desc'])->execute();
	var_dump($data->fetchAll());

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
array(2) {
  [0]=>
  array(1) {
    ["age"]=>
    int(27)
  }
  [1]=>
  array(1) {
    ["age"]=>
    int(25)
  }
}
array(1) {
  [0]=>
  array(4) {
    ["_id"]=>
    int(5)
    ["age"]=>
    int(25)
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(5) "Carlo"
  }
}
Exception!
array(1) {
  [0]=>
  array(2) {
    ["age"]=>
    int(10)
    ["job"]=>
    string(8) "Studioso"
  }
}
array(5) {
  [0]=>
  array(4) {
    ["_id"]=>
    int(17)
    ["age"]=>
    int(99)
    ["job"]=>
    string(5) "Cavia"
    ["name"]=>
    string(4) "Luca"
  }
  [1]=>
  array(4) {
    ["_id"]=>
    int(96)
    ["age"]=>
    int(35)
    ["job"]=>
    string(5) "Cavia"
    ["name"]=>
    string(7) "Alfonso"
  }
  [2]=>
  array(3) {
    ["_id"]=>
    int(99)
    ["job"]=>
    string(5) "Cavia"
    ["name"]=>
    string(3) "Ugp"
  }
  [3]=>
  array(3) {
    ["_id"]=>
    int(98)
    ["job"]=>
    string(5) "Cavia"
    ["name"]=>
    string(6) "Simone"
  }
  [4]=>
  array(3) {
    ["_id"]=>
    int(97)
    ["job"]=>
    string(5) "Cavia"
    ["name"]=>
    string(6) "Matteo"
  }
}
done!%A
