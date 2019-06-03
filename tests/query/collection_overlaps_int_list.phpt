--TEST--
collection [NOT] OVERLAPS operator on int list
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_collection_query_result($criteria, '_id', '_id', $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

$coll->add('{"_id": "1", "list": [1, 4, 5, 16]}')->execute();
$coll->add('{"_id": "2", "list": [4, 7, 11, 16]}')->execute();
$coll->add('{"_id": "3", "list": [5, 7, 13, 16]}')->execute();

verify_query_result('[1, 2, 3] overlaps $.list', array("1"));
verify_query_result('[1, 4, 7] overlaps $.list', array("1", "2", "3"));
verify_query_result('[4, 7] overlaps $.list', array("1", "2", "3"));
verify_query_result('[16] overlaps $.list', array("1", "2", "3"));

verify_query_result('[3, 5] not overlaps $.list', array("2"));
verify_query_result('[7] not overlaps $.list', array("1"));
verify_query_result('[16] not overlaps $.list', array());
verify_query_result('[1] not overlaps $.list', array("2", "3"));

verify_query_result('$.list OVERLAPS [4]', array("1", "2"));
verify_query_result('$.list OVERLAPS [4, 5]', array("1", "2", "3"));
verify_query_result('$.list OVERLAPS [1, 13]', array("1", "3"));
verify_query_result('$.list OVERLAPS [13]', array("3"));

verify_query_result('$.list NOT OVERLAPS [7]', array("1"));
verify_query_result('$.list NOT OVERLAPS [16]', array());
verify_query_result('$.list NOT OVERLAPS [4, 11]', array("3"));
verify_query_result('$.list NOT OVERLAPS [1, 4, 11]', array("3"));

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
