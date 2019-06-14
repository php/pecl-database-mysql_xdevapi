--TEST--
table [NOT] OVERLAPS operator on json int list
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_table_query_result("idx", $criteria, "idx asc", $expected_result);
}

$session = create_test_db(null, "idx int, list json");
$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);

$table->insert('idx', 'list')->values(
	[1, '[1, 4, 6, 32]'],
	[2, '[4, 9, 21, 32]'],
	[3, '[6, 9, 23, 42]'],
	[4, '[1, 2, 4, 8, 16, 32]']
	)->execute();

verify_query_result('[1, 2, 3] overlaps list', array(1, 4));
verify_query_result('[1, 4, 9] overlaps list', array(1, 2, 3, 4));
verify_query_result('[4, 9] overlaps list', array(1, 2, 3, 4));
verify_query_result('[32] overlaps list', array(1, 2, 4));

verify_query_result('[3, 6] not overlaps list', array(2, 4));
verify_query_result('[9] not overlaps list', array(1, 4));
verify_query_result('[32] not overlaps list', array(3));
verify_query_result('[1] not overlaps list', array(2, 3));

verify_query_result('list OVERLAPS [4]', array(1, 2, 4));
verify_query_result('list OVERLAPS [4, 6]', array(1, 2, 3, 4));
verify_query_result('list OVERLAPS [1, 23]', array(1, 3, 4));
verify_query_result('list OVERLAPS [23, 42]', array(3));
verify_query_result('list OVERLAPS [3, 5, 7]', array());

verify_query_result('list NOT OVERLAPS [9]', array(1, 4));
verify_query_result('list NOT OVERLAPS [32]', array(3));
verify_query_result('list NOT OVERLAPS [4, 21]', array(3));
verify_query_result('list NOT OVERLAPS [1, 4, 21]', array(3));
verify_query_result('list NOT OVERLAPS [6, 9, 16]', array());

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/query_utils.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
