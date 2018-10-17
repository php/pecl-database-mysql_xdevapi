--TEST--
mysqlx simple session test open close
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/session_utils.inc");

$session = mysql_xdevapi\getSession($connection_uri);
assert_session_valid_server_ver($session);

$session->close();
assert_session_invalid($session);

verify_expectations();
print "done!\n";
?>
--EXPECTF--
[10056][HY000] Session closed.
done!%A
