<?php

require(__DIR__."/../auth_utils.inc");

function is_tls_v13_supported() {
 	return PHP_VERSION_ID >= 70400;
}

function are_tls_ciphersuites_supported() {
 	return false;
}

function prepare_tls_uri($tls_options) {
	global $base_uri;
	global $default_schema;
	$tls_uri = $base_uri.'/'.$default_schema.'?'.$tls_options;
	return $tls_uri;
}

function test_tls_connection($tls_options, $expect_success, $verbose = false) {
	$tls_uri = prepare_tls_uri($tls_options);
	$empty_auth_method = "";
	return test_connection($tls_uri, $empty_auth_method, $expect_success, $verbose);
}

function skip_if_tls_v13_not_supported() {
	if (is_tls_v13_supported()) return;
	die('skipped: TLSv1.3 is not supported');
}

function skip_if_tls_ciphersuites_not_supported() {
	if (are_tls_ciphersuites_supported()) return;
	die('skipped: TLS ciphersuites are not supported');
}

?>
