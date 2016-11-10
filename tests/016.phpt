--TEST--
mysqlx collection delete
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$coll->remove('age > :age_from and age < :age_to')->bind(['age_from' => 20, 'age_to' => 50])->limit(7)->execute();

	$coll->remove()->sort('age desc')->limit(2)->execute();
	$coll->modify('_id in (1,13,5,7)')->unset(['age'])->execute();
	$coll->remove('job in (\'Barista\', \'Programmatore\', \'Ballerino\', \'Programmatrice\')')->limit(5)->sort(['age desc', 'name asc'])->execute();

	var_dump($coll->find()->execute()->fetchAll());
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
  array(3) {
    ["_id"]=>
    int(1)
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(5) "Marco"
  }
  [1]=>
  array(3) {
    ["_id"]=>
    int(5)
    ["job"]=>
    string(13) "Programmatore"
    ["name"]=>
    string(5) "Carlo"
  }
}
done!%A
