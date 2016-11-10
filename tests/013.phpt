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
  array(4) {
    ["_id"]=>
    int(1)
    ["age"]=>
    int(19)
    ["job"]=>
    array(3) {
      [0]=>
      string(10) "Calciatore"
      [1]=>
      string(13) "Programmatore"
      [2]=>
      string(7) "Grafico"
    }
    ["name"]=>
    string(6) "ENTITY"
  }
  [1]=>
  array(3) {
    ["_id"]=>
    int(11)
    ["job"]=>
    string(7) "Barista"
    ["name"]=>
    string(5) "Lucia"
  }
  [2]=>
  array(3) {
    ["_id"]=>
    int(15)
    ["job"]=>
    array(3) {
      [0]=>
      string(10) "Calciatore"
      [1]=>
      string(10) "Calciatore"
      [2]=>
      string(7) "Grafico"
    }
    ["name"]=>
    string(6) "ENTITY"
  }
  [3]=>
  array(4) {
    ["_id"]=>
    int(16)
    ["age"]=>
    int(23)
    ["job"]=>
    array(3) {
      [0]=>
      string(10) "Calciatore"
      [1]=>
      string(13) "Programmatore"
      [2]=>
      string(7) "Grafico"
    }
    ["name"]=>
    string(6) "ENTITY"
  }
  [4]=>
  array(4) {
    ["_id"]=>
    int(5)
    ["job"]=>
    array(3) {
      [0]=>
      string(10) "Calciatore"
      [1]=>
      string(13) "Programmatore"
      [2]=>
      string(7) "Grafico"
    }
    ["auto"]=>
    string(3) "BMW"
    ["name"]=>
    string(6) "ENTITY"
  }
  [5]=>
  array(4) {
    ["_id"]=>
    int(7)
    ["job"]=>
    array(3) {
      [0]=>
      string(10) "Calciatore"
      [1]=>
      string(13) "Programmatore"
      [2]=>
      string(7) "Grafico"
    }
    ["auto"]=>
    string(3) "BMW"
    ["name"]=>
    string(6) "ENTITY"
  }
}
done!%A
