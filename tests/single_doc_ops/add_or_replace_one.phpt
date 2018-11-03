--TEST--
mysqlx collection single doc ops - addOrReplaceOne
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
// addOrReplaceOne

$doc = $coll->getOne(13);
verify_doc($doc, 13, "Alessandra", 15, "Barista");
$doc["name"] = "Alessandro";
$doc["age"] = 86;

expect_null_doc('87');
$res = $coll->addOrReplaceOne(87, $doc);
verify_result($res, 87, 1);
expect_doc('87', "Alessandro", 86, "Barista");
expect_doc(13, "Alessandra", 15, "Barista");

$doc["age"] = 76;
$res = $coll->addOrReplaceOne(13, $doc);
verify_result($res, 13, 2);
expect_doc('13', "Alessandro", 76, "Barista");
expect_doc('87', "Alessandro", 86, "Barista");

// add or replace doc with one created ad hoc
expect_doc(11, "Lucia", 47, "Barista");
$record = array(
	"name" => "Overwrite-or-add",
	"surname" => "Prepared-doc",
	"info" => "Test-case"
);
$res = $coll->addOrReplaceOne(11, $record);
verify_result($res, '11', 2);
expect_modified_doc(11, "Overwrite-or-add", "Prepared-doc", "Test-case");

$res = $coll->addOrReplaceOne(111, $record);
verify_result($res, '111', 1);
expect_modified_doc(111, "Overwrite-or-add", "Prepared-doc", "Test-case");

// replace doc 9 with contents of 8
expect_doc(9, "Monica", 35, "Ballerino");
$doc = $coll->getOne(8);
verify_doc($doc, 8, "Antonella", 42, "Studente");
$res = $coll->addOrReplaceOne(9, $doc);
verify_result($res, 9, 2);
expect_doc(8, "Antonella", 42, "Studente");
expect_doc(9, "Antonella", 42, "Studente");

$coll->addOrReplaceOne(109, $doc);
expect_doc(9, "Antonella", 42, "Studente");
expect_doc(109, "Antonella", 42, "Studente");

// add or replace doc for non-existing id
$doc = $coll->getOne(24);
expect_doc(24, "Enzo", 90, "Ferrari");
$res = $coll->addOrReplaceOne(224, $doc);
verify_result($res, 224, 1);
expect_doc(224, "Enzo", 90, "Ferrari");
expect_doc(24, "Enzo", 90, "Ferrari");


// replace with doc which contains the same _id
expect_doc('21', "Fabio", 59, "Pilota automobilistico");
$res = $coll->addOrReplaceOne('121', '{"name": "Sakila", "age": 30, "job": "Radio", "_id": "121" }');
verify_result($res, 121, 1);
expect_doc('121', "Sakila", 30, "Radio");

expect_doc('21', "Fabio", 59, "Pilota automobilistico");
$res = $coll->addOrReplaceOne('21', '{"name": "Sakila", "age": 30, "job": "Radio", "_id": "21" }');
verify_result($res, 21, 2);
expect_doc('21', "Sakila", 30, "Radio");

// ignore existing _id in doc, different than replaced one
expect_doc(22, "Gianluigi", 39, "Portiere");
$res = $coll->addOrReplaceOne('122', '{"name": "Johannes", "age": 25, "job": "Boss", "_id": "21" }');
verify_result($res, 122, 1);
expect_doc('122', "Johannes", 25, "Boss");
expect_doc('21', "Sakila", 30, "Radio");

expect_doc(22, "Gianluigi", 39, "Portiere");
$res = $coll->addOrReplaceOne('22', '{"name": "Johannes", "age": 25, "job": "Boss", "_id": "21" }');
verify_result($res, 22, 2);
expect_doc('22', "Johannes", 25, "Boss");
expect_doc('21', "Sakila", 30, "Radio");

// should work despite _id is not in doc
expect_doc(23, "Sophia", 82, "Attrice");
$res = $coll->addOrReplaceOne('123', '{"name": "Filip", "age": 20, "job": "Programmatore"}');
verify_result($res, 123, 1);
expect_doc('123', "Filip", 20, "Programmatore");

expect_doc(23, "Sophia", 82, "Attrice");
$res = $coll->addOrReplaceOne('23', '{"name": "Filip", "age": 20, "job": "Programmatore"}');
verify_result($res, 23, 2);
expect_doc('23', "Filip", 20, "Programmatore");

// ignore non-existing _id in doc, different than replaced one
expect_doc(24, "Enzo", 90, "Ferrari");
$res = $coll->addOrReplaceOne('124', '{"name": "marines", "age": 65, "job": "Almost retired programmer", "_id": "177" }');
verify_result($res, 124, 1);
expect_doc('124', "marines", 65, "Almost retired programmer");
expect_null_doc(177);

expect_doc(24, "Enzo", 90, "Ferrari");
$res = $coll->addOrReplaceOne('24', '{"name": "marines", "age": 65, "job": "Almost retired programmer", "_id": "177" }');
verify_result($res, 24, 2);
expect_doc('24', "marines", 65, "Almost retired programmer");
expect_null_doc(177);


// addOrReplace non-existing doc with null doc
expect_null_doc(43);
$doc = $coll->getOne(43);
assert($doc == null);
try {
	expect_null_doc(96);
	$res = $coll->addOrReplaceOne(96, $doc);
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}
expect_null_doc(96);

// addOrReplace invalid doc
add_or_replace_invalid_doc(2000, 21, null);
add_or_replace_invalid_doc(2001, 22, '');
add_or_replace_invalid_doc(2002, 23, "");
add_or_replace_invalid_doc(2003, 24, []);

// addOrReplace empty doc
add_or_replace_empty_doc(700, 21, new class{});
add_or_replace_empty_doc(701, 22, '{}');
add_or_replace_empty_doc(702, 23, "{}");

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
