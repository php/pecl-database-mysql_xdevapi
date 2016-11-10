--TEST--
mysqlx collection modify sort/replace/merge
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$obj = $coll->modify('age > :age1 and age < :age2')->bind(['age1' => 25, 'age2' => 40]);
	$data = $obj->sort(['_id desc'])->limit(2)->replace('job', 'Disoccupato')->execute();

	var_dump($coll->find('job like \'Disoccupato\'')->execute()->fetchAll());

	$coll->remove('(_id > 1 and _id < 8) or (_id > 11 and _id < 15)')->execute();
	$coll->modify('_id >= 1 and _id <= 9')->unset(['age'])->execute();
	$coll->modify()->sort(['name desc', 'age asc'])->limit(4)->set('Married', 'NO')->execute();

	$coll->modify('Married like \'NO\'')->merge('{\'Divorced\' : \'NO\', \'Vegan\' : \'YES\'}')->execute();

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
  array(4) {
    ["_id"]=>
    int(10)
    ["age"]=>
    int(29)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Giulio"
  }
  [1]=>
  array(4) {
    ["_id"]=>
    int(7)
    ["age"]=>
    int(27)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(7) "Alfredo"
  }
  [2]=>
  array(4) {
    ["_id"]=>
    int(9)
    ["age"]=>
    int(35)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Monica"
  }
}
array(7) {
  [0]=>
  array(6) {
    ["_id"]=>
    int(1)
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(5) "Marco"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["Divorced"]=>
    string(2) "NO"
  }
  [1]=>
  array(4) {
    ["_id"]=>
    int(10)
    ["age"]=>
    int(29)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Giulio"
  }
  [2]=>
  array(7) {
    ["_id"]=>
    int(11)
    ["age"]=>
    int(47)
    ["job"]=>
    string(7) "Barista"
    ["name"]=>
    string(5) "Lucia"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["Divorced"]=>
    string(2) "NO"
  }
  [3]=>
  array(4) {
    ["_id"]=>
    int(15)
    ["age"]=>
    int(37)
    ["job"]=>
    string(10) "Calciatore"
    ["name"]=>
    string(5) "Carlo"
  }
  [4]=>
  array(7) {
    ["_id"]=>
    int(16)
    ["age"]=>
    int(23)
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(8) "Leonardo"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["Divorced"]=>
    string(2) "NO"
  }
  [5]=>
  array(3) {
    ["_id"]=>
    int(8)
    ["job"]=>
    string(8) "Studente"
    ["name"]=>
    string(9) "Antonella"
  }
  [6]=>
  array(6) {
    ["_id"]=>
    int(9)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Monica"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["Divorced"]=>
    string(2) "NO"
  }
}
done!%A
