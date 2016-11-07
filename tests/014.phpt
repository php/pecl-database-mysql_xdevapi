--TEST--
xmysqlnd collection modify sort/replace/merge
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$obj = $coll->modify('age > :age1 and age < :age2')->bind(['age1' => 25,'age2' => 40]);
	$data = $obj->sort(['_id desc'])->limit(2)->replace('job','Disoccupato')->execute();

	var_dump($coll->find('job like \'Disoccupato\'')->execute()->fetchAll());

	$coll->remove('(_id > 1 and _id < 8) or (_id > 11 and _id < 15)')->execute();
	$coll->modify('_id >= 1 and _id <= 9')->unset(['age'])->execute();
	$coll->modify()->sort(['name desc','age asc'])->limit(4)->set('Married','NO')->execute();

	$coll->modify('Married like \'NO\'')->merge('{\'Divorced\' : \'NO\' , \'Vegan\' : \'YES\'}')->execute();

	var_dump($coll->find()->execute()->fetchAll());
        print "done!\n";
?>
--CLEAN--
<?php
    require("connect.inc");
    clean_test_db();
?>
--EXPECTF--
array(3) {
  [0]=>
  array(1) {
    ["doc"]=>
    string(62) "{"_id": 10, "age": 29, "job": "Disoccupato", "name": "Giulio"}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(62) "{"_id": 7, "age": 27, "job": "Disoccupato", "name": "Alfredo"}"
  }
  [2]=>
  array(1) {
    ["doc"]=>
    string(61) "{"_id": 9, "age": 35, "job": "Disoccupato", "name": "Monica"}"
  }
}
array(7) {
  [0]=>
  array(1) {
    ["doc"]=>
    string(102) "{"_id": 1, "job": "Programmatore", "name": "Marco", "Vegan": "YES", "Married": "NO", "Divorced": "NO"}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(62) "{"_id": 10, "age": 29, "job": "Disoccupato", "name": "Giulio"}"
  }
  [2]=>
  array(1) {
    ["doc"]=>
    string(108) "{"_id": 11, "age": 47, "job": "Barista", "name": "Lucia", "Vegan": "YES", "Married": "NO", "Divorced": "NO"}"
  }
  [3]=>
  array(1) {
    ["doc"]=>
    string(60) "{"_id": 15, "age": 37, "job": "Calciatore", "name": "Carlo"}"
  }
  [4]=>
  array(1) {
    ["doc"]=>
    string(117) "{"_id": 16, "age": 23, "job": "Programmatore", "name": "Leonardo", "Vegan": "YES", "Married": "NO", "Divorced": "NO"}"
  }
  [5]=>
  array(1) {
    ["doc"]=>
    string(50) "{"_id": 8, "job": "Studente", "name": "Antonella"}"
  }
  [6]=>
  array(1) {
    ["doc"]=>
    string(101) "{"_id": 9, "job": "Disoccupato", "name": "Monica", "Vegan": "YES", "Married": "NO", "Divorced": "NO"}"
  }
}
done!
%a

