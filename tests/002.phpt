--TEST--
mysqlx getTable/getTables
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function assert_table($name, $is_view = false) {
		global $tables;
		expect_true($tables[$name]->existsInDatabase());
		expect_eq($tables[$name]->getName(), $name);
		expect_eq($tables[$name]->isView(), $is_view);
	}

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
	create_test_view($nodeSession);

	$tables = $schema->getTables();
	assert_table($test_table_name);
	assert_table('test_table2');
	assert_table('test_table3');
	assert_table($test_view_name, true);

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
