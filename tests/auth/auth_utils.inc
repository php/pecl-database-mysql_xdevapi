<?php

require_once(__DIR__."/../connect.inc");

function get_test_user_passwd($user) {
	return $user . '_password';
}

function drop_test_user($user) {
	global $host;
	$session = create_test_session();
	$query = "DROP USER IF EXISTS '$user'@'$host'";
	$session->executeSql($query);
}

function create_test_user($user, $auth_plugin) {
	global $host;
	$session = create_test_session();
	$password = get_test_user_passwd($user);
	$query = "CREATE USER '$user'@'$host' IDENTIFIED WITH '$auth_plugin' BY '$password'";
	// echo $query, "\n";
	$session->executeSql($query);
}

function reset_test_user($user, $auth_plugin) {
	drop_test_user($user);
	create_test_user($user, $auth_plugin);
}

// -----------

function prepare_base_uri($user) {
	global $scheme;
	global $host;
	global $port;
	$password = get_test_user_passwd($user);
	$base_uri = $scheme . '://' . $user . ':' . $password . '@' . $host . ':' . $port;
	return $base_uri;
}

function test_connection($base_uri, $auth_method, $expect_success) {
	try {
		$uri = $base_uri;
		if ($auth_method) {
			$uri .= '&auth=' . $auth_method;
		}
		$session = mysql_xdevapi\getSession($uri);
		if (($expect_success && $session) || (!$expect_success && !$session)) {
			test_step_ok();
		} else {
			test_step_failed($uri);
		}
	} catch(Exception $e) {
		if ($expect_success) {
			test_step_failed($uri, 3);
		} else {
			test_step_ok();
		}
	}
}

// -----------

function test_incorrect_connection($ssl_query) {
	global $base_uri;
	$uri = $base_uri . '/?' . $ssl_query;
	test_connection($uri, null, false);
}

// -----------

function prepare_unsecure_uri($user) {
	global $disable_ssl_opt;
	$base_uri = prepare_base_uri($user);
	$uri = $base_uri . '/?' . $disable_ssl_opt;
	return $uri;
}

function test_unsecure_connection($user, $auth_method, $expect_success = true) {
	$uri = prepare_unsecure_uri($user);
	test_connection($uri, $auth_method, $expect_success);
}

// -----------

function prepare_ssl_query() {
	$session = create_test_session();
	$mysql_cert_dir = get_mysql_variable($session, 'datadir');

	if (!is_dir($mysql_cert_dir)) {
		$mysql_cert_dir = __DIR__ . '/ssl';
	}
	$rsa_key_path = $mysql_cert_dir . DIRECTORY_SEPARATOR;

	$ssl_query = 'ssl-key=' . $rsa_key_path . 'client-key.pem'
		. '&ssl-cert=' . $rsa_key_path . 'client-cert.pem'
		. '&ssl-ca=' . $rsa_key_path . 'ca.pem';

	return $ssl_query;
}

function prepare_secure_uri($user) {
	global $ssl_query;
	$base_uri = prepare_base_uri($user);
	$uri = $base_uri . '/?' . $ssl_query;
	return $uri;
}

function test_secure_connection($user, $auth_method, $expect_success = true) {
	$uri = prepare_secure_uri($user);
	test_connection($uri, $auth_method, $expect_success);
}

?>
