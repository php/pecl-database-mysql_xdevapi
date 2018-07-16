--TEST--
mysqlx default connection timeout
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout($Non_routable_host4, 80, null);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@172.31.255.255:80
----------------------
connecting time %f
______________________
done!%A
