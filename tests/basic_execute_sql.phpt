--TEST--
mysqlx basic execute SQL
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);

	$session->sql("drop table if exists $db.$test_table_name")->execute();

	$session->sql("create table if not exists $db.$test_table_name (name text, age int , job text)")->execute();
	try {
		$session->sql("create table $db.$test_table_name (name text, age int, job text)")->execute();
	} catch(Exception $e) {
		expect_eq($e->getMessage(),
			"[HY000] Couldn't fetch data");
		expect_eq($e->getCode(), 10000);
		print "Exception!".PHP_EOL;
	}

	$session->sql("insert into $db.$test_table_name values ('Marco', 25, 'Programmer')")->execute();
	$session->sql("insert into $db.$test_table_name values ('Luca', 39, 'Student')")->execute();
	$sql = $session->sql("insert into $db.$test_table_name values ('Antonio', 66, 'Dentist'),('Marcello',19,'Studente')")->execute();

	expect_eq($sql->getAffectedItemsCount(), 2);
	expect_eq($sql->hasData(), false);

	expect_eq($sql->getColumnsCount(), 0);
	expect_eq($sql->getColumnNames(), []);
	expect_eq($sql->getColumns(), []);

	expect_eq($sql->getWarningsCount(), 0);
	expect_eq($sql->getWarnings(), []);

	$sql = $session->sql("select * from $db.$test_table_name")->execute();

	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->hasData(), true);

	expect_eq($sql->getColumnNames()[0], 'name');
	expect_eq($sql->getColumnNames()[1], 'age');
	expect_eq($sql->getColumnNames()[2], 'job');

	expect_eq($sql->getWarningsCount(), 0);
	expect_eq($sql->getWarnings(), []);

	$expected_names = array('name','age','job');
	$expected_is_signed = array(false, true, false);
	$expected_types = array( MYSQLX_TYPE_BYTES, MYSQLX_TYPE_INT, MYSQLX_TYPE_BYTES );
	$expected_lengths = array( 65535, 11, 65535 );
	$expected_collations = array( 'utf8mb4_0900_ai_ci', null, 'utf8mb4_0900_ai_ci' );

	// I know, I shall probably check all those fields..
	for($i = 0 ; $i < 3 ; $i++ ) {
		expect_eq($sql->getColumns()[$i]->getColumnLabel(), $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->getColumnName(), $expected_names[$i]);
		expect_eq($sql->getColumns()[$i]->getTableLabel(), $test_table_name);
		expect_eq($sql->getColumns()[$i]->getTableName(), $test_table_name);
		expect_eq($sql->getColumns()[$i]->getSchemaName(), $db);
		expect_eq($sql->getColumns()[$i]->isNumberSigned(), $expected_is_signed[$i], 'isNumberSigned');
		expect_eq($sql->getColumns()[$i]->getType(), $expected_types[$i], 'getType');
		expect_eq($sql->getColumns()[$i]->isPadded(), false, 'isPadded');
		expect_eq($sql->getColumns()[$i]->getLength(), $expected_lengths[$i]);
		expect_eq($sql->getColumns()[$i]->getFractionalDigits(), 0);
		expect_eq($sql->getColumns()[$i]->getCollationName(), $expected_collations[$i]);
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
