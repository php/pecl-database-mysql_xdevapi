--TEST--
table [NOT] OVERLAPS operator on int
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_table_query_result("name", $criteria, ['age asc','name asc'], $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);
fill_db_table();

verify_query_result(
	'CAST(age as JSON) overlaps [11, 13, 19]',
	array("Eulalia", "Mamie", "Cassidy"));
verify_query_result('CAST(age as JSON) overlaps [14, 21, 30]', array("Lev", "Olympia"));
verify_query_result('[13, 21, 30] overlaps CAST(age as JSON)', array("Cassidy"));
verify_query_result(
	'[7, 11, 12, 14, 15, 17] not overlaps CAST(age as JSON)',
	array("Cassidy", "Vesper"));

verify_query_result(
	'[17, 14, 10] OVERlaps CAST(age as JSON)',
	array("Lev", "Olympia", "Caspian", "Romy"));
verify_query_result('[18, 10] overLAPS CAST(age as JSON)', array());

verify_query_result(
	'CAST(age as JSON) Not Overlaps [17, 16, 14, 11]',
	array("Polly", "Rufus", "Cassidy", "Octavia", "Tierney"));
verify_query_result(
	'[1, 11, 12, 13, 14, 15, 16, 17, 20] NoT OvErLaPs CAST(age as JSON)',
	array());

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
