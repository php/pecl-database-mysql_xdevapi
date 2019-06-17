--TEST--
mysqlx elapsed connection timeout with explicit timeout given
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout(4, __FILE__);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/timeout_utils.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
