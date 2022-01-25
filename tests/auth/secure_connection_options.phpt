--TEST--
mysqlx Align TLS option checking across connectors
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

function test_ssl_connection_options($user, $ssl_mode, $ssl_option = '', $expect_success = true) {
    global $connection_uri;
    $uri = $connection_uri;
    try {
        $uri .= '?ssl-mode=' . $ssl_mode;
        if( $ssl_option ) {
            $uri .= '&' . $ssl_option;
        }

		$session = mysql_xdevapi\getSession($uri);
		if (($expect_success && $session) || (!$expect_success && !$session)) {
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

// setup
// $test_user = $Test_user_native;
// reset_test_user($test_user, 'mysql_native_password');

// First connect without SSL and verify if SSL is supported
$session = mysql_xdevapi\getSession($connection_uri);

// Any SSL variable?
$ssl_info_query = $session->sql("show variables like '%ssl%'")->execute();
$ssl_info = $ssl_info_query->fetchAll();
$mysql_datadir = get_mysql_variable($session, 'datadir');

if( false == is_dir( $mysql_datadir ) ) {
    // We've got nothing, let's use a standard dir location, perhaps we're lucky
	$mysql_datadir = __DIR__.'/ssl';
}
$rsa_key_path = $mysql_datadir.DIRECTORY_SEPARATOR;
$ssl_supported = false;
$uri_ssl_data = '';

if( file_exists( $rsa_key_path ) ) {
    //Attempt a connection using these ssl settings
    $new_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?';
    $uri_ssl_data = 'ssl-key=' . $rsa_key_path . 'client-key.pem&';
    $uri_ssl_data .= 'ssl-cert=' . $rsa_key_path . 'client-cert.pem&';
    $uri_ssl_data .= 'ssl-ca=' . $rsa_key_path . 'ca.pem';
    $new_uri .= $uri_ssl_data;
    $session = mysql_xdevapi\getSession($new_uri);
    $res = $session->sql('SELECT USER()')->execute();
    $userdata = $res->fetchOne();

    // Make sure we're getting it right, check the USER()
    $current_user = $userdata['USER()'];
    expect_eq($current_user, $user.'@'.$host);
    if( $current_user == $user.'@'.$host ) {
        $ssl_supported = true;
    }
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
// We can check this scenario only if SSL is supported
if( $ssl_supported == true ) {
    test_ssl_connection_options($user,'DISABLED','ssl-mode=REQUIRED&'.$uri_ssl_data);
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
