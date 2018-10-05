--TEST--
mysqlx client fail due to incorrect uri
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
  	"maxSize": 10,
  	"maxIdleTime": 3600,
  	"queueTimeOut": 1000
}';

try {
	$client = mysql_xdevapi\getClient("this_is_incorrect_uri&=mysql41&", $pooling_options);
	$session = $client->getSession();
	test_step_failed("shouldn't retrieve session for incorrect uri");
} catch (Exception $e) {
	test_step_ok();
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
done!%A
