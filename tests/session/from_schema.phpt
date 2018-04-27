--TEST--
mysqlx get session from schema
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");

function assert_test_schema($schema, $exists) {
	global $test_schema_name;
	global $test_table_name;
	global $test_collection_name;

	expect_eq($schema->getName(), $test_schema_name);
	expect_eq($schema->name, $test_schema_name);
	expect_eq($schema->existsInDatabase(), $exists);

	$table = $schema->getTable($test_table_name);
	expect_eq($table->existsInDatabase(), $exists);

	$coll = $schema->getCollection($test_collection_name);
	expect_eq($coll->existsInDatabase(), $exists);
}

function assert_schema_ok($schema) {
	assert_test_schema($schema, true);
}

function assert_schema_fail($schema) {
	assert_test_schema($schema, false);
}

$session0 = create_test_db();
$schema0 = $session0->getSchema($db);
assert_schema_ok($schema0);

$session1 = $schema0->getSession();
$schema1 = $session1->getSchema($db);
assert_schema_ok($schema1);

$session2 = $schema1->getSession();
$schema2 = $session2->getSchema($db);
assert_schema_ok($schema2);

$session0->close();

$schema3 = $session2->getSchema($db);
assert_schema_fail($schema3);

$session4 = $schema2->getSession();
$schema4 = $session4->getSchema($db);
assert_schema_fail($schema4);

$session5 = $schema3->getSession();
$schema5 = $session5->getSchema($db);
assert_schema_fail($schema5);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/../connect.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
