--TEST--
mysqlx getTable/getTables
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("wrong_table");

	expect_false($table->existsInDatabase());

	try {
		$table->insert(["name", "age"])->values(["Jackie", 256])->execute();
		test_step_failed();
	} catch(Exception $e) {
		//expected exception
	}
	expect_false($schema->getTable(""));

	$nodeSession->executeSql("create table $db.test_table2(job text, experience int, uuid int)");
	$nodeSession->executeSql("create table $db.test_table3(name text, surname text)");

	$tables = $schema->getTables();
	expect_true($tables['test_table']->existsInDatabase());
	expect_eq($tables['test_table']->getName(), 'test_table');
	expect_true($tables['test_table2']->existsInDatabase());
	expect_eq($tables['test_table2']->getName(), 'test_table2');
	expect_true($tables['test_table3']->existsInDatabase());
	expect_eq($tables['test_table3']->getName(), 'test_table3');

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
