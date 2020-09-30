--TEST--
mysqlx collection modify, deleting elements from an array
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);


	$coll->modify("job in ('Programmatore', 'Cantante')")
		->arrayAppend('job', 'Volontario')
		->arrayAppend('job', 'Tassinaro')
		->execute();
	$coll->modify("name in ('Riccardo', 'Carlo')")->unset('job[0]')->execute();
	$coll->modify("name in ('Alfredo', 'Leonardo')")->unset('job[1]')->execute();
	$coll->modify("name like 'Lonardo'")->unset('job[0]')->execute();
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
  array(5) {
    ["_id"]=>
    string(1) "1"
    ["age"]=>
    int(19)
    ["job"]=>
    array(3) {
      [0]=>
      string(13) "Programmatore"
      [1]=>
      string(10) "Volontario"
      [2]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(5) "Marco"
    ["ordinal"]=>
    int(1)
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
  array(5) {
    ["_id"]=>
    string(2) "11"
    ["age"]=>
    int(47)
    ["job"]=>
    string(7) "Barista"
    ["name"]=>
    string(5) "Lucia"
    ["ordinal"]=>
    int(11)
  }
  [3]=>
  array(5) {
    ["_id"]=>
    string(2) "12"
    ["age"]=>
    int(31)
    ["job"]=>
    string(8) "Spazzino"
    ["name"]=>
    string(7) "Filippo"
    ["ordinal"]=>
    int(12)
  }
  [4]=>
  array(5) {
    ["_id"]=>
    string(2) "13"
    ["age"]=>
    int(15)
    ["job"]=>
    string(7) "Barista"
    ["name"]=>
    string(10) "Alessandra"
    ["ordinal"]=>
    int(13)
  }
  [5]=>
  array(5) {
    ["_id"]=>
    string(2) "14"
    ["age"]=>
    int(22)
    ["job"]=>
    array(3) {
      [0]=>
      string(13) "Programmatore"
      [1]=>
      string(10) "Volontario"
      [2]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(7) "Massimo"
    ["ordinal"]=>
    int(14)
  }
  [6]=>
  array(4) {
    ["_id"]=>
    string(2) "15"
    ["age"]=>
    int(37)
    ["name"]=>
    string(5) "Carlo"
    ["ordinal"]=>
    int(15)
  }
  [7]=>
  array(5) {
    ["_id"]=>
    string(2) "16"
    ["age"]=>
    int(23)
    ["job"]=>
    array(2) {
      [0]=>
      string(13) "Programmatore"
      [1]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(8) "Leonardo"
    ["ordinal"]=>
    int(16)
  }
  [8]=>
  array(4) {
    ["_id"]=>
    string(1) "2"
    ["age"]=>
    int(59)
    ["name"]=>
    string(7) "Lonardo"
    ["ordinal"]=>
    int(2)
  }
  [9]=>
  array(5) {
    ["_id"]=>
    string(1) "3"
    ["age"]=>
    int(27)
    ["job"]=>
    array(2) {
      [0]=>
      string(10) "Volontario"
      [1]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(8) "Riccardo"
    ["ordinal"]=>
    int(3)
  }
  [10]=>
  array(5) {
    ["_id"]=>
    string(1) "4"
    ["age"]=>
    int(23)
    ["job"]=>
    string(14) "Programmatrice"
    ["name"]=>
    string(8) "Carlotta"
    ["ordinal"]=>
    int(4)
  }
  [11]=>
  array(5) {
    ["_id"]=>
    string(1) "5"
    ["age"]=>
    int(25)
    ["job"]=>
    array(2) {
      [0]=>
      string(10) "Volontario"
      [1]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(5) "Carlo"
    ["ordinal"]=>
    int(5)
  }
  [12]=>
  array(5) {
    ["_id"]=>
    string(1) "6"
    ["age"]=>
    int(41)
    ["job"]=>
    string(14) "Programmatrice"
    ["name"]=>
    string(10) "Mariangela"
    ["ordinal"]=>
    int(6)
  }
  [13]=>
  array(5) {
    ["_id"]=>
    string(1) "7"
    ["age"]=>
    int(27)
    ["job"]=>
    array(2) {
      [0]=>
      string(13) "Programmatore"
      [1]=>
      string(9) "Tassinaro"
    }
    ["name"]=>
    string(7) "Alfredo"
    ["ordinal"]=>
    int(7)
  }
  [14]=>
  array(5) {
    ["_id"]=>
    string(1) "8"
    ["age"]=>
    int(42)
    ["job"]=>
    string(8) "Studente"
    ["name"]=>
    string(9) "Antonella"
    ["ordinal"]=>
    int(8)
  }
  [15]=>
  array(5) {
    ["_id"]=>
    string(1) "9"
    ["age"]=>
    int(35)
    ["job"]=>
    string(9) "Ballerino"
    ["name"]=>
    string(6) "Monica"
    ["ordinal"]=>
    int(9)
  }
}
done!%A
