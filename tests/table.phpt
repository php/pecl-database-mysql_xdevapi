--TEST--
mysqlx Table
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	fill_db_table();

	expect_eq($table->getName(), 'test_table');
	expect_eq($table->name, 'test_table');
	expect_eq($table->count(), 12);
	expect_true($table->existsInDatabase());

	try {
		$schema = $table->getSchema();
		expect_eq($schema->getName(),$db);
		expect_true($schema->existsInDatabase());
		$table2 = $schema->getTable('test_table');
		expect_eq($table2->getName(),'test_table');
		expect_true($table2->existsInDatabase());
	} catch(Exception $ex) {
	        test_step_failed();
	}

	$table = $schema->getTable("not_existing_table");
	expect_eq($table->getName(), 'not_existing_table');
	expect_eq($table->name, 'not_existing_table');
	expect_false($table->existsInDatabase());

	try {
		$schema = $table->count();
		test_step_failed();
	} catch(Exception $ex) {
		test_step_ok();
	}

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
