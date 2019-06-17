--TEST--
mysqlx array/multi-value index integers
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

function add_lottery_draw($draw, $date, $numbers) {
	global $coll;
	$lottery_draw_json = '{"draw": "' . $draw
		. '", "date": "' . $date
		. '", "numbers": ' . $numbers . '}';
	$coll->add($lottery_draw_json)->execute();
}

function find_draws_with_number($number, $expected_draw) {
	global $coll;
	$res = $coll->find(":draw_number IN $.numbers")
		->bind(['draw_number' => $number])
		->execute();
	verify_all_values($res, "draw", $expected_draw, $number);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

expect_create_index_with_name(
	'lottery_draws_index',
	'{"fields": [{"field": "$.draw", "type": "TEXT(64)", "array": false}]}');
expect_create_index(
	'{"fields": [{"field": "$.numbers", "type": "UNSIGNED INTEGER", "array": true}]}');

add_lottery_draw('draw 6423', '2018-01-06', '[ 3, 24, 40, 43, 65 ]');
add_lottery_draw('draw 6810', '2018-02-15', '[ 1, 2, 8, 15, 78 ]');
add_lottery_draw('draw 7803', '2018-03-12', '[ 3, 6, 12, 19, 78 ]');
add_lottery_draw('draw 8419', '2018-07-09', '[ 4, 8, 17, 43, 54 ]');
add_lottery_draw('draw 9708', '2018-10-20', '[ 20, 34, 43, 44, 55 ]');
add_lottery_draw('draw 10708', '2018-11-18', '[ 4, 11, 19, 23, 76 ]');

find_draws_with_number(24, 'draw 6423');
find_draws_with_number(3, ['draw 6423', 'draw 7803']);
find_draws_with_number(43, ['draw 6423', 'draw 8419', 'draw 9708']);
find_draws_with_number(1, 'draw 6810');
find_draws_with_number(78, ['draw 6810', 'draw 7803']);
find_draws_with_number(8, ['draw 6810', 'draw 8419']);
find_draws_with_number(12, 'draw 7803');
find_draws_with_number(4, ['draw 8419', 'draw 10708']);
find_draws_with_number(55, 'draw 9708');
find_draws_with_number(23, ['draw 10708']);

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
