--TEST--
mysqlx authentication mechanisms
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=4
--FILE--
<?php
require(__DIR__."/../connect.inc");
require(__DIR__."/auth_utils.inc");

function prepare_ssl_query() {
	global $connection_uri;

	$nodeSession = mysql_xdevapi\getSession($connection_uri);
	$mysql_cert_dir = get_mysql_variable($nodeSession, 'datadir');

	if (!is_dir($mysql_cert_dir)) {
		$mysql_cert_dir = __DIR__.'/ssl';
	}
	$rsa_key_path = $mysql_cert_dir.DIRECTORY_SEPARATOR;

	$ssl_query = 'ssl-key=' . $rsa_key_path . 'client-key.pem'
		. '&ssl-cert=' . $rsa_key_path . 'client-cert.pem'
		. '&ssl-ca=' . $rsa_key_path . 'ca.pem';

	return $ssl_query;
}

function prepare_base_secure_uri($ssl_query) {
	global $base_uri;
	$base_secure_uri = $base_uri . '/?' . $ssl_query;
	return $base_secure_uri;
}

function test_connection($base_uri, $auth_mechanism, $expect_success) {
	try {
		$uri = $base_uri;
		if ($auth_mechanism) {
			$uri .= '&auth=' . $auth_mechanism;
		}
		$nodeSession = mysql_xdevapi\getSession($uri);
		if (($expect_success && $nodeSession) || (!$expect_success && !$nodeSession)) {
			test_step_ok();
		} else {
			test_step_failed($uri, 3);
		}
	} catch(Exception $e) {
		if ($expect_success) {
			test_step_failed($uri, 3);
		} else {
			test_step_ok();
		}
	}
}

function test_incorrect_connection($ssl_query) {
	global $base_uri;
	$uri = $base_uri . '/?' . $ssl_query;
	test_connection($uri, null, false);
}

function test_unsecure_connection($auth_mechanism, $expect_success = true) {
	global $connection_uri;
	test_connection($connection_uri, $auth_mechanism, $expect_success);
}

function test_secure_connection($auth_mechanism, $expect_success = true) {
	global $base_secure_uri;
	test_connection($base_secure_uri, $auth_mechanism, $expect_success);
}

// setup
$ssl_query = prepare_ssl_query();
$base_secure_uri = prepare_base_secure_uri($ssl_query);

// incorrect ssl query
test_incorrect_connection($disable_ssl_opt.'&');
test_incorrect_connection($ssl_query.'&=');
test_incorrect_connection($disable_ssl_opt.'&=&');
test_incorrect_connection($ssl_query.'&&&');
test_incorrect_connection($disable_ssl_opt.'&=mysql41&');
test_incorrect_connection($ssl_query.'&auth&');
test_incorrect_connection($disable_ssl_opt.'&auth');
test_incorrect_connection($ssl_query.'&auth=');
test_incorrect_connection($disable_ssl_opt.'&auth==');
test_incorrect_connection($ssl_query.'&auth==&');
test_incorrect_connection($disable_ssl_opt.'&auth=plain&&');
test_incorrect_connection($ssl_query.'&&auth=&');
test_incorrect_connection($disable_ssl_opt.'&&auth=mysql41');
test_incorrect_connection($ssl_query.'&&auth&=plain');
test_incorrect_connection($disable_ssl_opt.'&auth=plain&&auth=external');
test_incorrect_connection($ssl_query.'auth=plain');
test_incorrect_connection($disable_ssl_opt.'auth=');
test_incorrect_connection($ssl_query.'auth=sha256_memory&');
test_incorrect_connection($disable_ssl_opt.'&auth=sha256_memory&&&');
test_incorrect_connection('&'.$ssl_query);
test_incorrect_connection('&'.$disable_ssl_opt);
test_incorrect_connection('&'.$disable_ssl_opt.'&auth=mysql41');
test_incorrect_connection('&'.$disable_ssl_opt.'auth=sha256_memory&');

// unsecure
test_unsecure_connection(null);

$test_user = 'user_sha256_mem';
	create_test_user('user_native', 'mysql_native_password');

create_test_user($test_user, 'caching_sha2_password');

test_unsecure_connection('MYSQL41');
test_unsecure_connection('PLAIN', false);
test_unsecure_connection('SHA256_MEMORY', false);
test_unsecure_connection('EXTERNAL', false);
test_unsecure_connection('UNSUPPORTED', false);

test_unsecure_connection('mysql41');
test_unsecure_connection('plain', false);
test_unsecure_connection('sha256_memory', false);
test_unsecure_connection('external', false);
test_unsecure_connection('nonworking', false);

test_unsecure_connection('MySQL41');
test_unsecure_connection('plAin', false);
test_unsecure_connection('ShA256_MeMorY', false);
test_unsecure_connection('ExTeRnAl', false);
test_unsecure_connection('NonSupported', false);

// secure
test_secure_connection(null);

test_secure_connection('MYSQL41');
test_secure_connection('PLAIN');
test_secure_connection('SHA256_MEMORY', false);
test_secure_connection('EXTERNAL', false);
test_secure_connection('WRONG', false);

test_secure_connection('mysql41');
test_secure_connection('plain');
test_secure_connection('sha256_memory', false);
test_secure_connection('external', false);
test_secure_connection('non-existent', false);

test_secure_connection('mySql41');
test_secure_connection('PLain');
test_secure_connection('sHa256_mEmOrY', false);
test_secure_connection('ExternaL', false);
test_secure_connection('InCorrect', false);


$test_user = 'user_sha256_mem';
	create_test_user('user_native', 'mysql_native_password');

create_test_user($test_user, 'caching_sha2_password');


verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
