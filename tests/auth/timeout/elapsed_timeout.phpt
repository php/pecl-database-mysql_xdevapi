--TEST--
mysqlx elapsed connection timeout with explicit timeout given
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout($Non_routable_host, 33060, 3);
test_elapsed_timeout($Non_routable_host1, 3306, 2);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@198.51.100.23:33060/?connect-timeout=3
----------------------
connecting time %f
______________________
mysqlx://testuser:testpasswd@192.0.2.254:3306/?connect-timeout=2
----------------------
connecting time %f
______________________
done!%A
