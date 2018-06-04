--TEST--
mysqlx basic executeSql
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);

	$session->executeSql("drop table if exists $db.test_table");

	$session->executeSql("create table if not exists $db.test_table (name text, age int , job text)");
	try {
		$session->executeSql("create table $db.test_table (name text, age int, job text)");
	} catch(Exception $e) {
		expect_eq($e->getMessage(),
			'[HY000] Couldn\'t fetch data');
		expect_eq($e->getCode(), 10000);
		print "Exception!".PHP_EOL;
	}

	$session->executeSql("insert into $db.test_table values ('Marco', 25, 'Programmer')");
	$session->executeSql("insert into $db.test_table values ('Luca', 39, 'Student')");
	$sql = $session->executeSql("insert into $db.test_table values ('Antonio', 66, 'Dentist'),('Marcello',19,'Studente')");

	expect_eq($sql->getAffectedItemsCount(), 2);
	expect_eq($sql->hasData(), false);

	expect_eq($sql->getColumnCount(), false);
	expect_eq($sql->getColumnNames(), false);
	expect_eq($sql->getColumns(), false);

	expect_eq($sql->getWarningsCount(), 0);
	expect_eq($sql->getWarnings(), false);

	$sql = $session->executeSql("select * from $db.test_table");

	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->hasData(), true);

	expect_eq($sql->getColumnNames()[0], 'name');
	expect_eq($sql->getColumnNames()[1], 'age');
	expect_eq($sql->getColumnNames()[2], 'job');

	expect_eq($sql->getWarningsCount(), 0);
	expect_eq($sql->getWarnings(), false);

	$expected_names = array('name','age','job');
	$expected_lengths = array( 65535, 11, 65535 );
	$expected_collations = array( 255, 0, 255 );

	// I know, I shall probably check all those fields..
	for($i = 0 ; $i < 3 ; $i++ ) {
		expect_eq($sql->getColumns()[$i]->name, $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->original_name, $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->table, 'test_table');
		expect_eq($sql->getColumns()[$i]->original_table, 'test_table');
		expect_eq($sql->getColumns()[$i]->schema, $db);
		expect_eq($sql->getColumns()[$i]->catalog, "def");
		expect_eq($sql->getColumns()[$i]->content_type, 0);
		expect_eq($sql->getColumns()[$i]->flags, 0);
		expect_eq($sql->getColumns()[$i]->length, $expected_lengths[$i]);
		expect_eq($sql->getColumns()[$i]->fractional_digits, 0);
		expect_eq($sql->getColumns()[$i]->collation, $expected_collations[$i]);

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
