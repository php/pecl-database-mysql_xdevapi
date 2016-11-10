--TEST--
mysqlx basic executeSql
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);

	$nodeSession->executeSql("drop table if exists $db.test_table");

	$nodeSession->executeSql("create table if not exists $db.test_table (name text, age int , job text)");
	try {
		$nodeSession->executeSql("create table $db.test_table (name text, age int, job text)");
	} catch(Exception $ex) {
		print "Exception!".PHP_EOL;
	}

	$nodeSession->executeSql("insert into $db.test_table values ('Marco', 25, 'Programmer')");
	$nodeSession->executeSql("insert into $db.test_table values ('Luca', 39, 'Student')");
	$sql = $nodeSession->executeSql("insert into $db.test_table values ('Antonio', 66, 'Dentist'),('Marcello',19,'Studente')");

	expect_eq($sql->getAffectedItemsCount(), 2);
	expect_eq($sql->hasData(), false);

	expect_eq($sql->getColumnCount(), false);
	expect_eq($sql->getColumnNames(), false);
	expect_eq($sql->getColumns(), false);

	expect_eq($sql->getWarningCount(), 0);
	expect_eq($sql->getWarnings(), false);

	$sql = $nodeSession->executeSql("select * from $db.test_table");

	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->hasData(), true);

	expect_eq($sql->getColumnNames()[0], 'name');
	expect_eq($sql->getColumnNames()[1], 'age');
	expect_eq($sql->getColumnNames()[2], 'job');

	expect_eq($sql->getWarningCount(), 0);
	expect_eq($sql->getWarnings(), false);

	$expected_names = array('name','age','job');

	// I know, I shall probably check all those fields..
	for($i = 0 ; $i < 3 ; $i++ ) {
		expect_eq($sql->getColumns()[$i]->name, $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->original_name, $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->table, 'test_table');
		expect_eq($sql->getColumns()[$i]->original_table, 'test_table');
		expect_eq($sql->getColumns()[$i]->schema, $db);
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
Exception!
done!%A
