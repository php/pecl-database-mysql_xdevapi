--TEST--
mysqlx merge-patch
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");
	$nodeSession = create_test_db();

	$coll = $nodeSession->getSchema($db)->getCollection( $test_collection_name );
	expect_true( null != $coll );
	fill_db_collection( $coll );

        $coll->modify('_id = 1')->patch('{"name" : "New_Marco"}')->execute();
	$coll->modify('name = :nm')->patch('{"name" : null,"birth" : { "year": year(CURDATE())-age }}')->bind(['nm' => 'Alfredo'])->execute();
	$coll->modify(true)->patch('{"Hobby" : ["Swimming","Dancing"], "code": concat("secret_" , name) }')->execute();
	$coll->modify('_id IN [2,5,7,10]')->patch('{"age": age + 100}')->execute();
	$coll->modify('"Programmatore" IN job')->patch('{"Hobby" : "Programmare"}')->execute();
	$coll->modify('_id >= 10')->patch('{"name" : concat( "UP_", upper(name) )}')->execute();
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
  array(6) {
    ["_id"]=>
    int(1)
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
  }
  [1]=>
  array(5) {
    ["_id"]=>
    int(10)
    ["age"]=>
    int(129)
    ["job"]=>
    string(11) "Disoccupato"
    ["code"]=>
    string(13) "secret_Giulio"
    ["name"]=>
    string(9) "UP_GIULIO"
  }
  [2]=>
  array(6) {
    ["_id"]=>
    int(11)
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
  }
  [3]=>
  array(6) {
    ["_id"]=>
    int(12)
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
  }
  [4]=>
  array(5) {
    ["_id"]=>
    int(13)
    ["age"]=>
    int(15)
    ["job"]=>
    string(7) "Barista"
    ["code"]=>
    string(17) "secret_Alessandra"
    ["name"]=>
    string(13) "UP_ALESSANDRA"
  }
  [5]=>
  array(5) {
    ["_id"]=>
    int(14)
    ["age"]=>
    int(22)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(14) "secret_Massimo"
    ["name"]=>
    string(10) "UP_MASSIMO"
  }
  [6]=>
  array(6) {
    ["_id"]=>
    int(15)
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
  }
  [7]=>
  array(6) {
    ["_id"]=>
    int(16)
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
  }
  [8]=>
  array(5) {
    ["_id"]=>
    int(2)
    ["age"]=>
    int(159)
    ["job"]=>
    string(8) "Paninaro"
    ["code"]=>
    string(14) "secret_Lonardo"
    ["name"]=>
    string(7) "Lonardo"
  }
  [9]=>
  array(5) {
    ["_id"]=>
    int(3)
    ["age"]=>
    int(27)
    ["job"]=>
    string(8) "Cantante"
    ["code"]=>
    string(15) "secret_Riccardo"
    ["name"]=>
    string(8) "Riccardo"
  }
  [10]=>
  array(6) {
    ["_id"]=>
    int(4)
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
  }
  [11]=>
  array(6) {
    ["_id"]=>
    int(5)
    ["age"]=>
    int(125)
    ["job"]=>
    string(13) "Programmatore"
    ["code"]=>
    string(12) "secret_Carlo"
    ["name"]=>
    string(5) "Carlo"
    ["Hobby"]=>
    string(11) "Programmare"
  }
  [12]=>
  array(6) {
    ["_id"]=>
    int(6)
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
  }
  [13]=>
  array(5) {
    ["_id"]=>
    int(7)
    ["age"]=>
    int(127)
    ["job"]=>
    string(13) "Programmatore"
    ["Hobby"]=>
    string(11) "Programmare"
    ["birth"]=>
    array(1) {
      ["year"]=>
      int(1990)
    }
  }
  [14]=>
  array(5) {
    ["_id"]=>
    int(8)
    ["age"]=>
    int(42)
    ["job"]=>
    string(8) "Studente"
    ["code"]=>
    string(16) "secret_Antonella"
    ["name"]=>
    string(9) "Antonella"
  }
  [15]=>
  array(6) {
    ["_id"]=>
    int(9)
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
  }
}
done!%A
