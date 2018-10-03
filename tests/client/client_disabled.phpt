--TEST--
mysqlx client with disabled pooling vs validity of retrieved sessions
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

$pooling_options = '{
  	"queueTimeOut": 1000,
  	"maxIdleTime": 3600,
  	"maxSize": 10,
	"enabled": false
}';

$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
$session0 = $client->getSession();
create_test_db($session0);
assert_session_valid($session0);

$client->close();

assert_session_valid($session0);

$session1 = $client->getSession();
assert_session_valid($session1);

$client->close();

assert_session_valid($session0);
assert_session_valid($session1);

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
