--TEST--
check new session reset without reauthentication
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

function get_session_id($session) {
	$query = $session->sql("SELECT CONNECTION_ID()");
	$res = $query->execute()->fetchAll();
	return $res[0]["CONNECTION_ID()"];
}

$pooling_options = '{
	"enabled": true,
  	"maxSize": 5,
  	"queueTimeOut": 10000
}';

$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
$session0 = $client->getSession();
$session_id0 = get_session_id($session0);
create_test_db($session0);
assert_session_valid($session0);

$session0->close();
assert_session_invalid($session0);

$session1 = $client->getSession();
assert_session_valid($session1);
$session_id1 = get_session_id($session1);

const SERVER_SUPPORTING_NEW_SESSION_RESET = 80016;
$svr_version = $session1->getServerVersion();
if (SERVER_SUPPORTING_NEW_SESSION_RESET <= $svr_version) {
	expect_true($session_id0 == $session_id1);
} else {
	expect_true($session_id0 != $session_id1);
}

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
