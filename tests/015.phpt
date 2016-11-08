--TEST--
mysqlx collection modify arrayDelete
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);


	$coll->modify('job in (\'Programmatore\', \'Cantante\')')->arrayAppend('job', 'Volontario')->arrayAppend('job', 'Tassinaro')->execute();
	$coll->modify('name in (\'Riccardo\', \'Carlo\')')->arrayDelete('job[0]')->execute();
	$coll->modify('name in (\'Alfredo\', \'Leonardo\')')->arrayDelete('job[1]')->execute();
	$coll->modify('name like \'Lonardo\'')->arrayDelete('job[0]')->execute();
	var_dump($coll->find()->execute()->fetchAll());
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
array(16) {
  [0]=>
  array(1) {
    ["doc"]=>
    string(91) "{"_id": 1, "age": 19, "job": ["Programmatore", "Volontario", "Tassinaro"], "name": "Marco"}"
  }
  [1]=>
  array(1) {
    ["doc"]=>
    string(62) "{"_id": 10, "age": 29, "job": "Disoccupato", "name": "Giulio"}"
  }
  [2]=>
  array(1) {
    ["doc"]=>
    string(57) "{"_id": 11, "age": 47, "job": "Barista", "name": "Lucia"}"
  }
  [3]=>
  array(1) {
    ["doc"]=>
    string(60) "{"_id": 12, "age": 31, "job": "Spazzino", "name": "Filippo"}"
  }
  [4]=>
  array(1) {
    ["doc"]=>
    string(62) "{"_id": 13, "age": 15, "job": "Barista", "name": "Alessandra"}"
  }
  [5]=>
  array(1) {
    ["doc"]=>
    string(94) "{"_id": 14, "age": 22, "job": ["Programmatore", "Volontario", "Tassinaro"], "name": "Massimo"}"
  }
  [6]=>
  array(1) {
    ["doc"]=>
    string(39) "{"_id": 15, "age": 37, "name": "Carlo"}"
  }
  [7]=>
  array(1) {
    ["doc"]=>
    string(81) "{"_id": 16, "age": 23, "job": ["Programmatore", "Tassinaro"], "name": "Leonardo"}"
  }
  [8]=>
  array(1) {
    ["doc"]=>
    string(40) "{"_id": 2, "age": 59, "name": "Lonardo"}"
  }
  [9]=>
  array(1) {
    ["doc"]=>
    string(77) "{"_id": 3, "age": 27, "job": ["Volontario", "Tassinaro"], "name": "Riccardo"}"
  }
  [10]=>
  array(1) {
    ["doc"]=>
    string(66) "{"_id": 4, "age": 23, "job": "Programmatrice", "name": "Carlotta"}"
  }
  [11]=>
  array(1) {
    ["doc"]=>
    string(74) "{"_id": 5, "age": 25, "job": ["Volontario", "Tassinaro"], "name": "Carlo"}"
  }
  [12]=>
  array(1) {
    ["doc"]=>
    string(68) "{"_id": 6, "age": 41, "job": "Programmatrice", "name": "Mariangela"}"
  }
  [13]=>
  array(1) {
    ["doc"]=>
    string(79) "{"_id": 7, "age": 27, "job": ["Programmatore", "Tassinaro"], "name": "Alfredo"}"
  }
  [14]=>
  array(1) {
    ["doc"]=>
    string(61) "{"_id": 8, "age": 42, "job": "Studente", "name": "Antonella"}"
  }
  [15]=>
  array(1) {
    ["doc"]=>
    string(59) "{"_id": 9, "age": 35, "job": "Ballerino", "name": "Monica"}"
  }
}
done!%A
