--TEST--
collection [NOT] OVERLAPS operator on string
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_collection_query_result($criteria, '_id', 'ordinal', $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);
fill_db_collection($coll);

verify_query_result('["Monica"] overlaps $.name', array("9"));
verify_query_result('["Carlo", "Carlotta"] OVERLAPS $.name', array("4", "5", "15"));
verify_query_result(
	'["Marco", "Riccardo", "Alfredo", "Giulio", "Alessandra", "Carlo"] not overlaps $.name',
	array("2", "4", "6", "8", "9", "11", "12", "14", "16"));

verify_query_result('["Paninaro", "Cantante"] overlaps $.job', array("2", "3"));
verify_query_result('["Studente"] OVERLAPS $.job', array("8"));
verify_query_result(
	'["Programmatore", "Barista", "Programmatrice", "Calciatore"] NOT OVERLAPS $.job',
	array("2", "3", "8", "9", "10", "12"));

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
