--TEST--
mysqlx get session from collection
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");

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

$schema3 = $session2->getSchema($db);
expect_false($schema3->existsInDatabase());
$coll3 = $schema3->getCollection($test_collection_name);
assert_collection_fail($coll3);

$session4 = $coll2->getSession();
$schema4 = $session4->getSchema($db);
expect_false($schema4->existsInDatabase());
$coll4 = $schema4->getCollection($test_collection_name);
assert_collection_fail($coll4);

$session5 = $coll4->getSession();
$schema5 = $session5->getSchema($db);
expect_false($schema5->existsInDatabase());
$coll5 = $schema5->getCollection($test_collection_name);
assert_collection_fail($coll5);

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
