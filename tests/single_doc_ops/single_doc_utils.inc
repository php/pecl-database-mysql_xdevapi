<?php

require_once(__DIR__."/../connect.inc");

// ----------------------------------------------------------------------

function verify_result($result, $id, $items_count) {
	expect_eq($result->getAffectedItemsCount(), $items_count, $id." affected items");
	expect_eq($result->getWarningsCount(), 0, $id." warnings");
}

// ----------------------------------------------------------------------

function verify_doc($doc, $id, $name, $age, $job) {
	expect_eq_id($doc["_id"], $id, $id);
	expect_eq($doc["name"], $name, $id);
	expect_eq($doc["age"], $age, $id);
	expect_eq($doc["job"], $job, $id);
}

function expect_doc($id, $name, $age, $job) {
	global $coll;
	$doc = $coll->getOne($id);
	verify_doc($doc, $id, $name, $age, $job);
}


function verify_modified_doc($doc, $id, $name, $surname, $info) {
	expect_eq_id($doc["_id"], $id, $id);
	expect_eq($doc["name"], $name, $id);
	expect_eq($doc["surname"], $surname, $id);
	expect_eq($doc["info"], $info, $id);
	expect_null($doc["age"], $id);
	expect_null($doc["job"], $id);
}

function expect_modified_doc($id, $name, $surname, $info) {
	global $coll;
	$doc = $coll->getOne($id);
	verify_modified_doc($doc, $id, $name, $surname, $info);
}


function verify_empty_doc($doc, $id) {
	expect_eq_id($doc["_id"], $id);
	expect_false(array_key_exists("name", $doc), $id);
	expect_false(array_key_exists("age", $doc), $id);
	expect_false(array_key_exists("job", $doc), $id);
	expect_false(array_key_exists("surname", $doc), $id);
	expect_false(array_key_exists("info", $doc), $id);
}

function expect_empty_doc($id) {
	global $coll;
	$doc = $coll->getOne($id);
	verify_empty_doc($doc, $id);
}


function verify_non_empty_doc($doc, $id) {
	expect_eq_id($doc["_id"], $id, $id." non-empty id");
	expect_true(array_key_exists("name", $doc), $id." non-empty name");
	expect_true(array_key_exists("age", $doc), $id." non-empty age");
	expect_true(array_key_exists("job", $doc), $id." non-empty job");
}

function expect_non_empty_doc($id) {
	global $coll;
	$doc = $coll->getOne($id);
	verify_non_empty_doc($doc, $id);
}


function expect_null_doc($id) {
	global $coll;
	$doc = $coll->getOne($id);
	expect_null($doc, $id);
}

// ----------------------------------------------------------------------

function replace_empty_doc($add_id, $replace_id, $empty_doc) {
	global $coll;

	expect_null_doc($add_id);
	$res = $coll->replaceOne($add_id, $empty_doc);
	verify_result($res, $add_id, 0);
	expect_null_doc($add_id);

	expect_non_empty_doc($replace_id);
	$res = $coll->replaceOne($replace_id, $empty_doc);
	verify_result($res, $replace_id, 1);
	expect_empty_doc($replace_id);
}

function replace_invalid_doc($add_id, $replace_id, $invalid_doc) {
	global $coll;

	expect_null_doc($add_id);
	try {
		$coll->replaceOne($add_id, $invalid_doc);
		test_step_failed();
	} catch(Exception $e) {
		test_step_ok();
	}
	expect_null_doc($add_id);

	expect_non_empty_doc($replace_id);
	try {
		$coll->replaceOne($replace_id, $invalid_doc);
		test_step_failed();
	} catch(Exception $e) {
		test_step_ok();
	}
	expect_non_empty_doc($replace_id);
}

// ----------------------------------------------------------------------

function add_or_replace_empty_doc($add_id, $replace_id, $empty_doc) {
	global $coll;

	expect_null_doc($add_id);
	$res = $coll->addOrReplaceOne($add_id, $empty_doc);
	verify_result($res, $add_id, 1);
	expect_empty_doc($add_id);

	expect_non_empty_doc($replace_id);
	$res = $coll->addOrReplaceOne($replace_id, $empty_doc);
	verify_result($res, $replace_id, 2);
	expect_empty_doc($replace_id);
}

function add_or_replace_invalid_doc($add_id, $replace_id, $invalid_doc) {
	global $coll;

	expect_null_doc($add_id);
	try {
		$coll->addOrReplaceOne($add_id, $invalid_doc);
		test_step_failed();
	} catch(Exception $e) {
		test_step_ok();
	}
	expect_null_doc($add_id);

	expect_non_empty_doc($replace_id);
	try {
		$coll->addOrReplaceOne($replace_id, $invalid_doc);
		test_step_failed();
	} catch(Exception $e) {
		test_step_ok();
	}
	expect_non_empty_doc($replace_id);
}

?>
