--TEST--
mysqlx disabled connection timeout
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=3
--FILE--
<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout($Non_routable_host2, 3306, 0);
test_elapsed_timeout($Non_routable_host3, 33060, 0);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@203.0.113.4:3306/?connect-timeout=0
----------------------
connecting time %f
______________________
mysqlx://testuser:testpasswd@198.51.100.8:33060/?connect-timeout=0
----------------------
connecting time %f
______________________
done!%A
