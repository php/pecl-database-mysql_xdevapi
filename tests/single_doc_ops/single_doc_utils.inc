<?php

require_once(__DIR__."/../connect.inc");

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

?>
