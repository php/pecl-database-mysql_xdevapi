--TEST--
table [NOT] OVERLAPS operator on json string list
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/query_utils.inc");

function verify_query_result($criteria, $expected_result) {
	verify_table_query_result("idx", $criteria, "idx", $expected_result);
}

$session = create_test_db(null, "idx int, names json");
$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);

$table->insert('idx', 'names')->values(
	[1, '["Lech", "Jaroslaw", "Donald", "Beata", "Mateusz"]'],
	[2, '["Jaroslaw", "Donald", "Elzbieta", "Pawel"]'],
	[3, '["Janusz", "Beata", "Mateusz", "Wlodzimierz"]'],
	[4, '["Mateusz", "Teresa", "Antoni"]'],
	[5, '["Donald", "Grzegorz", "Ewa", "Vincent", "Slawomir"]']
	)->execute();

verify_query_result('["Slawomir", "Lech", "Vincent"] overlaps names', array(1, 5));
verify_query_result('["Mateusz", "Teresa", "Leszek"] overlaps names', array(1, 3, 4));
verify_query_result('["Aleksander", "Lech", "Andrzej"] overlaps names', array(1));
verify_query_result('["Lech", "Andrzej", "Jan"] overlaps names', array(1));
verify_query_result('["Donald"] overlaps names', array(1, 2, 5));

verify_query_result('["Beata", "Jaroslaw"] not overlaps names', array(4, 5));
verify_query_result('["Andrzej"] not overlaps names', array(1, 2, 3, 4, 5));
verify_query_result('["Donald", "Maria"] not overlaps names', array(3, 4));
verify_query_result('["Mateusz", "Ryszard"] not overlaps names', array(2, 5));
verify_query_result('["Jaroslaw", "Mateusz", "Ewa"] not overlaps names', array());

verify_query_result('names OVERLAPS ["Lech", "Donald"]', array(1, 2, 5));
verify_query_result('names OVERLAPS ["Lech", "Jaroslaw"]', array(1, 2));
verify_query_result('names OVERLAPS ["Mateusz", "Pawel", "Wlodzimierz"]', array(1, 2, 3, 4));
verify_query_result('names OVERLAPS ["Pawel", "Vincent", "Wlodzimierz"]', array(2, 3, 5));
verify_query_result('names OVERLAPS ["Andrzej", "Jan", "Marek"]', array());

verify_query_result('names NOT OVERLAPS ["Andrzej", "Jaroslaw", "Mateusz"]', array(5));
verify_query_result('names NOT OVERLAPS ["Donald", "Ewa", "Grzegorz"]', array(3, 4));
verify_query_result('names NOT OVERLAPS ["Pawel", "Wlodzimierz", "Grzegorz"]', array(1, 4));
verify_query_result('names NOT OVERLAPS ["Mateusz", "Lech", "Grzegorz"]', array(2));
verify_query_result('names NOT OVERLAPS ["Beata", "Elzbieta", "Teresa", "Ewa"]', array());

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/query_utils.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
