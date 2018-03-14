--TEST--
mysqlx warnings
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

        $session = mysql_xdevapi\getSession($connection_uri);
	$session->executeSql("create database $db");
	$session->executeSql("create table $db.test_table(x int)");

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert(['x'])->values([1])->values([2])->values([3])->execute();
	$res = $table->select(['x/0 as bad_x'])->execute();

	expect_eq($res->getWarningCount(), 3);

	$warn = $res->getWarnings();
	for( $i = 0 ; $i < 3; $i++ ) {
	    //expect_eq($warn[0]->message,'');
	    expect_eq($warn[0]->level,2);
	    expect_eq($warn[0]->code,1365);
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
done!%A
