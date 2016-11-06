--TEST--
xmysqlnd collection find
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

function dump_all_row($table){
    $res = $table->select(['age','name'])->execute();
    $all_row = $res->fetchAll();
    var_dump($all_row);
}

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	//TODO, fields do not work for non-array types
	$res = $coll->find('job like :job and age > :age')->fields(['age']);
	$res = $res->bind(['job' => 'Programmatore','age' => 20])->sort('age desc')->limit(2);
	$data = $res->execute();

	var_dump($data->fetchAll());

	$res = $coll->find('job like :job and age < :age')->fields(['age','name']);
	$res = $res->bind(['job' => 'Programmatore','age' => 40])->sort('age desc')->limit(2);
	$data = $res->execute();

	var_dump($data->fetchAll());

	//TODO: Need some clarification how this groupBy works
/*
	$res = $coll->find('job like :job and age > :age')->groupBy('age');
	$res = $res->bind(['job' => 'Programmatore','age' => 20])->sort('age desc')->limit(2);
	$data = $res->execute();
*/
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
    ["doc"]=>
    string(11) "{"age": 27}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(11) "{"age": 25}"
  }
}
array(2) {
  [0]=>
  array(1) {
    ["doc"]=>
    string(30) "{"age": 27, "name": "Alfredo"}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(28) "{"age": 25, "name": "Carlo"}"
  }
}
done!
%a

