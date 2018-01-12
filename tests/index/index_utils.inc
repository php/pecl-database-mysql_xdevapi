<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

$Default_index_name = 'myIndex';

function expect_index($index_name, $exists, $stack_frame_depth = 2) {
	global $db;
	global $nodeSession;

	$query = "SHOW INDEX FROM test_collection FROM $db WHERE Key_name='$index_name'";
	$res = $nodeSession->executeSql($query);
	expect_eq($res->hasData(), $exists, 'exists index '.$index_name, $stack_frame_depth);
}

// -------

function assert_index($index_name, $stack_frame_depth = 3) {
	expect_index($index_name, true, $stack_frame_depth + 1);

	global $coll;
	expect_true($coll->dropIndex($index_name), $index_name, $stack_frame_depth);

	expect_index($index_name, false, $stack_frame_depth + 1);
}

function expect_create_index_with_name($index_name, $index_def) {
	global $coll;
	$coll->createIndex($index_name, $index_def);
	assert_index($index_name);
}

function expect_create_index($index_def) {
	global $Default_index_name;
	expect_create_index_with_name($Default_index_name, $index_def);
}

// -------

function expect_fail_index_with_name($index_name, $index_def) {
	global $coll;

	$stack_frame_depth = 3;
	try {
		$coll->createIndex($index_name, $index_def);
		test_step_failed($index_name, $stack_frame_depth);
	} catch(Exception $e) {
		test_step_ok();
	}

	expect_index($index_name, false, $stack_frame_depth + 1);
}

function expect_fail_index($index_def) {
	global $Default_index_name;
	expect_fail_index_with_name($Default_index_name, $index_def);
}

// -------

function create_index($index_def) {
	global $Default_index_name;
	global $coll;
	$coll->createIndex($Default_index_name, $index_def);
	expect_index($Default_index_name, true, $stack_frame_depth + 1);
}

function drop_index($exists) {
	global $Default_index_name;
	global $coll;
	$stack_frame_depth = 2;
	expect_index($Default_index_name, $exists, $stack_frame_depth + 1);
	expect_eq($coll->dropIndex($Default_index_name), $exists, 'drop_index ' + $Default_index_name, $stack_frame_depth);
}

?>
