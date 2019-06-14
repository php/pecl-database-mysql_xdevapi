--TEST--
collection [NOT] OVERLAPS operator on int
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_collection_query_result($criteria, 'name', 'ordinal', $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);
fill_db_collection($coll);

verify_query_result(
	'[19, 27, 37] overlaps $.age',
	array("Marco", "Riccardo", "Alfredo", "Carlo"));
verify_query_result('[44, 42, 13] overlaps $.age', array("Antonella"));
verify_query_result('[7, 1, 4] overlaps $.age', array());

verify_query_result(
	'$.age OVERLAPS [15, 47, 29, 15]',
	array("Giulio", "Lucia", "Alessandra"));
verify_query_result('$.age OVERLAPS [23]', array("Carlotta", "Leonardo"));
verify_query_result('$.age OVERLAPS [13, 24, 36, 13]', array());

verify_query_result(
	'[19, 59, 27, 23, 25, 41, 27, 42, 35, 29, 47, 31, 15, 22, 37, 23] not overlaps $.age',
	array());
verify_query_result(
	'[19, 27, 23, 41, 27, 35, 29, 47, 15, 37, 23] NOT overlaps $.age',
	array("Lonardo", "Carlo", "Antonella", "Filippo", "Massimo"));
verify_query_result(
	'[19, 59, 27, 25, 41, 27, 42, 35, 29, 47, 31, 15, 22, 37] not overlaps $.age',
	array("Carlotta", "Leonardo"));

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
