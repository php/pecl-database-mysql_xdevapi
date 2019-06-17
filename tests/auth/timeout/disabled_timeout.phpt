--TEST--
mysqlx disabled connection timeout
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/timeout_utils.inc");

test_elapsed_timeout(0, __FILE__);

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
