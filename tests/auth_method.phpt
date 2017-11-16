--TEST--
mysqlx authentication methods
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
	require(__DIR__."/connect.inc");

	function prepare_base_secure_uri() {
		global $base_uri;
		global $connection_uri;

		$nodeSession = mysql_xdevapi\getSession($connection_uri);
		$mysql_cert_dir = get_mysql_variable($nodeSession, 'datadir');

		if (!is_dir($mysql_cert_dir)) {
			$mysql_cert_dir = __DIR__.'/ssl';
		}
		$rsa_key_path = $mysql_cert_dir.DIRECTORY_SEPARATOR;

		$base_secure_uri = $base_uri
			. '/?ssl-key=' . $rsa_key_path . 'client-key.pem'
			. '&ssl-cert=' . $rsa_key_path . 'client-cert.pem'
			. '&ssl-ca=' . $rsa_key_path . 'ca.pem';

		return $base_secure_uri;
	}

	function test_connection($base_uri, $auth_method, $expect_success) {
        try {
			$uri = $base_uri;
			if ($auth_method) {
				$uri .= '&auth=' . $auth_method;
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

	function test_unsecure_connection($auth_method, $expect_success = true) {
		global $connection_uri;
		test_connection($connection_uri, $auth_method, $expect_success);
	}

	function test_secure_connection($auth_method, $expect_success = true) {
		global $base_secure_uri;
		test_connection($base_secure_uri, $auth_method, $expect_success);
	}

	// unsecure
	test_unsecure_connection(null);

	test_unsecure_connection('MYSQL41');
	test_unsecure_connection('PLAIN', false);
	test_unsecure_connection('EXTERNAL', false);
	test_unsecure_connection('UNSUPPORTED', false);

	test_unsecure_connection('mysql41');
	test_unsecure_connection('plain', false);
	test_unsecure_connection('external', false);
	test_unsecure_connection('nonworking', false);

	test_unsecure_connection('MySQL41');
	test_unsecure_connection('plAin', false);
	test_unsecure_connection('ExTeRnAl', false);
	test_unsecure_connection('NonSupported', false);

	// secure
	$base_secure_uri = prepare_base_secure_uri();

	test_secure_connection(null);

	test_secure_connection('MYSQL41');
	test_secure_connection('PLAIN');
	test_secure_connection('EXTERNAL', false);
	test_secure_connection('WRONG', false);

	test_secure_connection('mysql41');
	test_secure_connection('plain');
	test_secure_connection('external', false);
	test_secure_connection('non-existent', false);

	test_secure_connection('mySql41');
	test_secure_connection('PLain');
	test_secure_connection('ExternaL', false);
	test_secure_connection('InCorrect', false);

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require(__DIR__."/connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
