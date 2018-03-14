--TEST--
mysqlx getCollectionAsTable
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	fill_test_collection();

	$schema = $session->getSchema($db);
	$collection_as_table = $schema->getCollectionAsTable("test_collection");
	expect_false($collection_as_table->isView());
	$collection_as_table->delete()->limit(10)->execute();
	$collection_as_table->insert(["doc"])
		->values(['{"_id": "128", "name": "T-1000", "age": 512, "job": "Terminator", "year": 1991}'])
		->values(['{"_id": "1024", "surname": "Prototype", "kind": "Cyborg"}'])
		->execute();
	$collection_as_table->insert(["doc"])->values(['{"_id": "2048"}'])->execute();
	$collection_as_table->update()->set('doc', '{"_id": "7", "name": "Czesiek", "job": "Zlota Raczka"}')->where('_id = "7"')->execute();
	$collection_as_table->update()->set('doc', '{"_id": "4", "name": "Helena", "spouse": "Marian Pazdzioch"}')->where('_id = "4"')->execute();
	$res = $collection_as_table->select(['_id', 'doc'])->orderBy("_id")->execute();

	var_dump($res->fetchAll());

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
array(9) {
  [0]=>
  array(2) {
    ["_id"]=>
    string(4) "1024"
    ["doc"]=>
    string(57) "{"_id": "1024", "kind": "Cyborg", "surname": "Prototype"}"
  }
  [1]=>
  array(2) {
    ["_id"]=>
    string(3) "128"
    ["doc"]=>
    string(79) "{"_id": "128", "age": 512, "job": "Terminator", "name": "T-1000", "year": 1991}"
  }
  [2]=>
  array(2) {
    ["_id"]=>
    string(4) "2048"
    ["doc"]=>
    string(15) "{"_id": "2048"}"
  }
  [3]=>
  array(2) {
    ["_id"]=>
    string(1) "4"
    ["doc"]=>
    string(60) "{"_id": "4", "name": "Helena", "spouse": "Marian Pazdzioch"}"
  }
  [4]=>
  array(2) {
    ["_id"]=>
    string(1) "5"
    ["doc"]=>
    string(78) "{"_id": "5", "age": 25, "job": "Programmatore", "name": "Carlo", "ordinal": 5}"
  }
  [5]=>
  array(2) {
    ["_id"]=>
    string(1) "6"
    ["doc"]=>
    string(84) "{"_id": "6", "age": 41, "job": "Programmatrice", "name": "Mariangela", "ordinal": 6}"
  }
  [6]=>
  array(2) {
    ["_id"]=>
    string(1) "7"
    ["doc"]=>
    string(54) "{"_id": "7", "job": "Zlota Raczka", "name": "Czesiek"}"
  }
  [7]=>
  array(2) {
    ["_id"]=>
    string(1) "8"
    ["doc"]=>
    string(77) "{"_id": "8", "age": 42, "job": "Studente", "name": "Antonella", "ordinal": 8}"
  }
  [8]=>
  array(2) {
    ["_id"]=>
    string(1) "9"
    ["doc"]=>
    string(75) "{"_id": "9", "age": 35, "job": "Ballerino", "name": "Monica", "ordinal": 9}"
  }
}
done!%A
