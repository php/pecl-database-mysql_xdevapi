--TEST--
mysqlx simple session test open
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

verify_expectations();
print "done!\n";
?>
--EXPECTF--
done!%A
