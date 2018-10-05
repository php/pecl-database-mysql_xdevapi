--TEST--
mysqlx trigger prune expired idle connections in pool
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

// in msec
$maxIdleTime = 100;

$pooling_options = '{
  	"maxSize": 3,
  	"maxIdleTime": '.$maxIdleTime.'
}';

$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
$session0 = $client->getSession();
create_test_db($session0);
assert_session_valid($session0);

$session1 = $client->getSession();
assert_session_valid($session1);

$session2 = $client->getSession();
assert_session_valid($session2);

$session0->close();
assert_session_invalid($session0);

$session1->close();
assert_session_invalid($session1);

$session2->close();
assert_session_invalid($session2);

msleep(2 * $maxIdleTime);

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
[10056][HY000] Session closed.
[10056][HY000] Session closed.
done!%A
