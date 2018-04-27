--TEST--
mysqlx get session from table
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");

function assert_test_table($table, $exists, $expected_items_count) {
	global $test_table_name;
	expect_eq($table->getName(), $test_table_name);
	expect_eq($table->name, $test_table_name);
	expect_eq($table->existsInDatabase(), $exists);
	expect_eq($table->count(), $expected_items_count);
}

function assert_table_ok($table) {
	assert_test_table($table, true, 12);
}

function assert_table_fail($table) {
	assert_test_table($table, false, 0);
}

$session0 = create_test_db();
$schema0 = $session0->getSchema($db);
expect_true($schema0->existsInDatabase());
$table0 = $schema0->getTable($test_table_name);
fill_db_table($table0);
assert_table_ok($table0);

$session1 = $table0->getSession();
$schema1 = $session1->getSchema($db);
expect_true($schema1->existsInDatabase());
$table1 = $schema1->getTable($test_table_name);
assert_table_ok($table1);

$session2 = $table1->getSession();
$schema2 = $session2->getSchema($db);
expect_true($schema2->existsInDatabase());
$table2 = $schema2->getTable($test_table_name);
assert_table_ok($table2);

$session0->close();

$schema3 = $session2->getSchema($db);
expect_false($schema3->existsInDatabase());
$table3 = $schema3->getTable($test_table_name);
assert_table_fail($table3);

$session4 = $table2->getSession();
$schema4 = $session4->getSchema($db);
expect_false($schema4->existsInDatabase());
$table4 = $schema4->getTable($test_table_name);
assert_table_fail($table4);

$session5 = $table4->getSession();
$schema5 = $session5->getSchema($db);
expect_false($schema5->existsInDatabase());
$table5 = $schema5->getTable($test_table_name);
assert_table_fail($table5);

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
