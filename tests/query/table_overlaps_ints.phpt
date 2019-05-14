--TEST--
table [NOT] OVERLAPS operator on ints
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/query_utils.inc");

	clean_test_db();

function verify_query_result($criteria, $expected_result) {
	global $table;
	verify_table_query_result($table, "name", $criteria, "name", $expected_result);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);
fill_db_table();

//verify_query_result('age overlaps (11, 13, 19)', array("Mamie", "Eulalia", "Cassidy"));
verify_query_result(":age overlaps ['11']", array("Mamie", "Eulalia", "Cassidy"));
// verify_query_result('[1, 4, 7] overlaps $.age', array("1", "2", "3"));
// verify_query_result('[4, 7] overlaps $.age', array("1", "2", "3"));
// verify_query_result('[16] overlaps $.age', array("1", "2", "3"));

// verify_query_result('[3, 5] not overlaps $.age', array("2"));
// verify_query_result('[7] not overlaps $.age', array("1"));
// verify_query_result('[16] not overlaps $.age', array());
// verify_query_result('[1] not overlaps $.age', array("2", "3"));

// verify_query_result('$.age OVERLAPS [4]', array("1", "2"));
// verify_query_result('$.age OVERLAPS [4, 5]', array("1", "2", "3"));
// verify_query_result('$.age OVERLAPS [1, 13]', array("1", "3"));
// verify_query_result('$.age OVERLAPS [13]', array("3"));

// verify_query_result('$.age NOT OVERLAPS [7]', array("1"));
// verify_query_result('$.age NOT OVERLAPS [16]', array());
// verify_query_result('$.age NOT OVERLAPS [4, 11]', array("3"));
// verify_query_result('$.age NOT OVERLAPS [1, 4, 11]', array("3"));

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
