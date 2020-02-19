--TEST--
mysqlx collection modify sort/replace/merge
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$obj = $coll->modify('age > :age1 and age < :age2')->bind(['age1' => 25, 'age2' => 40]);
	$data = $obj->sort(['_id desc'])->limit(2)
		->replace('job', ':newJob')
		->bind(['newJob' => 'Disoccupato'])
		->execute();

	var_dump($coll->find("job like 'Disoccupato'")->execute()->fetchAll());

	$coll->remove('(ordinal > 1 and ordinal < 8) or (ordinal > 11 and ordinal < 15)')->execute();
	$coll->modify('ordinal >= 1 and ordinal <= 9')->unset(['age'])->execute();
	$coll->modify('true')->sort('name desc', 'age asc')->limit(4)->set('Married', 'NO')->execute();

	$coll->modify("Married like 'NO'")->patch('{"Divorced" : "NO", "Vegan" : "YES"}')->execute();

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
  array(5) {
    ["_id"]=>
    string(2) "10"
    ["age"]=>
    int(29)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Giulio"
    ["ordinal"]=>
    int(10)
  }
  [1]=>
  array(5) {
    ["_id"]=>
    string(1) "7"
    ["age"]=>
    int(27)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(7) "Alfredo"
    ["ordinal"]=>
    int(7)
  }
  [2]=>
  array(5) {
    ["_id"]=>
    string(1) "9"
    ["age"]=>
    int(35)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Monica"
    ["ordinal"]=>
    int(9)
  }
}
array(7) {
  [0]=>
  array(7) {
    ["_id"]=>
    string(1) "1"
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(5) "Marco"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["ordinal"]=>
    int(1)
    ["Divorced"]=>
    string(2) "NO"
  }
  [1]=>
  array(5) {
    ["_id"]=>
    string(2) "10"
    ["age"]=>
    int(29)
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Giulio"
    ["ordinal"]=>
    int(10)
  }
  [2]=>
  array(8) {
    ["_id"]=>
    string(2) "11"
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
    ["ordinal"]=>
    int(11)
    ["Divorced"]=>
    string(2) "NO"
  }
  [3]=>
  array(5) {
    ["_id"]=>
    string(2) "15"
    ["age"]=>
    int(37)
    ["job"]=>
    string(10) "Calciatore"
    ["name"]=>
    string(5) "Carlo"
    ["ordinal"]=>
    int(15)
  }
  [4]=>
  array(8) {
    ["_id"]=>
    string(2) "16"
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
    ["ordinal"]=>
    int(16)
    ["Divorced"]=>
    string(2) "NO"
  }
  [5]=>
  array(4) {
    ["_id"]=>
    string(1) "8"
    ["job"]=>
    string(8) "Studente"
    ["name"]=>
    string(9) "Antonella"
    ["ordinal"]=>
    int(8)
  }
  [6]=>
  array(7) {
    ["_id"]=>
    string(1) "9"
    ["job"]=>
    string(11) "Disoccupato"
    ["name"]=>
    string(6) "Monica"
    ["Vegan"]=>
    string(3) "YES"
    ["Married"]=>
    string(2) "NO"
    ["ordinal"]=>
    int(9)
    ["Divorced"]=>
    string(2) "NO"
  }
}
done!%A
