--TEST--
mysqlx select / fetch
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	fill_db_table_use_dup();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable('test_table');

	$res = $table->select(['name','age'])->where('name like \'P%\' or name like\'C%\'')
		->execute()->fetchAll();
	expect_eq(count($res), 6);
	$res = $table->select(['name','age'])->where('name like :name and age > :age')
		->bind(['name' => 'Tierney', 'age' => 34])->orderBy('age desc')->execute();
	$val = $res->fetchOne();
	expect_eq($val['name'], 'Tierney');
	expect_eq($val['age'], 46);
	$val = $res->fetchOne();
	expect_eq($val['name'], 'Tierney');
	expect_eq($val['age'], 39);

	$res = $table->select(['name','age'])->where('name in (\'Cassidy\',\'Polly\')')
		->orderBy(['age desc','name asc'])->execute()->fetchAll();
	expect_eq(count($res), 5);
	expect_eq($res[0]['name'], 'Cassidy');
	expect_eq($res[0]['age'], 34);
	expect_eq($res[1]['name'], 'Polly');
	expect_eq($res[1]['age'], 34);

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
