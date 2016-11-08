--TEST--
mysqlx modify arrayAppend/Insert
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$coll->modify('name in (\'Marco\', \'Alfredo\', \'Carlo\', \'Leonardo\')')->arrayAppend('job', 'Grafico')->set('name', 'ENTITY')->execute();
	$coll->modify('name like :name')->arrayInsert('job[0]', 'Calciatore')->bind(['name' => 'ENTITY'])->execute();
	$coll->modify('age > :age')->sort('age desc')->unset(['age'])->bind(['age' => 20])->limit(4)->skip(0)->execute();//TODO: only skip(0) allowed?
	$coll->modify('name like \'ENTITY\' and age > :age')->bind(['age' => 23])->unset(['age'])->execute();
	$coll->modify('name in (\'ENTITY\', \'Lucia\')')->sort('_id desc')->limit(2)->set('auto', 'BMW')->execute();

	try {
		$coll->modify()->limit(-1)->unset('name')->execute();
	} catch(Exception $ex) {
		print "Exception!\n";
	}
	$res = $coll->find('name in (\'ENTITY\', \'Lucia\')')->execute();
	$data = $res->fetchAll();
	var_dump($data);

	print "done!
";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
Exception!
array(6) {
  [0]=>
  array(1) {
    ["doc"]=>
    string(90) "{"_id": 1, "age": 19, "job": ["Calciatore", "Programmatore", "Grafico"], "name": "ENTITY"}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(46) "{"_id": 11, "job": "Barista", "name": "Lucia"}"
  }
  [2]=>
  array(1) {
    ["doc"]=>
    string(77) "{"_id": 15, "job": ["Calciatore", "Calciatore", "Grafico"], "name": "ENTITY"}"
  }
  [3]=>
  array(1) {
    ["doc"]=>
    string(91) "{"_id": 16, "age": 23, "job": ["Calciatore", "Programmatore", "Grafico"], "name": "ENTITY"}"
  }
  [4]=>
  array(1) {
    ["doc"]=>
    string(94) "{"_id": 5, "job": ["Calciatore", "Programmatore", "Grafico"], "auto": "BMW", "name": "ENTITY"}"
  }
  [5]=>
  array(1) {
    ["doc"]=>
    string(94) "{"_id": 7, "job": ["Calciatore", "Programmatore", "Grafico"], "auto": "BMW", "name": "ENTITY"}"
  }
}
done!%A
