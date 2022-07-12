--TEST--
mysqlx successful connection, no timeout
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
require_once(__DIR__."/timeout_utils.inc");

function test_successful_not_elapsed_timeouts($host, $timeout) {
	test_successful_not_elapsed_timeout($host, $timeout);
	test_successful_not_elapsed_timeout($host, null);
	test_successful_not_elapsed_timeout($host, 0);
}

test_successful_not_elapsed_timeouts($host, 4);
test_successful_not_elapsed_timeouts($host, 7);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/timeout_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://%s:%S@%S:%d/?connect-timeout=4
----------------------
mysqlx://%s:%S@%S:%d
----------------------
mysqlx://%s:%S@%S:%d/?connect-timeout=0
----------------------
mysqlx://%s:%S@%S:%d/?connect-timeout=7
----------------------
mysqlx://%s:%S@%S:%d
----------------------
mysqlx://%s:%S@%S:%d/?connect-timeout=0
----------------------
done!%A
