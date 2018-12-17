--TEST--
mysqlx collection modify set vs replace
--SKIPIF--
--FILE--
<?php
require("connect.inc");

function run_query($query) {
	$res = $query->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	return $res->fetchAll();
}

function verify_record($record_data, $expected) {
	assert(is_even(count($expected)));
	$fields_count = count($expected) / 2;
	for ($i = 0; $i < $fields_count; ++$i) {
		$field_name_idx = $i * 2;
		$field_name = $expected[$field_name_idx];

		$field_value_idx = $field_name_idx + 1;
		$field_value = $expected[$field_value_idx];

		if ($field_value !== null) {
			expect_eq($record_data[$field_name], $field_value);
		} else {
			expect_false(array_key_exists($field_name, $record_data));
		}
	}
}

function verify_query($query_find, $expected) {
	$data = run_query($query_find);
	$record_data = $data[0];
	verify_record($record_data, $expected);
}

function modify_replace($field, $value) {
	global $coll;
	$query_replace = $coll->modify('name like :name');
	$query_replace = $query_replace->bind(['name' => 'Antonella']);
	$query_replace = $query_replace->replace($field, $value);
	$query_replace->execute();
}

function modify_set($field, $value) {
	global $coll;
	$query_set = $coll->modify('name like :name');
	$query_set = $query_set->bind(['name' => 'Antonella'])->limit(1);
	$query_set = $query_set->set($field, $value);
	$query_set->execute();
}

// -----

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

fill_db_collection($coll);

$query_find = $coll->find('name like :name');
$query_find = $query_find->bind(['name' => 'Antonella']);
verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 42,
		'job', 'Studente'));

// 'replace' can only overwrite existing field, ...
modify_replace('age', 21);

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 21,
		'job', 'Studente'));

// ...although should NOT add new field 'city'
modify_replace('city', 'Katowice');

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 21,
		'job', 'Studente',
		'city', null));

// 'set' can overwrite existing field, or add a brand new one
// in this case it should add new field 'city'
modify_set('city', 'Katowice');

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 21,
		'job', 'Studente',
		'city', 'Katowice'));

// now 'replace' can overwrite 'city', because the field does exist
modify_replace('city', 'Gdansk');

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 21,
		'job', 'Studente',
		'city', 'Gdansk'));

// use 'set' to overwrite existing fields - 'city', ...
modify_set('city', 'Munchen');

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', 21,
		'job', 'Studente',
		'city', 'Munchen'));

// ...and 'age', also overwriting its type from int to string
modify_set('age', '33');

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', '33',
		'job', 'Studente',
		'city', 'Munchen'));

// 'replace' can also change type of existing field
modify_replace('job', true);

verify_query(
	$query_find,
	array(
		'name', 'Antonella',
		'age', '33',
		'job', true,
		'city', 'Munchen'));

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
