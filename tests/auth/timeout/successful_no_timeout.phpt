--TEST--
mysqlx successful connection, no timeout
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/timeout_utils.inc");

function test_successful_no_timeouts($host, $timeout) {
	test_successful_no_timeout($host, $timeout);
	test_successful_no_timeout($host, null);
	test_successful_no_timeout($host, 0);
}

test_successful_no_timeouts("127.0.0.1", 4);
test_successful_no_timeouts("localhost", 7);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@127.0.0.1:%d/?connect-timeout=4
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:%d
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:%d
----------------------
mysqlx://testuser:testpasswd@localhost:%d/?connect-timeout=7
----------------------
mysqlx://testuser:testpasswd@localhost:%d
----------------------
mysqlx://testuser:testpasswd@localhost:%d
----------------------
done!%A
