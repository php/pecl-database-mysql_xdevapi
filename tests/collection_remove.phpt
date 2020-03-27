--TEST--
mysqlx collection remove
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$coll->remove('age > :age_from and age < :age_to')->bind(['age_from' => 20, 'age_to' => 50])->limit(7)->execute();

	$coll->remove('true')->sort('age desc')->limit(2)->execute();
	$coll->modify('_id in (1,13,5,7)')->unset(['age'])->execute();
	$coll->remove("job in ('Barista', 'Programmatore', 'Ballerino', 'Programmatrice')")->limit(5)->sort(['age desc', 'name asc'])->execute();

	$res = $coll->find()->execute()->fetchAll();
	expect_eq($res[0]['job'],'Programmatore');
	expect_eq($res[0]['name'],'Marco');
	expect_eq($res[1]['job'],'Programmatore');
	expect_eq($res[1]['name'],'Carlo');

	// fails expected due to empty or incorrect search-condition
	function check_incorrect_condition($condition) {
		global $coll;
		try {
			$coll->remove($condition);
			test_step_failed();
		} catch(Exception $e) {
			test_step_ok();
		}
	}

	check_incorrect_condition('');
	check_incorrect_condition(' ');
	check_incorrect_condition('@ incorrect $ condition &');

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
