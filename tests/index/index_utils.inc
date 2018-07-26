<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

$Default_index_name = 'myIndex';

function expect_index($index_name, $exists) {
	global $db;
	global $session;

	$query = "SHOW INDEX FROM test_collection FROM $db WHERE Key_name='$index_name'";
	$res = $session->executeSql($query);
	expect_eq($res->hasData(), $exists, 'exists index '.$index_name);
}

// -------

function assert_index($index_name) {
	expect_index($index_name, true);

	global $coll;
	expect_true($coll->dropIndex($index_name), $index_name);

	expect_index($index_name, false);
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

	try {
		$coll->createIndex($index_name, $index_def);
		test_step_failed($index_name);
	} catch(Exception $e) {
		test_step_ok();
	}

	expect_index($index_name, false);
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
	expect_index($Default_index_name, true);
}

function drop_index($exists) {
	global $Default_index_name;
	global $coll;
	expect_index($Default_index_name, $exists);
	expect_eq($coll->dropIndex($Default_index_name), $exists, 'drop_index ' + $Default_index_name);
}

// -------

function verify_value($result, $field_name, $expected_value, $hint) {
	if ($expected_value != null) {
		expect_eq_or_in($result[$field_name], $expected_value, $hint);
	} else {
		expect_null($result, $field_name . ' ' . $expected_value);
	}
}

function verify_one_value($result, $field_name, $expected_value, $hint) {
	verify_value($result->fetchOne(), $field_name, $expected_value, $hint);
}

function verify_all_values($result, $field_name, $expected_value, $hint) {
	$data = $result->fetchAll();
	for ($i = 0; $i < count($data); $i++) {
		verify_value($data[$i], $field_name, $expected_value, $hint);
	}
}

?>
