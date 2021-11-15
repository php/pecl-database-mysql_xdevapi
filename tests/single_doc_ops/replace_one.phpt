--TEST--
mysqlx collection single doc ops - replaceOne
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/single_doc_utils.inc");

$session = create_test_db();
$coll = fill_test_collection(true);

// ----------------------------------------------------------------------
// replaceOne
$doc = $coll->getOne(16);
verify_doc($doc, '16', "Leonardo", 23, "Programmatore");
$doc["age"] = 55;
$doc["job"] = "SeniorProgrammatore";
$res = $coll->replaceOne(16, $doc);
verify_result($res, 16, 1);
expect_doc(16, "Leonardo", 55, "SeniorProgrammatore");

// replace doc with one created ad hoc
expect_doc(12, "Filippo", 31, "Spazzino");
$record = array(
	"name" => "Overwrite",
	"surname" => "Doc",
	"info" => "Test"
);
$res = $coll->replaceOne(12, $record);
verify_result($res, '12', 1);
expect_modified_doc(12, "Overwrite", "Doc", "Test");

// replace doc 15 with contents of 14
$doc = $coll->getOne(14);
verify_doc($doc, 14, "Massimo", 22, "Programmatore");
$res = $coll->replaceOne(15, $doc);
verify_result($res, 15, 1);
expect_doc(14, "Massimo", 22, "Programmatore");
expect_doc(15, "Massimo", 22, "Programmatore");

// trial of replace non-existing doc
$doc = $coll->getOne(20);
$res = $coll->replaceOne(99, $doc);
verify_result($res, null, 0);
expect_null_doc(99);

// replace with doc which contains the same _id
expect_doc('1', "Marco", 19, "Programmatore");
$res = $coll->replaceOne('1', '{"name": "Sakila", "age": 30, "job": "Radio", "_id": "1" }');
verify_result($res, 1, 1);
expect_doc('1', "Sakila", 30, "Radio");

// ignore existing _id in doc different than replaced one
expect_doc(2, "Lonardo", 59, "Paninaro");
$res = $coll->replaceOne('2', '{"name": "Johannes", "age": 25, "job": "Boss", "_id": "1" }');
verify_result($res, 2, 1);
expect_doc('2', "Johannes", 25, "Boss");
expect_doc('1', "Sakila", 30, "Radio");

// should work despite _id is not in doc
expect_doc('3', "Riccardo", 27, "Cantante");
$res = $coll->replaceOne('3', '{"name": "Filip", "age": 20, "job": "Programmatore"}');
verify_result($res, 3, 1);
expect_doc('3', "Filip", 20, "Programmatore");

// ignore non-existing _id in doc, different than replaced one
expect_doc('4', "Carlotta", 23, "Programmatrice");
$res = $coll->replaceOne('4', '{"name": "marines", "age": 65, "job": "Almost retired programmer", "_id": "77" }');
verify_result($res, 4, 1);
expect_doc('4', "marines", 65, "Almost retired programmer");
expect_null_doc(77);

// trial of replace non-existing doc with null doc
expect_null_doc(37);
$doc = $coll->getOne(37);
assert($doc == null);
expect_null_doc(99);
try {
	$coll->replaceOne(99, $doc);
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}
expect_null_doc(99);

// replace invalid doc
replace_invalid_doc(1000, 1, null);
replace_invalid_doc(1001, 2, '');
replace_invalid_doc(1002, 3, "");
replace_invalid_doc(1003, 4, []);

// replace empty doc
replace_empty_doc(500, 1, new class{});
replace_empty_doc(501, 2, '{}');
replace_empty_doc(502, 3, "{}");


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
