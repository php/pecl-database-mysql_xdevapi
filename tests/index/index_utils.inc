<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

$Default_index_name = 'myIndex';

function assert_field_traits($fields, $expected_fields_traits = null) {
	if (!$expected_fields_traits) return;

	$index_fields = $fields->fetchAll();
	$fields_count = count($index_fields);

	$expected_fields_count = count($expected_fields_traits);
	expect_eq($fields_count, $expected_fields_count, 'index fields count');

	for ($i = 0; $i < $expected_fields_count; ++$i) {
		$field_traits = $expected_fields_traits[$i];

		expect_eq($index_fields[$i]['Seq_in_index'], $i + 1, 'Seq_in_index for '.$i);
		expect_eq($index_fields[$i]['Key_name'], $field_traits[0], 'Key_name for '.$i);
		expect_eq($index_fields[$i]['Non_unique'], $field_traits[1], 'Non_unique for '.$i);
		expect_eq($index_fields[$i]['Null'], $field_traits[2], 'Null for '.$i);
	}
}

function expect_index($index_name, $exists, $expected_fields_traits = null) {
	global $db;
	global $session;

	$query = "SHOW INDEX FROM test_collection FROM $db WHERE Key_name='$index_name'";
	$res = $session->sql($query)->execute();
	expect_eq($res->hasData(), $exists, 'exists index '.$index_name);

	if ($expected_fields_traits) assert_field_traits($res, $expected_fields_traits);
}

// -------

function assert_index($index_name, $expected_fields_traits = null) {
	expect_index($index_name, true, $expected_fields_traits);

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

function expect_create_multi_index_with_name($index_name, $index_def, $expected_fields_traits) {
	global $coll;
	$coll->createIndex($index_name, $index_def);
	assert_index($index_name, $expected_fields_traits);
}

function expect_create_multi_index($index_def, $expected_fields_traits) {
	global $Default_index_name;
	expect_create_multi_index_with_name($Default_index_name, $index_def, $expected_fields_traits);
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
