--TEST--
mysqlx test initializer
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	// it is meant only for init/clean state before running tests
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
