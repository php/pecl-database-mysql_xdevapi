--TEST--
mysqlx collection single document operations
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");

	// ----------------------------------------------------------------------

	function verify_result($result, $id, $items_count, $stack_frame_depth = 2) {
		expect_eq($result->getAffectedItemsCount(), $items_count, $id." affected items", $stack_frame_depth);
		expect_eq($result->getWarningCount(), 0, $id." warnings", $stack_frame_depth);
	}

	// ----------------------------------------------------------------------

	function verify_doc($doc, $id, $name, $age, $job, $stack_frame_depth = 2) {
		expect_eq($doc["_id"], $id, $id, $stack_frame_depth);
		expect_eq($doc["name"], $name, $id, $stack_frame_depth);
		expect_eq($doc["age"], $age, $id, $stack_frame_depth);
		expect_eq($doc["job"], $job, $id, $stack_frame_depth);
	}

	function expect_doc($id, $name, $age, $job) {
		global $coll;
		$stack_frame_depth = 3;
		$doc = $coll->getOne($id);
		verify_doc($doc, $id, $name, $age, $job, $stack_frame_depth);
	}


	function verify_modified_doc($doc, $id, $name, $surname, $info, $stack_frame_depth = 2) {
		expect_eq($doc["_id"], $id, $id, $stack_frame_depth);
		expect_eq($doc["name"], $name, $id, $stack_frame_depth);
		expect_eq($doc["surname"], $surname, $id, $stack_frame_depth);
		expect_eq($doc["info"], $info, $id, $stack_frame_depth);
		expect_null($doc["age"], $id, $stack_frame_depth);
		expect_null($doc["job"], $id, $stack_frame_depth);
	}

	function expect_modified_doc($id, $name, $surname, $info) {
		global $coll;
		$stack_frame_depth = 3;
		$doc = $coll->getOne($id);
		verify_modified_doc($doc, $id, $name, $surname, $info, $stack_frame_depth);
	}


	function verify_empty_doc($doc, $id, $stack_frame_depth = 2) {
		expect_eq($doc["_id"], $id, $stack_frame_depth);
		expect_false(array_key_exists("name", $doc), $id, $stack_frame_depth);
		expect_false(array_key_exists("age", $doc), $id, $stack_frame_depth);
		expect_false(array_key_exists("job", $doc), $id, $stack_frame_depth);
		expect_false(array_key_exists("surname", $doc), $id, $stack_frame_depth);
		expect_false(array_key_exists("info", $doc), $id, $stack_frame_depth);
	}

	function expect_empty_doc($id, $stack_frame_depth = 3) {
		global $coll;
		$doc = $coll->getOne($id);
		verify_empty_doc($doc, $id, $stack_frame_depth);
	}


	function verify_non_empty_doc($doc, $id, $stack_frame_depth = 2) {
		expect_eq($doc["_id"], $id, $id." non-empty id", $stack_frame_depth);
		expect_true(array_key_exists("name", $doc), $id." non-empty name", $stack_frame_depth);
		expect_true(array_key_exists("age", $doc), $id." non-empty age", $stack_frame_depth);
		expect_true(array_key_exists("job", $doc), $id." non-empty job", $stack_frame_depth);
	}

	function expect_non_empty_doc($id, $stack_frame_depth = 3) {
		global $coll;
		$doc = $coll->getOne($id);
		verify_non_empty_doc($doc, $id, $stack_frame_depth);
	}


	function expect_null_doc($id, $stack_frame_depth = 2) {
		global $coll;
		$doc = $coll->getOne($id);
		expect_null($doc, $id, $stack_frame_depth);
	}

	// ----------------------------------------------------------------------

	function replace_empty_doc($add_id, $replace_id, $empty_doc) {
		global $coll;
		$stack_frame_depth = 4;

		expect_null_doc($add_id, $stack_frame_depth);
		$res = $coll->replaceOne($add_id, $empty_doc);
		verify_result($res, $add_id, 0, $stack_frame_depth);
		expect_null_doc($add_id, $stack_frame_depth);

		expect_non_empty_doc($replace_id, $stack_frame_depth);
		$res = $coll->replaceOne($replace_id, $empty_doc);
		verify_result($res, $replace_id, 1, $stack_frame_depth);
		expect_empty_doc($replace_id);
	}

	function replace_invalid_doc($add_id, $replace_id, $invalid_doc) {
		global $coll;
		$stack_frame_depth = 4;

		expect_null_doc($add_id, $stack_frame_depth);
		try {
			$coll->replaceOne($add_id, $invalid_doc);
			test_step_failed($stack_frame_depth);
		} catch(Exception $e) {
			test_step_ok();
		}
		expect_null_doc($add_id, $stack_frame_depth);

		expect_non_empty_doc($replace_id, $stack_frame_depth);
		try {
			$coll->replaceOne($replace_id, $invalid_doc);
			test_step_failed($stack_frame_depth);
		} catch(Exception $e) {
			test_step_ok();
		}
		expect_non_empty_doc($replace_id, $stack_frame_depth);
	}

	// ----------------------------------------------------------------------

	function add_or_replace_empty_doc($add_id, $replace_id, $empty_doc) {
		global $coll;
		$stack_frame_depth = 4;

		expect_null_doc($add_id, $stack_frame_depth);
		$res = $coll->addOrReplaceOne($add_id, $empty_doc);
		verify_result($res, $add_id, 1, $stack_frame_depth);
		expect_empty_doc($add_id, $stack_frame_depth);

		expect_non_empty_doc($replace_id, $stack_frame_depth);
		$res = $coll->addOrReplaceOne($replace_id, $empty_doc);
		verify_result($res, $replace_id, 2, $stack_frame_depth);
		expect_empty_doc($replace_id);
	}

	function add_or_replace_invalid_doc($add_id, $replace_id, $invalid_doc) {
		global $coll;
		$stack_frame_depth = 4;

		expect_null_doc($add_id, $stack_frame_depth);
		try {
			$coll->addOrReplaceOne($add_id, $invalid_doc);
			test_step_failed($stack_frame_depth);
		} catch(Exception $e) {
			test_step_ok();
		}
		expect_null_doc($add_id, $stack_frame_depth);

		expect_non_empty_doc($replace_id, $stack_frame_depth);
		try {
			$coll->addOrReplaceOne($replace_id, $invalid_doc);
			test_step_failed($stack_frame_depth);
		} catch(Exception $e) {
			test_step_ok();
		}
		expect_non_empty_doc($replace_id, $stack_frame_depth);
	}

	// ----------------------------------------------------------------------

	$nodeSession = create_test_db();
	$coll = fill_test_collection(true);

	// ----------------------------------------------------------------------
	// getOne
	$doc = $coll->getOne(0);
	expect_null($doc);

	$doc = $coll->getOne(5);
	verify_doc($doc, 5, "Carlo", '25', "Programmatore");

	$doc = $coll->getOne('31');
	expect_null($doc);

	$doc = $coll->getOne('13');
	verify_doc($doc, 13, "Alessandra", 15, "Barista");

	// ----------------------------------------------------------------------
	// replaceOne
	$doc = $coll->getOne(16);
	verify_doc($doc, '16', "Leonardo", 23, "Programmatore");
	$doc["age"] = "55";
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

	// ----------------------------------------------------------------------
	// addOrReplaceOne

	$doc = $coll->getOne(13);
	verify_doc($doc, 13, "Alessandra", 15, "Barista");
	$doc["name"] = "Alessandro";
	$doc["age"] = "86";

	expect_null_doc('87');
	$res = $coll->addOrReplaceOne(87, $doc);
	verify_result($res, 87, 1);
	expect_doc('87', "Alessandro", 86, "Barista");
	expect_doc(13, "Alessandra", 15, "Barista");

	$doc["age"] = "76";
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

	// ----------------------------------------------------------------------
	// removeOne

	expect_doc(25, "Sergio", 65, "Direzione aziendale");
	$res = $coll->removeOne('25');
	verify_result($res, '25', 1);
	expect_null_doc(25);

	$res = $coll->removeOne('25');
	verify_result($res, null, 0);
	expect_null_doc(25);

	expect_null_doc(1001);
	$res = $coll->removeOne('1001');
	verify_result($res, null, 0);
	expect_null_doc(1001);

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
