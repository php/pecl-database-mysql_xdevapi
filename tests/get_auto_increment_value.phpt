--TEST--
mysqlx getAutoIncrementValue
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

        $session = mysql_xdevapi\getSession($connection_uri);
	$session->sql("create database $db")->execute();
	$session->sql("create table $db.test_table(id int not null auto_increment,name char(30), primary key (id))")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	$res = $table->insert(['name'])->values(['name'=>'Luigi']);
	$res = $res->values(['name'=>'Carlo'])->execute();

	expect_eq($res->getAffectedItemsCount(), 2);
	expect_eq($res->getAutoIncrementValue(), 1);

	$res = $table->insert(['name'])->values(['name'=>'Arturo'])->execute();

	expect_eq($res->getAffectedItemsCount(), 1);
	expect_eq($res->getAutoIncrementValue(), 3);

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
