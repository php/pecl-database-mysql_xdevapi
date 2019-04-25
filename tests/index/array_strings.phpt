--TEST--
mysqlx array/multi-value index strings
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

function add_F1_team($team, $staff) {
	global $coll;
	$team_F1_json = '{"team": "' . $team . '", "staff": ' . $staff . '}';
	$coll->add($team_F1_json)->execute();
}

function find_staff_member($staff_member, $expected_team) {
	global $coll;
	$res = $coll->find(":staff_member IN $.staff")
		->bind(['staff_member' => $staff_member])
		->execute();
	verify_one_value($res, "team", $expected_team, $staff_member);
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

expect_create_index_with_name(
	'F1_teams_index',
	'{"fields": [{"field": "$.team", "type": "TEXT(64)", "array": false}]}');
expect_create_index(
	'{"fields": [{"field": "$.staff", "type": "CHAR(128)", "array": true}]}');

add_F1_team('Mercedes',
	'[ "Toto Wolff", "Niki Lauda", "Lewis Hamilton", "Valtteri Bottas" ]');

add_F1_team("Ferrari",
	'[ "Maurizio Arrivabene", "Sebastian Vettel", "Kimi Raikkonen" ]');

add_F1_team("Red Bull",
	'[ "Christian Horner", "Daniel Ricciardo", "Max Verstappen" ]');

add_F1_team("Renault",
	'[ "Cyril Abiteboul", "Nico Hulkenberg", "Carlos Sainz" ]');

add_F1_team("McLaren",
	'[ "Eric Boullier", "Fernando Alonso", "Stoffel Vandoorne" ]');

add_F1_team("Williams",
	'[ "Frank Williams", "Lance Stroll", "Sergey Sirotkin", "Robert Kubica" ]');

find_staff_member('Robert Kubica', 'Williams');
find_staff_member('Michael Schumacher', null);
find_staff_member('Fernando Alonso', 'McLaren');
find_staff_member('Ayrton Senna', null);
find_staff_member('Daniel Ricciardo', 'Red Bull');
find_staff_member('Nigel Mansell', null);
find_staff_member('Lewis Hamilton', 'Mercedes');
find_staff_member('Jackie Stewart', null);
find_staff_member('Kimi Raikkonen', 'Ferrari');
find_staff_member('Alain Prost', null);
find_staff_member('Niki Lauda', 'Mercedes');
find_staff_member('Juan Manuel Fangio', null);

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
