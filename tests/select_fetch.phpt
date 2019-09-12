--TEST--
mysqlx select / fetch
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	fill_db_table_use_dup();

	$schema = $session->getSchema($db);
	$table = $schema->getTable('test_table');

        $res = $table->select('name','age')->where("name like 'P%' or name like'C%'")
		->execute()->fetchAll();
	expect_eq(count($res), 6);
	$res = $table->select('name','age')->where('name like :name and age > :age')
		->bind(['name' => 'Tierney', 'age' => 34])->orderBy('age desc')->execute();

	$val = $res->fetchOne();
	expect_eq($val['name'], 'Tierney');
	expect_eq($val['age'], 46);
	$val = $res->fetchOne();
	expect_eq($val['name'], 'Tierney');
	expect_eq($val['age'], 39);

	$res = $table->select(['name','age'])->where("name in ('Cassidy','Polly')")
		->orderBy(['age desc','name asc'])->execute();

	expect_eq($res->getColumnsCount(), 2);
	$columns = $res->getColumnNames();
	expect_eq($columns[0], 'name');
	expect_eq($columns[1], 'age');

	$res = $res->fetchAll();

	expect_eq(count($res), 5);
	expect_eq($res[0]['name'], 'Cassidy');
	expect_eq($res[0]['age'], 34);
	expect_eq($res[1]['name'], 'Polly');
	expect_eq($res[1]['age'], 34);

        //Verify aliases
	$res = $table->select('name as nm','age as ag')->where('age > 34')->execute();
	$data = $res->fetchAll();
	expect_eq(count($data),2);
	expect_eq($data[0]['nm'],'Tierney');
	expect_eq($data[0]['ag'],46);
	expect_eq($data[1]['nm'],'Tierney');
	expect_eq($data[1]['ag'],39);

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
