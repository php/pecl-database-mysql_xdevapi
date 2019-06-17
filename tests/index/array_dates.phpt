--TEST--
mysqlx array/multi-value index dates
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

function add_world_cup_champion($country, $world_cup_championships) {
	global $coll;
	$world_cup_champion_json = '{"country": "' . $country
		. '", "world_cup_championships": ' . $world_cup_championships . '}';
	$coll->add($world_cup_champion_json)->execute();
}

function find_world_cup_champion($world_cup_final_date, $expected_country) {
	global $coll;
	$res = $coll->find(":world_cup_final IN $.world_cup_championships")
	    ->bind(['world_cup_final' => $world_cup_final_date])
		->execute();
	verify_one_value($res, "country", $expected_country, $world_cup_final_date);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

expect_create_index_with_name(
	'world_cup_champions_index',
	'{"fields": [{"field": "$.country", "type": "TEXT(64)", "array": false}]}');
expect_create_index(
	'{"fields": [{"field": "$.world_cup_championships", "type": "DATE", "array": true}]}');

add_world_cup_champion('Uruguay', '[ "1930-07-30", "1950-07-16"]');
add_world_cup_champion('Italy', '[ "1934-06-10", "1938-06-19", "1982-07-11", "2006-07-09" ]');
add_world_cup_champion('Germany', '[ "1954-07-04", "1974-07-07", "1990-07-08", "2014-07-13" ]');
add_world_cup_champion('Brazil', '[ "1958-06-29", "1962-06-17", "1970-06-21", "1994-07-17", "2002-06-30" ]');
add_world_cup_champion('England', '[ "1966-07-30" ]');
add_world_cup_champion('Argentina', '[ "1978-06-25", "1986-06-29" ]');
add_world_cup_champion('France', '[ "1998-07-12", "2018-07-15" ]');
add_world_cup_champion('Spain', '[ "2010-07-11" ]');

find_world_cup_champion('1950-07-16', 'Uruguay');
find_world_cup_champion('1938-06-19', 'Italy');
find_world_cup_champion('2006-07-09', 'Italy');
find_world_cup_champion('1954-07-04', 'Germany');
find_world_cup_champion('1990-07-08', 'Germany');
find_world_cup_champion('1962-06-17', 'Brazil');
find_world_cup_champion('1970-06-21', 'Brazil');
find_world_cup_champion('2002-06-30', 'Brazil');
find_world_cup_champion('1966-07-30', 'England');
find_world_cup_champion('1978-06-25', 'Argentina');
find_world_cup_champion('1998-07-12', 'France');
find_world_cup_champion('2010-07-11', 'Spain');

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
