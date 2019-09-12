--TEST--
mysqlx merge-patch
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");
	$session = create_test_db();

	$coll = $session->getSchema($db)->getCollection( $test_collection_name );
	expect_true( null != $coll );
	fill_db_collection( $coll );

	$coll->modify('CAST(_id AS SIGNED) = 1')
		->patch(':newDoc')
		->bind(["newDoc" => '{"name" : "New_Marco"}'])
		->execute();
	$coll->modify('name = :nm')->patch('{"name" : null,"birth" : { "year": 2018-age }}')->bind(['nm' => 'Alfredo'])->execute();
	$coll->modify(true)->patch('{"Hobby" : ["Swimming","Dancing"], "code": concat("secret_" , name) }')->execute();
	$coll->modify('_id IN ["2","5","7","10"]')->patch('{"age": age + 100}')->execute();
	$coll->modify('"Programmatore" IN job')->patch('{"Hobby" : "Programmare"}')->execute();
	$coll->modify('CAST(_id AS SIGNED) >= 10')->patch('{"name" : concat( "UP_", upper(name) )}')->execute();
	$coll->modify('age MOD 2 = 0 OR age MOD 3 = 0')->patch('{"Hobby" : null}')->execute();

	$data = $coll->find()->execute()->fetchAll();
	var_dump( $data );

	verify_expectations();
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
  array(7) {
    ["_id"]=>
    string(1) "1"
    ["age"]=>
    int(19)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(16) "secret_New_Marco"
    ["name"]=>
    string(9) "New_Marco"
    ["Hobby"]=>
    string(11) "Programmare"
    ["ordinal"]=>
    int(1)
  }
  [1]=>
  array(6) {
    ["_id"]=>
    string(2) "10"
    ["age"]=>
    %rint|float%r(129)
    ["job"]=>
    string(11) "Disoccupato"
    ["code"]=>
    string(13) "secret_Giulio"
    ["name"]=>
    string(9) "UP_GIULIO"
    ["ordinal"]=>
    int(10)
  }
  [2]=>
  array(7) {
    ["_id"]=>
    string(2) "11"
    ["age"]=>
    int(47)
    ["job"]=>
    string(7) "Barista"
    ["code"]=>
    string(12) "secret_Lucia"
    ["name"]=>
    string(8) "UP_LUCIA"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(11)
  }
  [3]=>
  array(7) {
    ["_id"]=>
    string(2) "12"
    ["age"]=>
    int(31)
    ["job"]=>
    string(8) "Spazzino"
    ["code"]=>
    string(14) "secret_Filippo"
    ["name"]=>
    string(10) "UP_FILIPPO"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(12)
  }
  [4]=>
  array(6) {
    ["_id"]=>
    string(2) "13"
    ["age"]=>
    int(15)
    ["job"]=>
    string(7) "Barista"
    ["code"]=>
    string(17) "secret_Alessandra"
    ["name"]=>
    string(13) "UP_ALESSANDRA"
    ["ordinal"]=>
    int(13)
  }
  [5]=>
  array(6) {
    ["_id"]=>
    string(2) "14"
    ["age"]=>
    int(22)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(14) "secret_Massimo"
    ["name"]=>
    string(10) "UP_MASSIMO"
    ["ordinal"]=>
    int(14)
  }
  [6]=>
  array(7) {
    ["_id"]=>
    string(2) "15"
    ["age"]=>
    int(37)
    ["job"]=>
    string(10) "Calciatore"
    ["code"]=>
    string(12) "secret_Carlo"
    ["name"]=>
    string(8) "UP_CARLO"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(15)
  }
  [7]=>
  array(7) {
    ["_id"]=>
    string(2) "16"
    ["age"]=>
    int(23)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(15) "secret_Leonardo"
    ["name"]=>
    string(11) "UP_LEONARDO"
    ["Hobby"]=>
    string(11) "Programmare"
    ["ordinal"]=>
    int(16)
  }
  [8]=>
  array(6) {
    ["_id"]=>
    string(1) "2"
    ["age"]=>
    %rint|float%r(159)
    ["job"]=>
    string(8) "Paninaro"
    ["code"]=>
    string(14) "secret_Lonardo"
    ["name"]=>
    string(7) "Lonardo"
    ["ordinal"]=>
    int(2)
  }
  [9]=>
  array(6) {
    ["_id"]=>
    string(1) "3"
    ["age"]=>
    int(27)
    ["job"]=>
    string(8) "Cantante"
    ["code"]=>
    string(15) "secret_Riccardo"
    ["name"]=>
    string(8) "Riccardo"
    ["ordinal"]=>
    int(3)
  }
  [10]=>
  array(7) {
    ["_id"]=>
    string(1) "4"
    ["age"]=>
    int(23)
    ["job"]=>
    string(14) "Programmatrice"
    ["code"]=>
    string(15) "secret_Carlotta"
    ["name"]=>
    string(8) "Carlotta"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(4)
  }
  [11]=>
  array(7) {
    ["_id"]=>
    string(1) "5"
    ["age"]=>
    %rint|float%r(125)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(12) "secret_Carlo"
    ["name"]=>
    string(5) "Carlo"
    ["Hobby"]=>
    string(11) "Programmare"
    ["ordinal"]=>
    int(5)
  }
  [12]=>
  array(7) {
    ["_id"]=>
    string(1) "6"
    ["age"]=>
    int(41)
    ["job"]=>
    string(14) "Programmatrice"
    ["code"]=>
    string(17) "secret_Mariangela"
    ["name"]=>
    string(10) "Mariangela"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(6)
  }
  [13]=>
  array(6) {
    ["_id"]=>
    string(1) "7"
    ["age"]=>
    %rint|float%r(127)
    ["job"]=>
    string(13) "Programmatore"
    ["Hobby"]=>
    string(11) "Programmare"
    ["birth"]=>
    array(1) {
      ["year"]=>
      %rint|float%r(1991)
    }
    ["ordinal"]=>
    int(7)
  }
  [14]=>
  array(6) {
    ["_id"]=>
    string(1) "8"
    ["age"]=>
    int(42)
    ["job"]=>
    string(8) "Studente"
    ["code"]=>
    string(16) "secret_Antonella"
    ["name"]=>
    string(9) "Antonella"
    ["ordinal"]=>
    int(8)
  }
  [15]=>
  array(7) {
    ["_id"]=>
    string(1) "9"
    ["age"]=>
    int(35)
    ["job"]=>
    string(9) "Ballerino"
    ["code"]=>
    string(13) "secret_Monica"
    ["name"]=>
    string(6) "Monica"
    ["Hobby"]=>
    array(2) {
      [0]=>
      string(8) "Swimming"
      [1]=>
      string(7) "Dancing"
    }
    ["ordinal"]=>
    int(9)
  }
}
done!%A
