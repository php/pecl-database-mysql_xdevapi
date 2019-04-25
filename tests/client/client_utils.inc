<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");
require_once(__DIR__.DIRECTORY_SEPARATOR."../session/session_utils.inc");

// -------

function triggerQueueTimeoutAtGetSession($client) {
	try {
		$session = $client->getSession();
		test_step_failed("connection pool should be full");
	} catch (Exception $e) {
		echo $e->getMessage(), PHP_EOL;
		test_step_ok();
	}
}

// -------

function assert_client_session_with_options($pooling_options) {
	global $connection_uri;
	$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
	$session = $client->getSession();
	create_test_db($session);
	assert_session_valid($session);
}

function assert_client_fail_with_options($pooling_options) {
	global $connection_uri;
	try {
		$client = mysql_xdevapi\getClient($connection_uri, $pooling_options);
		test_step_failed("client shouldn't accept incorrect options");
	} catch (Exception $e) {
		echo $e->getMessage(), PHP_EOL;
		test_step_ok();
	}
}

?>
