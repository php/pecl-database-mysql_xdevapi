<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

function assert_session_valid($session) {
	global $test_schema_name;
	$schema = $session->getSchema($test_schema_name);
	expect_true($schema->existsInDatabase());
}

function assert_session_invalid($session) {
	try {
		$serverVersion = $session->getServerVersion();
		test_step_failed('invalid session expected');
	} catch(Exception $e) {
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

function assert_client_session_fail_with_options($pooling_options) {
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
