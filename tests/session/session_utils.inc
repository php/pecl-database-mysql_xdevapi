<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

function assert_session_valid($session) {
	global $test_schema_name;
	$schema = $session->getSchema($test_schema_name);
	expect_true($schema->existsInDatabase());
}

function assert_session_valid_server_ver($session) {
	global $test_schema_name;
	$server_ver = $session->getServerVersion();
	expect_true(is_int($server_ver) && ($server_ver != 0));
}

function assert_session_invalid($session) {
	try {
		$serverVersion = $session->getServerVersion();
		test_step_failed('invalid session expected');
	} catch(Exception $e) {
		echo $e->getMessage(), PHP_EOL;
		test_step_ok();
	}
}

?>
