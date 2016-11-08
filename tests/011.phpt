--TEST--
mysqlx warnings
--SKIPIF--
--FILE--
<?php
	require("connect.inc");
	$test = "00";

	$nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
	$nodeSession->executeSql("create database $db");
	$nodeSession->executeSql("create table $db.test_table(x int)");

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert(['x'])->values([1])->values([2])->values([3])->execute();
	$res = $table->select(['x/0 as bad_x'])->execute();

	if (3 == $res->getWarningCount())
		$test[0] = '1';

	//$warn = $res->getWarnings();
	//var_dump($warn);

	var_dump($test);
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
%A
string(2) "11"
done!%A
