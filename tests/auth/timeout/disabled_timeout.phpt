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

test_elapsed_timeout($Non_routable_host2, 83, 0);
test_elapsed_timeout($Non_routable_host3, null, 0);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@192.168.255.255:83
----------------------
connecting time %f
______________________
mysqlx://testuser:testpasswd@172.16.0.0
----------------------
connecting time %f
______________________
done!%A
