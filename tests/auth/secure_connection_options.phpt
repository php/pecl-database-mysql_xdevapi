--TEST--
mysqlx Align TLS option checking across connectors
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

function test_ssl_connection_options($user, $ssl_mode, $ssl_option = '', $expect_success = true, $only_report = false) {
	global $connection_uri;
	$uri = $connection_uri;
	try {
		$uri .= '?ssl-mode=' . $ssl_mode;
		if( $ssl_option ) {
			$uri .= '&' . $ssl_option;
		}

		$session = mysql_xdevapi\getSession($uri);
		if (!( ($expect_success && $session) || (!$expect_success && !$session)) ) {
			if( $only_report == false ) {
				test_step_failed($uri);
			}
			return false;
		}
	} catch(Exception $e) {
		echo $e->getMessage(), PHP_EOL;
		if ($expect_success) {
			if( $only_report == false ) {
			test_step_failed($uri);
			}
			return false;
		}
	}
	if( $only_report == false ) {
		test_step_ok();
	}
	return true;
}

// Now test the WL14844 changes

test_ssl_connection_options($user,'DISABLED');
test_ssl_connection_options($user,'DISABLED','ssl-ca=foo');
test_ssl_connection_options($user,'DISABLED','ssl-capath=foo');
test_ssl_connection_options($user,'DISABLED','ssl-cert=foo');
test_ssl_connection_options($user,'DISABLED','ssl-cipher=foo');
test_ssl_connection_options($user,'DISABLED','ssl-ciphers=foo');
test_ssl_connection_options($user,'DISABLED','ssl-key=foo');
test_ssl_connection_options($user,'DISABLED','ssl-no-defaults');
test_ssl_connection_options($user,'DISABLED','ssl-foo=foo',false);
test_ssl_connection_options($user,'REQUIRED','ssl-mode=DISABLED');
// connect using the default uri, check for ssl
$session = mysql_xdevapi\getSession($connection_uri);
$ssl_info_query = $session->sql("show variables like '%have_ssl%'")->execute();
$ssl_info = $ssl_info_query->fetchAll();
if( !empty($ssl_info) ) {
	$ssl_supported = $ssl_info[0][ "Value" ] == "YES";
} else {
	$ssl_support = false;
}
$connection_result = test_ssl_connection_options($user,'DISABLED','ssl-mode=REQUIRED',true ,true);
if( $connection_result && !$ssl_supported ) {
	test_step_failed();
} else if( !$connection_result && $ssl_supported ) {
	test_step_failed();
} else {
	test_step_ok();
}

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
[10063][HY000] Unknown client connection option in Uri: ssl-foo
done!%A
