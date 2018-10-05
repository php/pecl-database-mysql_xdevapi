--TEST--
mysqlx get session from collection
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/session_utils.inc");

function assert_test_collection($coll, $exists, $expected_items_count) {
	global $test_collection_name;
	expect_eq($coll->getName(), $test_collection_name);
	expect_eq($coll->name, $test_collection_name);
	expect_eq($coll->existsInDatabase(), $exists);
	expect_eq($coll->count(), $expected_items_count);
}

function assert_collection_ok($coll) {
	assert_test_collection($coll, true, 16);
}

function assert_collection_fail($coll) {
	assert_test_collection($coll, false, 0);
}

$session0 = create_test_db();
$schema0 = $session0->getSchema($db);
expect_true($schema0->existsInDatabase());
$coll0 = $schema0->getCollection($test_collection_name);
fill_db_collection($coll0);
assert_collection_ok($coll0);

$session1 = $coll0->getSession();
$schema1 = $session1->getSchema($db);
expect_true($schema1->existsInDatabase());
$coll1 = $schema1->getCollection($test_collection_name);
assert_collection_ok($coll1);

$session2 = $coll1->getSession();
$schema2 = $session2->getSchema($db);
expect_true($schema2->existsInDatabase());
$coll2 = $schema2->getCollection($test_collection_name);
assert_collection_ok($coll2);

$session0->close();

assert_session_invalid($session0);
assert_session_invalid($session1);
assert_session_invalid($session2);

assert_collection_fail($coll0);
assert_collection_fail($coll1);
assert_collection_fail($coll2);

$session4 = $coll0->getSession();
assert_session_invalid($session4);

$session5 = $coll1->getSession();
assert_session_invalid($session5);

$session6 = $coll2->getSession();
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
