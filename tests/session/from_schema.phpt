--TEST--
mysqlx get session from schema
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/session_utils.inc");

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

assert_session_invalid($session0);
assert_session_invalid($session1);
assert_session_invalid($session2);

assert_schema_fail($schema0);
assert_schema_fail($schema1);
assert_schema_fail($schema2);

$session4 = $schema0->getSession();
assert_session_invalid($session4);

$session5 = $schema1->getSession();
assert_session_invalid($session5);

$session6 = $schema2->getSession();
assert_session_invalid($session6);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/../connect.inc");
clean_test_db();
?>
--EXPECTF--
[10056][HY000] Session closed.
[10056][HY000] Session closed.
[10056][HY000] Session closed.
[10056][HY000] Session closed.
[10056][HY000] Session closed.
[10056][HY000] Session closed.
done!%A
