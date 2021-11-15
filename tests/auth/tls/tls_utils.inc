<?php

require(__DIR__."/../auth_utils.inc");

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

function is_tls_v13_supported() {
	if (OPENSSL_VERSION_NUMBER < 0x10101000) return false;

	try {
		$tls_uri = prepare_tls_uri('');
		$session = mysql_xdevapi\getSession($tls_uri);
		if($session) {
			$result = $session->sql("SELECT @@tls_version")->execute();
			$data = $result->fetchOne();
			if(strstr($data['@@tls_version'], 'TLSv1.3')){
				return true;
			}
		}
	} catch(Exception $e) { }
	
	return false;
}

function are_tls_ciphersuites_supported() {
	return true;
}

function skip_if_tls_v13_not_supported() {
	if (is_tls_v13_supported()) return;
	die('skip TLSv1.3 is not supported');
}

function skip_if_tls_ciphersuites_not_supported() {
	if (are_tls_ciphersuites_supported()) return;
	die('skip TLS ciphersuites are not supported');
}

function test_tls_version($uri, $expect_success, $version)
{
	try {
		$tls_uri = prepare_tls_uri($uri);
		$session = mysql_xdevapi\getSession($tls_uri);
		if ($expect_success && $session) {
			$result = $session->sql("SHOW STATUS LIKE 'mysqlx_ssl_version'")->execute();
			$data = $result->fetchOne();
			if( $data['Value'] == $version ) {
				test_step_ok();
			} else {
				test_step_failed($uri . " expected version: (".$version.") " . "SHOW STATUS LIKE 'mysqlx_ssl_version': " . $data['Value'] );
			}
		} else if (!$expect_success && !$session) {
			test_step_ok();
		} else {
			test_step_failed($uri);
		}
	} catch(Exception $e) {
		echo $e->getMessage(), PHP_EOL;
		if ($expect_success) {
			test_step_failed($uri);
		} else {
			test_step_ok();
		}
	}
}

?>
