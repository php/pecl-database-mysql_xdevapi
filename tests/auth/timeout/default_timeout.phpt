--TEST--
mysqlx default connection timeout
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout(null, __FILE__);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
