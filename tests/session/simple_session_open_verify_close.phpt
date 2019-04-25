--TEST--
mysqlx simple session test open verify close
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/session_utils.inc");

$session = mysql_xdevapi\getSession($connection_uri);
create_test_db($session);
assert_session_valid($session);

$session->close();
assert_session_invalid($session);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
[10056][HY000] Session closed.
done!%A
