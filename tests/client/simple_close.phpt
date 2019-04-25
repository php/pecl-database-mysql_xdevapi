--TEST--
mysqlx client open and close session from pool
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

$pooling_options = '{
	"enabled": true,
  	"maxSize": 2,
  	"queueTimeOut": 200
}';

$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
$session0 = $client->getSession();
create_test_db($session0);
assert_session_valid($session0);

$session0->close();
assert_session_invalid($session0);

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
