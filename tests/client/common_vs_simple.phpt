--TEST--
mysqlx get one common session, then check session from client
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

$common_session = mysql_xdevapi\getSession($connection_uri);
create_test_db($common_session);
assert_session_valid($common_session);

$pooling_options = '{
	"enabled": true,
  	"maxSize": 5
}';

$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
$session = $client->getSession();
assert_session_valid($session);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
