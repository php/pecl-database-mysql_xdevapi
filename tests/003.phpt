--TEST--
mysqlx select / fetch
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$test = "100";
	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert(["name", "age"])->values(["Sakila", 128])->values(["Oracila", 512])->execute();
	$table->insert(["name", "age"])->values(["Jackie", 256])->execute();
	$res = $table->select(['age', 'name'])->execute();

	var_dump($table->getName());
	var_dump($res->fetchOne());
	var_dump($res->fetchOne());
	var_dump($res->fetchOne());

	$all_row = $res->fetchAll();
	var_dump($all_row);

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
string(10) "test_table"
array(2) {
  ["age"]=>
  int(128)
  ["name"]=>
  string(6) "Sakila"
}
array(2) {
  ["age"]=>
  int(512)
  ["name"]=>
  string(7) "Oracila"
}
array(2) {
  ["age"]=>
  int(256)
  ["name"]=>
  string(6) "Jackie"
}
array(3) {
  [0]=>
  array(2) {
    ["age"]=>
    int(128)
    ["name"]=>
    string(6) "Sakila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(512)
    ["name"]=>
    string(7) "Oracila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(256)
    ["name"]=>
    string(6) "Jackie"
  }
}
done!%A
