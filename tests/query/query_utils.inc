<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

function compare_result($result, $expected_result, $field) {
	$result_count = count($result);
	expect_eq($result_count, count($expected_result), 'record count in result');

	for ($i = 0; $i < $result_count; ++$i) {
		expect_eq($result[$i][$field], $expected_result[$i], '$i = '.$i);
	}
}

function verify_collection_query_result($criteria, $field, $sort_by, $expected_result) {
	global $coll;
	$query = $coll->find($criteria)->fields($field)->sort($sort_by)->execute();
	$result = $query->fetchAll();
	compare_result($result, $expected_result, $field);
}

function verify_table_query_result($select_column, $criteria, $order_by, $expected_result) {
	global $table;
	$query = $table->select($select_column)->where($criteria)->orderBy($order_by)->execute();
	$result = $query->fetchAll();
	compare_result($result, $expected_result, $select_column);
}

?>
