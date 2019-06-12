--TEST--
collection [NOT] OVERLAPS operator on string list
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_collection_query_result($criteria, 'idx', 'idx', $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

$coll->add('{"idx": "1", "names": ["Mateusz", "Lech", "Jaroslaw", "Donald"]}')->execute();
$coll->add('{"idx": "2", "names": ["Lech", "Andrzej", "Grzegorz", "Donald"]}')->execute();
$coll->add('{"idx": "3", "names": ["Jaroslaw", "Andrzej", "Pawel", "Donald"]}')->execute();

verify_query_result('["Slawomir", "Lech", "Vincent"] overlaps $.names', array("1", "2"));
verify_query_result('["Mateusz", "Teresa", "Leszek"] overlaps $.names', array("1"));
verify_query_result('["Aleksander", "Lech", "Andrzej"] overlaps $.names', array("1", "2", "3"));
verify_query_result('["Lech", "Andrzej"] overlaps $.names', array("1", "2", "3"));
verify_query_result('["Donald"] overlaps $.names', array("1", "2", "3"));

verify_query_result('["Beata", "Jaroslaw"] not overlaps $.names', array("2"));
verify_query_result('["Andrzej"] not overlaps $.names', array("1"));
verify_query_result('["Donald"] not overlaps $.names', array());
verify_query_result('["Mateusz"] not overlaps $.names', array("2", "3"));

verify_query_result('$.names OVERLAPS ["Lech"]', array("1", "2"));
verify_query_result('$.names OVERLAPS ["Lech", "Jaroslaw"]', array("1", "2", "3"));
verify_query_result('$.names OVERLAPS ["Mateusz", "Pawel"]', array("1", "3"));
verify_query_result('$.names OVERLAPS ["Pawel"]', array("3"));

verify_query_result('$.names NOT OVERLAPS ["Andrzej"]', array("1"));
verify_query_result('$.names NOT OVERLAPS ["Donald"]', array());
verify_query_result('$.names NOT OVERLAPS ["Lech", "Grzegorz"]', array("3"));
verify_query_result('$.names NOT OVERLAPS ["Mateusz", "Lech", "Grzegorz"]', array("3"));

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
