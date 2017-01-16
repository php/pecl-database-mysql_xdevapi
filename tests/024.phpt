--TEST--
	mysqlx sql simple
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = mysql_xdevapi\getNodeSession($connection_uri);
	$nodeSession->executeSql("create database $db");
	$nodeSession->executeSql("create table $db.test_table(name text, age tinyint)");

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

	fill_db_table();

	$sql = $nodeSession->sql("select * from $db.test_table")->execute();
	expect_true($sql->hasData());
	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->getWarningCount(), 0);
	expect_eq($sql->getColumnCount(), 2);

	$res = $sql->fetchAll();
	expect_eq(count($res), 12);

	$sql = $nodeSession->sql("insert into $db.test_table values ('Alessio',56),('Mattia',33),('Lucrezia',67)")->execute();
	expect_false($sql->hasData());
	expect_eq($sql->getAffectedItemsCount(), 3);
	expect_eq($sql->getWarningCount(), 0);
	expect_eq($sql->getColumnCount(), 0);

	$sql = $nodeSession->sql("select * from $db.test_table where age < ? order by age desc limit ?")->bind(14)->bind(2)->execute();
	expect_true($sql->hasData());
	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->getWarningCount(), 0);
	expect_eq($sql->getColumnCount(), 2);

	$res = $sql->fetchOne();
	expect_eq($res['name'],'Cassidy');
	expect_eq($res['age'], 13);
	$res = $sql->fetchOne();
	expect_eq($res['name'],'Polly');
	expect_eq($res['age'], 12);

	try {
		$nodeSession->sql("select wrong from $db.test_table")->execute();
		test_step_failed();
	}catch(Exception $e) {
		expect_eq($e->getMessage(),
				'[HY000] Coulnd\'t fetch data');
		expect_eq($e->getCode(), 10000);
	}

	try {
		//Not tiny!
		$sql = $nodeSession->sql("insert into $db.test_table values ('Carlos',9999)")->execute();
		test_step_failed();
	}catch(Exception $e) {
		expect_eq($e->getMessage(),
				'[HY000] Coulnd\'t fetch data');
		expect_eq($e->getCode(), 10000);
	}

	$sql = $nodeSession->sql("select age, age/0 as x from $db.test_table limit 2")->execute();
	expect_true($sql->hasData());
	expect_eq($sql->getAffectedItemsCount(), 0);
	expect_eq($sql->getWarningCount(), 2);
	$warn = $sql->getWarnings();
	for( $i = 0 ; $i < 2; $i++ ) {
	    expect_eq($warn[0]->message,'');
	    expect_eq($warn[0]->level,2);
	    expect_eq($warn[0]->code,1365);
	}
	expect_eq($sql->getColumnCount(), 2);
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
